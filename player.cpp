#include "player.h"
#include "main.h"

#include <math.h>

CPlayer::CPlayer()
{
    vPos.x = 0.0f;
    vPos.y = 0.0f;
    vPos.z = 0.0f;

    fZAngle = 0.0f;
    fXAngle = 0.0f;
}

VECTOR3D CPlayer::GetPosition()
{
    return vPos;
}

void CPlayer::SetPosition(float fx, float fy, float fz)
{
    vPos.x = fx;
    vPos.y = fy;
    vPos.z = fz;
}

void CPlayer::SetPosition(VECTOR3D v)
{
    vPos = v;
}

VECTOR2D CPlayer::GetViewAngles()
{
    VECTOR2D vec;
    vec.x = fXAngle;
    vec.y = fZAngle;
    return vec;
}

void CPlayer::SetViewAngles(float fx, float fz)
{
    fXAngle = fx;
    fZAngle = fz;
}

void CPlayer::SetPosition(VECTOR2D v)
{
    fXAngle = v.x;
    fZAngle = v.y;
}

void CPlayer::UpdateFromInput(double dFrameInterval)
{
    // Save old position
    VECTOR3D vOldPos = vPos;

    if(g_bCaptureMouse)
    {
        POINT pt;
        GetCursorPos(&pt);

        // Update rotation based on mouse input
        fZAngle += PLAYER_LOOK_SENS * (float)(int)(g_windowCenter.x - pt.x);

        // Correct z angle to interval [0;360]
        if(fZAngle >= 360.0f)
            fZAngle -= 360.0f;

        if(fZAngle < 0.0f)
            fZAngle += 360.0f;

        // Update up down view
        fXAngle += PLAYER_LOOK_SENS * (float)(int)(g_windowCenter.y - pt.y);

        // Correct x angle to interval [-90;90]
        if (fXAngle < -90.0f)
            fXAngle = -90.0f;

        if (fXAngle > 90.0f)
            fXAngle = 90.0f;

        // Reset cursor
        SetCursorPos(g_windowCenter.x, g_windowCenter.y);
    }

    float fTmpMoveSens = PLAYER_MOVE_SENS * dFrameInterval;

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
        vPos.x += cos(DEGTORAD(fZAngle)) * fTmpMoveSens;
        vPos.y += sin(DEGTORAD(fZAngle)) * fTmpMoveSens;
    }

    if (g_abKeys[(int)'S']) // BACKWARD
    {
        vPos.x -= cos(DEGTORAD(fZAngle)) * fTmpMoveSens;
        vPos.y -= sin(DEGTORAD(fZAngle)) * fTmpMoveSens;
    }

    if (g_abKeys[(int)'A']) // LEFT
    {
        vPos.x += cos(DEGTORAD(fZAngle + 90.0f)) * fTmpMoveSens;
        vPos.y += sin(DEGTORAD(fZAngle + 90.0f)) * fTmpMoveSens;
    }

    if (g_abKeys[(int)'D']) // RIGHT
    {
        vPos.x += cos(DEGTORAD(fZAngle - 90.0f)) * fTmpMoveSens;
        vPos.y += sin(DEGTORAD(fZAngle - 90.0f)) * fTmpMoveSens;
    }

    // Perform collision detection
    VECTOR3D vTrace = g_bsp.TraceBox(vOldPos, vPos, {-16,-16,0}, {16,16,72});
    //VECTOR3D vTrace = g_bsp.TraceRay(vOldPos, vPos);

    if(vTrace != vPos)
		printf("collision\n");

    //printf("old: %.1f/%.1f/%.1f new: %.1f/%.1f/%.1f trace: %.1f/%.1f/%.1f\n", vOldPos.x, vOldPos.y, vOldPos.z, vPos.x, vPos.y, vPos.z, vTrace.x, vTrace.y, vTrace.z);

    vPos = vTrace;
}
