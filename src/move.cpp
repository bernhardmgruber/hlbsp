#include "move.h"

#include "Bsp.h"
#include <cassert>
#include <iostream>
#include <numbers>

// the following code is largly inspired by WinQuake's sv_move.c and Valve's Halflife SDK's pm_shared.c

namespace {
	constexpr auto distEpsilon = 0.03125f;
	constexpr auto movevars_accelerate = 5.6f;
	constexpr auto movevars_stepsize = 18.0f;
	constexpr auto movevars_bounce = 0.0f;
	constexpr auto movevars_gravity = 800.0f;
	constexpr auto movevars_airaccelerate = 12.0f; // TODO
	constexpr auto movevars_friction = 4.0f;
	constexpr auto movevars_edgefriction = 2.0f; // from quake 1
	constexpr auto movevars_stopspeed = 75.0f;
	constexpr auto movevars_maxvelocity = 300.0f;
	constexpr auto maxSpeed = 192.0f;
	constexpr auto stopEpsilon = 0.1f;
	constexpr auto bunnyJumpMaxSpeedFactor = 1.7f;
	constexpr auto deadViewHeight = -8;

	struct Trace {
		bool allsolid;   // if true, plane is not valid
		bool startsolid; // if true, the initial point was in a solid area
		bool inopen, inwater;
		float fraction;     // time completed, 1.0 = didn't hit anything
		glm::vec3 endpos;   // final position
		bsp30::Plane plane; // surface normal at impact
	};

	int hullPointContents(const Hull& hull, int nodeIndex, glm::vec3 p) {
		while (nodeIndex >= 0) {
			assert(nodeIndex >= hull.firstclipnode && nodeIndex <= hull.lastclipnode);
			const auto& node = hull.clipnodes[nodeIndex];
			const auto& plane = hull.planes[node.planeIndex];
			const auto d = glm::dot(plane.normal, p) - plane.dist;
			nodeIndex = node.childIndex[d < 0];
		}
		return nodeIndex;
	}

	bool recursiveHullCheck(const Hull& hull, int nodeIndex, float p1f, float p2f, glm::vec3 p1, glm::vec3 p2, Trace& trace) {
		if (nodeIndex < 0) {
			if (nodeIndex != bsp30::CONTENTS_SOLID) {
				trace.allsolid = false;
				if (nodeIndex == bsp30::CONTENTS_EMPTY)
					trace.inopen = true;
				else
					trace.inwater = true;
			} else
				trace.startsolid = true;
			return true;
		}

		assert(nodeIndex >= hull.firstclipnode && nodeIndex <= hull.lastclipnode);

		// find the point distances
		const auto& node = hull.clipnodes[nodeIndex];
		const auto& plane = hull.planes[node.planeIndex];
		const auto t1 = glm::dot(plane.normal, p1) - plane.dist;
		const auto t2 = glm::dot(plane.normal, p2) - plane.dist;
		if (t1 >= 0 && t2 >= 0)
			return recursiveHullCheck(hull, node.childIndex[0], p1f, p2f, p1, p2, trace);
		if (t1 < 0 && t2 < 0)
			return recursiveHullCheck(hull, node.childIndex[1], p1f, p2f, p1, p2, trace);

		float frac;
		if (t1 < 0)
			frac = (t1 + distEpsilon) / (t1 - t2);
		else
			frac = (t1 - distEpsilon) / (t1 - t2);
		frac = std::clamp(frac, 0.0f, 1.0f);

		float midf = p1f + (p2f - p1f) * frac;
		glm::vec3 mid = p1 + frac * (p2 - p1);
		const auto side = t1 < 0;

		// handle all nodes on the near side of this node's plane
		if (!recursiveHullCheck(hull, node.childIndex[side], p1f, midf, p1, mid, trace))
			return false;

		// check the content of the node on the far side of this node's plane
		if (hullPointContents(hull, node.childIndex[!side], mid) != bsp30::CONTENTS_SOLID)
			// not solid, continue checking on the far side of this node's plane
			return recursiveHullCheck(hull, node.childIndex[!side], midf, p2f, mid, p2, trace);

		if (trace.allsolid)
			return false;

		// the far side of this node's plane is solid, this node's plane is where we hit
		trace.plane = plane;
		if (side) {
			trace.plane.normal = -trace.plane.normal;
			trace.plane.dist = -trace.plane.dist;
		}

		// taken from WinQuake, shouldn't really happen, but does occasionally
		// step backward until the collision point is not classified as solid
		while (hullPointContents(hull, hull.firstclipnode, mid) == bsp30::CONTENTS_SOLID) { // shouldn't really happen, but does occasionally
			frac -= 0.1f;
			if (frac < 0) {
				trace.fraction = midf;
				trace.endpos = mid;
				std::clog << "backup past 0\n";
				return false;
			}
			midf = p1f + (p2f - p1f) * frac;
			mid = p1 + frac * (p2 - p1);
		}

		trace.fraction = midf;
		trace.endpos = mid;

		return false;
	}

	auto playerTrace(const Hull& hull, glm::vec3 start, glm::vec3 end) {
		Trace t{};
		t.fraction = 1;
		t.allsolid = true;
		t.endpos = end;
		recursiveHullCheck(hull, hull.firstclipnode, 0, 1, start, end, t);
		return t;
	}

	auto clipVelocity(glm::vec3 in, glm::vec3 normal, float overbounce) -> glm::vec3 {
		const auto backoff = glm::dot(in, normal) * overbounce;
		glm::vec3 out = in - normal * backoff;
		for (int i = 0; i < 3; i++) {
			if (std::abs(out[i]) < stopEpsilon)
				out[i] = 0;
		}
		return out;
	}

	void noClip(PlayerMove& pmove) {
		pmove.forward = glm::normalize(pmove.forward);
		pmove.right = glm::normalize(pmove.right);

		glm::vec3 wishvel = pmove.forward * pmove.cmd.forwardmove + pmove.right * pmove.cmd.sidemove;
		wishvel[2] += pmove.cmd.upmove;

		pmove.origin += pmove.frametime * wishvel;
		pmove.velocity = {};
	}

	void flyMove(const Hull& hull, PlayerMove& pmove) {
		constexpr auto maxBumps = 4;

		auto originalVelocity = pmove.velocity;
		const auto primalVelocity = pmove.velocity;
		float timeLeft = pmove.frametime;
		float totalFraction = 0;
		std::vector<glm::vec3> planeNormals;
		for (int bump = 0; bump < maxBumps; bump++) {
			if (pmove.velocity == glm::vec3{})
				break;

			const auto end = pmove.origin + timeLeft * pmove.velocity;
			const Trace trace = playerTrace(hull, pmove.origin, end);

			if (trace.allsolid) {
				pmove.velocity = {};
				return;
			}

			totalFraction += trace.fraction;

			if (trace.fraction > 0) {
				pmove.origin = trace.endpos;
				originalVelocity = pmove.velocity;
				planeNormals.clear();
			}

			if (trace.fraction == 1)
				break;

			timeLeft -= timeLeft * trace.fraction;

			planeNormals.push_back(trace.plane.normal);

			// modify originalVelocity so it parallels all of the clip planes
			if (pmove.onground == -1 || pmove.friction != 1) {
				glm::vec3 newVelocity{};
				for (const auto& planeNormal : planeNormals) {
					if (planeNormal.z > 0.7f) { // floor or slope
						newVelocity = clipVelocity(originalVelocity, planeNormal, 1);
						originalVelocity = newVelocity;
					} else
						newVelocity = clipVelocity(originalVelocity, planeNormal, 1.0f + movevars_bounce * (1 - pmove.friction));
				}

				pmove.velocity = newVelocity;
				originalVelocity = newVelocity;
			} else {
				size_t i;
				for (i = 0; i < planeNormals.size(); i++) {
					pmove.velocity = clipVelocity(originalVelocity, planeNormals[i], 1);
					size_t j;
					for (j = 0; j < planeNormals.size(); j++)
						if (j != i) {
							if (glm::dot(pmove.velocity, planeNormals[j]) < 0)
								break;
						}
					if (j == planeNormals.size()) // didn't have to clip, so we're ok
						break;
				}

				// did we clip against all planes?
				if (i == planeNormals.size()) {
					if (planeNormals.size() != 2) {
						pmove.velocity = {};
						break;
					}
					const auto dir = glm::cross(planeNormals[0], planeNormals[1]);
					pmove.velocity = dir * glm::dot(dir, pmove.velocity);
				}

				// avoid tiny occilations in sloping corners
				if (glm::dot(pmove.velocity, primalVelocity) <= 0) {
					pmove.velocity = {};
					break;
				}
			}
		}

		if (totalFraction == 0)
			pmove.velocity = {};
	}

	void accelerate(PlayerMove& pmove, glm::vec3 wishdir, float wishspeed) {
		if (pmove.dead || pmove.waterjumptime)
			return;

		const auto currentspeed = glm::dot(pmove.velocity, wishdir);
		const auto addspeed = wishspeed - currentspeed;
		if (addspeed <= 0)
			return;

		float accelspeed = movevars_accelerate * pmove.frametime * wishspeed * pmove.friction;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		pmove.velocity += accelspeed * wishdir;
	}

	auto normalizeIfNonZero(glm::vec3 v) {
		if (const auto l = glm::length(v))
			v /= l;
		return v;
	}

	void walkMove(const Hull& hull, PlayerMove& pmove) {
		const auto& fmove = pmove.cmd.forwardmove;
		const auto& smove = pmove.cmd.sidemove;

		pmove.forward[2] = 0;
		pmove.right[2] = 0;

		pmove.forward = glm::normalize(pmove.forward);
		pmove.right = glm::normalize(pmove.right);

		glm::vec3 wishvel = pmove.forward * fmove + pmove.right * smove;
		wishvel[2] = 0;

		auto wishdir = normalizeIfNonZero(wishvel);
		auto wishspeed = glm::length(wishvel);
		if (wishspeed > maxSpeed) {
			wishvel = (maxSpeed / wishspeed) * wishvel;
			wishspeed = maxSpeed;
		}

		pmove.velocity[2] = 0;
		accelerate(pmove, wishdir, wishspeed);
		pmove.velocity[2] = 0;
		if (glm::length(pmove.velocity) < 1) {
			pmove.velocity = {};
			return;
		}

		const int oldonground = pmove.onground;

		// first try just moving to the destination
		auto dest = pmove.origin;
		dest[0] += pmove.velocity[0] * pmove.frametime;
		dest[1] += pmove.velocity[1] * pmove.frametime;

		auto trace = playerTrace(hull, pmove.origin, dest);
		if (trace.fraction == 1) {
			pmove.origin = trace.endpos;
			return;
		}

		// don't walk up stairs if not on ground
		if (oldonground == -1 && pmove.waterlevel == 0)
			return;

		if (pmove.waterjumptime)
			return;

		// try sliding forward both on ground and up 16 pixels. take the move that goes the farthest
		auto original = pmove.origin;
		auto originalvel = pmove.velocity;
		flyMove(hull, pmove);

		auto down = pmove.origin;
		auto downvel = pmove.velocity;
		pmove.origin = original;
		pmove.velocity = originalvel;

		// start out up one stair height
		dest = pmove.origin;
		dest[2] += movevars_stepsize;

		trace = playerTrace(hull, pmove.origin, dest);
		// if we started okay and made it part of the way at least, copy the results to the movement start position and then run another move try.
		if (!trace.startsolid && !trace.allsolid)
			pmove.origin = trace.endpos;

		flyMove(hull, pmove);

		// try going back down the stepheight
		dest = pmove.origin;
		dest[2] -= movevars_stepsize;
		trace = playerTrace(hull, pmove.origin, dest);

		// if we are not on the ground any more then use the original movement attempt
		if (trace.plane.normal[2] < 0.7)
			goto usedown;
		if (!trace.startsolid && !trace.allsolid)
			pmove.origin = trace.endpos;
		pmove.up = pmove.origin;

		// decide which one went farther
		const float downdist = std::pow(down[0] - original[0], 2) + std::pow(down[1] - original[1], 2);
		const float updist = std::pow(pmove.up[0] - original[0], 2) + std::pow(pmove.up[1] - original[1], 2);

		if (downdist > updist) {
		usedown:
			pmove.origin = down;
			pmove.velocity = downvel;
		} else
			pmove.velocity[2] = downvel[2];
	}

	void angleVectors(glm::vec3 angles, glm::vec3& forward, glm::vec3& right, glm::vec3& up) {
		auto angle = angles.y * (std::numbers::pi * 2 / 360);
		auto sy = sin(angle);
		auto cy = cos(angle);
		angle = angles.x * (std::numbers::pi * 2 / 360);
		auto sp = sin(angle);
		auto cp = cos(angle);
		angle = angles.z * (std::numbers::pi * 2 / 360);
		auto sr = sin(angle);
		auto cr = cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}

	auto inWater(PlayerMove& pmove) {
		return pmove.waterlevel > 1;
	}

	void checkVelocity(PlayerMove& pmove) {
		for (int i = 0; i < 3; i++) {
			if (std::isnan(pmove.velocity[i]))
				pmove.velocity[i] = 0;
			if (std::isnan(pmove.origin[i]))
				pmove.origin[i] = 0;

			pmove.velocity[i] = std::clamp(pmove.velocity[i], -movevars_maxvelocity, movevars_maxvelocity);
		}
	}

	// same as fixupGravityVelocity() because basevelocity is not implemented
	void addCorrectGravity(PlayerMove& pmove) {
		if (pmove.waterjumptime)
			return;

		float ent_gravity = 1.0f;
		if (pmove.gravity)
			ent_gravity = pmove.gravity;
		pmove.velocity[2] -= ent_gravity * movevars_gravity * pmove.frametime * 0.5f;

		checkVelocity(pmove);
	}

	void preventMegaBunnyJumping(PlayerMove& pmove) {
		const auto maxscaledspeed = bunnyJumpMaxSpeedFactor * maxSpeed;
		if (maxscaledspeed <= 0)
			return;

		const auto currentSpeed = glm::length(pmove.velocity);
		if (currentSpeed <= maxscaledspeed)
			return;

		const auto fraction = (maxscaledspeed / currentSpeed) * 0.65f;
		pmove.velocity *= fraction;
	}

	void fixupGravityVelocity(PlayerMove& pmove) {
		if (pmove.waterjumptime)
			return;

		float ent_gravity = 1.0;
		if (pmove.gravity)
			ent_gravity = pmove.gravity;
		pmove.velocity[2] -= ent_gravity * movevars_gravity * pmove.frametime * 0.5f;

		checkVelocity(pmove);
	}

	void jump(PlayerMove& pmove) {
		if (pmove.dead) {
			pmove.oldbuttons |= IN_JUMP;
			return;
		}

		if (pmove.waterjumptime) {
			pmove.waterjumptime -= pmove.frametime * 1000;
			if (pmove.waterjumptime < 0)
				pmove.waterjumptime = 0;
			return;
		}

		if (pmove.onground == -1) {
			pmove.oldbuttons |= IN_JUMP;
			return;
		}

		if (pmove.oldbuttons & IN_JUMP)
			return;

		pmove.onground = -1;

		preventMegaBunnyJumping(pmove);

		pmove.velocity[2] = std::sqrt(2 * 800 * 45);
		fixupGravityVelocity(pmove);

		pmove.oldbuttons |= IN_JUMP;
	}

	void airAccelerate(PlayerMove& pmove, glm::vec3 wishdir, float wishspeed, float accel) {
		if (pmove.dead || pmove.waterjumptime)
			return;

		auto wishspd = wishspeed;
		if (wishspd > 30)
			wishspd = 30;

		const auto currentspeed = glm::dot(pmove.velocity, wishdir);
		const auto addspeed = wishspd - currentspeed;
		if (addspeed <= 0)
			return;

		float accelspeed = accel * wishspeed * pmove.frametime * pmove.friction;
		if (accelspeed > addspeed)
			accelspeed = addspeed;

		pmove.velocity += accelspeed * wishdir;
	}

	void airMove(const Hull& hull, PlayerMove& pmove) {
		pmove.forward[2] = 0;
		pmove.right[2] = 0;
		pmove.forward = glm::normalize(pmove.forward);
		pmove.right = glm::normalize(pmove.right);

		glm::vec3 wishvel = pmove.forward * pmove.cmd.forwardmove + pmove.right * pmove.cmd.sidemove;
		wishvel[2] = 0;

		auto wishspeed = glm::length(wishvel);
		if (wishspeed > maxSpeed) {
			wishvel *= maxSpeed / wishspeed;
			wishspeed = maxSpeed;
		}

		const auto wishdir = normalizeIfNonZero(wishvel);
		airAccelerate(pmove, wishdir, wishspeed, movevars_airaccelerate);
		flyMove(hull, pmove);
	}

	void catagorizePosition(const Hull& hull, PlayerMove& pmove) {
		//PM_CheckWater();

		if (pmove.velocity[2] > 180) {
			pmove.onground = -1;
			return;
		}

		auto point = pmove.origin;
		point.z -= 2;
		const auto tr = playerTrace(hull, pmove.origin, point);
		// if we hit a steep plane, we are not on ground
		if (tr.plane.normal[2] < 0.7)
			pmove.onground = -1;
		else
			pmove.onground = 0;

		if (pmove.onground != -1) {
			pmove.waterjumptime = 0;
			if (pmove.waterlevel < 2 && !tr.startsolid && !tr.allsolid)
				pmove.origin = tr.endpos;
		}
	}

	void friction(const Hull& hull, PlayerMove& pmove) {
		if (pmove.waterjumptime)
			return;

		const auto vel = pmove.velocity;
		const auto speed = glm::length(vel);
		if (speed < 0.1f)
			return;

		// apply ground friction
		float drop = 0;
		if (pmove.onground != -1) {
			glm::vec3 start, stop;
			start[0] = stop[0] = pmove.origin[0] + vel[0] / speed * 16;
			start[1] = stop[1] = pmove.origin[1] + vel[1] / speed * 16;
			start[2] = pmove.origin[2] + 0; //pmove.player_mins[pmove.usehull][2];
			stop[2] = start[2] - 34;

			const auto trace = playerTrace(hull, start, stop);

			float friction = movevars_friction;
			if (trace.fraction == 1.0)
				friction *= movevars_edgefriction;
			friction *= pmove.friction;

			const auto control = std::max(speed, movevars_stopspeed);
			drop += control * friction * pmove.frametime;
		}

		const auto newspeed = std::max(speed - drop, 0.0f) / speed;
		pmove.velocity = vel * newspeed;
	}

	void checkParamters(PlayerMove& pmove) {
		const auto spd = glm::length(glm::vec3{pmove.cmd.forwardmove, pmove.cmd.sidemove, pmove.cmd.upmove});

		if (spd > maxSpeed) {
			const float ratio = maxSpeed / spd;
			pmove.cmd.forwardmove *= ratio;
			pmove.cmd.sidemove *= ratio;
			pmove.cmd.upmove *= ratio;
		}

		if (pmove.dead) {
			pmove.cmd.forwardmove = 0;
			pmove.cmd.sidemove = 0;
			pmove.cmd.upmove = 0;
			pmove.view_ofs[2] = deadViewHeight;
		} else {
			pmove.angles = pmove.cmd.viewangles;
			pmove.angles.z = 0;
		}

		if (pmove.angles.y > 180.0f)
			pmove.angles.y -= 360.0f;
	}
}

void playerMove(const Hull& hull, PlayerMove& pmove) {
	checkParamters(pmove);
	pmove.frametime = pmove.cmd.frameTime;
	angleVectors(pmove.angles, pmove.forward, pmove.right, pmove.up);
	catagorizePosition(hull, pmove);
	switch (pmove.movetype) {
		case MoveType::noclip:
			noClip(pmove);
			break;
		case MoveType::walk:
			if (!inWater(pmove))
				addCorrectGravity(pmove);
			if (pmove.waterlevel >= 2) {
			} else {
				// not under water
				if (pmove.cmd.buttons & IN_JUMP) {
					//					if (!pLadder) {
					jump(pmove);
					//					}
				} else
					pmove.oldbuttons &= ~IN_JUMP;

				if (pmove.onground != -1) {
					pmove.velocity[2] = 0;
					friction(hull, pmove);
				}
				checkVelocity(pmove);

				if (pmove.onground != -1)
					walkMove(hull, pmove);
				else
					airMove(hull, pmove);
				catagorizePosition(hull, pmove);
				checkVelocity(pmove);

				if (!inWater(pmove))
					fixupGravityVelocity(pmove);

				if (pmove.onground != -1)
					pmove.velocity[2] = 0;
			}
			break;
	}
}
