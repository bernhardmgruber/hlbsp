#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "3dmath.h"

#define CAMERA_MOVE_SENS 500 //old: 192
#define CAMERA_LOOK_SENS 0.15

class CCamera
{
    public:
        CCamera(); // ctor

        VECTOR3D GetPosition();                         // Returns the current position
        void SetPosition(float fx, float fy, float fz); // Sets the current position

        VECTOR2D GetViewAngles();               // Returns a vector containing the current view angles
        void SetViewAngles(float fx, float fz); // Sets the current view angles

        float GetMoveSens();               // Returns the current movement sensitivity
        void SetMoveSens(float fNewMoveSens); // Sets the current movement sensitivity

        void CaptureMouse(bool bCapture); // Sets whether or not the cursor should be captured, not be shown and always set to the middle of the scene.

        void UpdateView(double dFrameInterval); // Updates the position of the coord system according to user input and passed time since the last frame
        void Look();
    private:
        VECTOR3D vPos; // Current position

        float fXAngle; // Angle of up down rotation
        float fZAngle; // Angle of side to side rotation

        float fMoveSens; // Movement sensitivity
        float fLookSens; // Look sensitivity

        unsigned int nMouseOriginX; // Pixel coords of the origin of the mouse
        unsigned int nMouseOriginY; // Pixel coords of the origin of the mouse

        bool bCaptureMouse; // Boolen whether or not we capture the mouse
};

#endif // CAMERA_H_INCLUDED
