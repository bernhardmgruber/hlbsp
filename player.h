#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "mathlib.h"

/// Movement sensibility
#define PLAYER_MOVE_SENS 500 //192

/// Mouse sensibility
#define PLAYER_LOOK_SENS 0.15

/// The maximum height a player can pass without jumping
#define PLAYER_MAX_STEP_HEIGHT 10.0f

/**
 *  @brief Describes a player in the game
 *  Contains information about the player's current position, view direction, movement, health, etc.
 */
class CPlayer
{
public:
    /// Constructor
    CPlayer();

    /// Returns the current position
    VECTOR3D GetPosition();

    /// Sets the current position
    void SetPosition(float fx, float fy, float fz);

    /// Sets the current position
    void SetPosition(VECTOR3D v);

    /// Returns a vector containing the current view angles
    VECTOR2D GetViewAngles();

    /// Returns a vector pointing representing the current view direction
    VECTOR3D GetViewVector();

    /// Sets the current view angles
    void SetViewAngles(float fx, float fz);

    /// Sets the current view angles
    void SetViewAngles(VECTOR2D v);

    /// Updates the position according to user input and passed time. Handels physics (e.g. gravity) and collision detection.
    void Update(double dFrameInterval);

private:
    /// Current position
    VECTOR3D vPos;

    /// Velocity of player
    VECTOR3D vVel;

    /// Angle of up down rotation
    float fXAngle;

    /// Angle of side to side rotation
    float fZAngle;
};

#endif // PLAYER_H_INCLUDED
