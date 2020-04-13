#include "move.h"

#include "Bsp.h"
#include <cassert>
#include <iostream>

// the following code is largly inspired by WinQuake's sv_move.c

namespace {
	constexpr auto DIST_EPSILON = 0.03125f;

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
			frac = (t1 + DIST_EPSILON) / (t1 - t2);
		else
			frac = (t1 - DIST_EPSILON) / (t1 - t2);
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
}

auto move(const Hull& hull, glm::vec3 start, glm::vec3 end) -> glm::vec3 {
	if (start == end)
		return end;

	Trace trace{};
	trace.fraction = 1;
	trace.allsolid = true;
	trace.endpos = end;

	recursiveHullCheck(hull, hull.firstclipnode, 0, 1, start, end, trace);

	return trace.endpos;
}
