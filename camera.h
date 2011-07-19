#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "mathlib.h"
#include "player.h"

#define CAMERA_MOVE_SENS 500 //old: 192
#define CAMERA_LOOK_SENS 0.15

#define CAMERA_MODE_FLY_THROUGH 0
#define CAMERA_MODE_PLAYER_EGO  1

class CCamera
{
public:
    CCamera(); // ctor

    VECTOR3D GetPosition();                         // Returns the current position
    void SetPosition(float fx, float fy, float fz); // Sets the current position
    void SetPosition(VECTOR3D v); // Sets the current position

    VECTOR2D GetViewAngles();               // Returns a vector containing the current view angles
    void SetViewAngles(float fx, float fz); // Sets the current view angles
    void SetPosition(VECTOR2D v); // Sets the current view angles

    float GetMoveSens();               // Returns the current movement sensitivity
    void SetMoveSens(float fNewMoveSens); // Sets the current movement sensitivity

    void Update(double dFrameInterval); // Updates the position of the coord system according to user input and passed time since the last frame
    void Look();

    void SetMode(int iMode);
    void BindPlayer(CPlayer* pPl);
private:
    int mode;
    CPlayer* pPlayer;

    VECTOR3D vPos; // Current position

    float fXAngle; // Angle of up down rotation
    float fZAngle; // Angle of side to side rotation

    float fMoveSens; // Movement sensitivity
    float fLookSens; // Look sensitivity

    unsigned int nMouseOriginX; // Pixel coords of the origin of the mouse
    unsigned int nMouseOriginY; // Pixel coords of the origin of the mouse
};

#endif // CAMERA_H_INCLUDED
