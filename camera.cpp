#include "camera.h"

#include <windows.h>
#include <gl/gl.h>
#include <math.h>
#include "3dmath.h"

#define DEGTORAD 0.017460317460317460317460317460317

extern bool	g_abKeys[256];

CCamera::CCamera()
{
    fZAngle = 0.0f;
    fXAngle = 0.0f;

    fMoveSens = CAMERA_MOVE_SENS;
    fLookSens = CAMERA_LOOK_SENS;

    bCaptureMouse = false;
}

VECTOR3D CCamera::GetPosition()
{
    return vPos;
}

void CCamera::SetPosition(float fx, float fy, float fz)
{
    vPos.x = fx;
    vPos.y = fy;
    vPos.z = fz;
}

VECTOR2D CCamera::GetViewAngles()
{
    VECTOR2D vec;
    vec.x = fXAngle;
    vec.y = fZAngle;
    return vec;
}

void CCamera::SetViewAngles(float fx, float fz)
{
    fXAngle = fx;
    fZAngle = fz;
}

float CCamera::GetMoveSens()
{
    return fMoveSens;
}

void CCamera::SetMoveSens(float fNewMoveSens)
{
    fMoveSens = fNewMoveSens;
}

void CCamera::CaptureMouse(bool bCapture)
{
    bCaptureMouse = bCapture;

    if(bCapture)
    {
        POINT pt;
        GetCursorPos(&pt);
        nMouseOriginX = pt.x;
        nMouseOriginY = pt.y;
    }
}

void CCamera::UpdateView(double dFrameInterval)
{
    if(bCaptureMouse)
    {
        POINT pt;
        GetCursorPos(&pt);

        // Update rotation based on mouse input
        fZAngle += fLookSens * (float)(int)(nMouseOriginX - pt.x);

        // Correct z angle to interval [0;360]
        if(fZAngle >= 360.0f)
            fZAngle -= 360.0f;

        if(fZAngle < 0.0f)
            fZAngle += 360.0f;

        // Update up down view
        fXAngle += fLookSens * (float)(int)(nMouseOriginY - pt.y);

        // Correct x angle to interval [-90;90]
        if (fXAngle < -90.0f)
            fXAngle = -90.0f;

        if (fXAngle > 90.0f)
            fXAngle = 90.0f;

        // Reset cursor
        SetCursorPos(nMouseOriginX, nMouseOriginY);
    }

    float fTmpMoveSens = fMoveSens * dFrameInterval;

    if (g_abKeys[VK_SPACE]) // UP
    {
        vPos.z += fTmpMoveSens;
    }

    if (g_abKeys[VK_CONTROL]) // DOWN
    {
        vPos.z -= fTmpMoveSens;
    }

    // TODO: If strafing and moving reduce speed to keep total move per frame constant
    if (g_abKeys[(int)'W']) // FORWARD
    {
        vPos.x += cos(fZAngle * DEGTORAD) * fTmpMoveSens;
        vPos.y += sin(fZAngle * DEGTORAD) * fTmpMoveSens;
    }

    if (g_abKeys[(int)'S']) // BACKWARD
    {
        vPos.x -= cos(fZAngle * DEGTORAD) * fTmpMoveSens;
        vPos.y -= sin(fZAngle * DEGTORAD) * fTmpMoveSens;
    }

    if (g_abKeys[(int)'A']) // LEFT
    {
        vPos.x += cos((fZAngle + 90.0f) * DEGTORAD) * fTmpMoveSens;
        vPos.y += sin((fZAngle + 90.0f) * DEGTORAD) * fTmpMoveSens;
    }

    if (g_abKeys[(int)'D']) // RIGHT
    {
        vPos.x += cos((fZAngle - 90.0f) * DEGTORAD) * fTmpMoveSens;
        vPos.y += sin((fZAngle - 90.0f) * DEGTORAD) * fTmpMoveSens;
    }
}

void CCamera::Look()
{
    // In BSP v30 the z axis points up and we start looking parallel to x axis.
    // Look Up/Down
    glRotatef(-fXAngle - 90.0f, 1.0f, 0.0f, 0.0f);
    // Look Left/Right
    glRotatef(-fZAngle + 90.0f, 0.0f, 0.0f, 1.0f);
    // Move
    glTranslatef(-vPos.x, -vPos.y, -vPos.z);
}
