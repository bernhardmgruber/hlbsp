#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "3dmath.h"

#define PLAYER_MOVE_SENS 500 //192
#define PLAYER_LOOK_SENS 0.15

class CPlayer
{
public:
    CPlayer();

    VECTOR3D GetPosition();                         // Returns the current position
    void SetPosition(float fx, float fy, float fz); // Sets the current position
    void SetPosition(VECTOR3D v); // Sets the current position

    VECTOR2D GetViewAngles();               // Returns a vector containing the current view angles
    void SetViewAngles(float fx, float fz); // Sets the current view angles
    void SetPosition(VECTOR2D v); // Sets the current view angles

    void UpdateFromInput(double dFrameInterval); // Updates the position of the coord system according to user input and passed time since the last frame

private:
    VECTOR3D vPos; // Current position

    float fXAngle; // Angle of up down rotation
    float fZAngle; // Angle of side to side rotation
};

#endif // PLAYER_H_INCLUDED
