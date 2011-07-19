#include "player.h"
#include "main.h"

#include <math.h>

CPlayer::CPlayer()
{
    vPos.x = 0.0f;
    vPos.y = 0.0f;
    vPos.z = 0.0f;

    vVel.x = 0.0f;
    vVel.y = 0.0f;
    vVel.z = 0.0f;

    yaw = 0.0f;
    pitch = 0.0f;

    crouching = false;
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
    vec.x = pitch;
    vec.y = yaw;
    return vec;
}

VECTOR3D CPlayer::GetViewVector()
{
    VECTOR3D v;
    v.x = 1;
    v.y = 0;
    v.z = 0;

    // rotate pitch along -y
    v = RotateY(-pitch, v);

    // rotate yaw along z
    v = RotateZ(yaw, v);

    return v;
}

void CPlayer::SetViewAngles(float fx, float fz)
{
    pitch = fx;
    yaw = fz;
}

void CPlayer::SetViewAngles(VECTOR2D v)
{
    pitch = v.x;
    yaw = v.y;
}

void CPlayer::Update(double dFrameInterval)
{
    // Save old position
    VECTOR3D vNewPos = vPos;

    //
    // Update from user input
    //

    if(g_bCaptureMouse)
    {
        int x, y;
        SDL_GetMouseState(&x, &y);

        // Update rotation based on mouse input
        yaw += PLAYER_LOOK_SENS * (float)(int)(g_windowCenter.x - x);

        // Correct z angle to interval [0;360]
        if(yaw >= 360.0f)
            yaw -= 360.0f;

        if(yaw < 0.0f)
            yaw += 360.0f;

        // Update up down view
        pitch += PLAYER_LOOK_SENS * (float)(int)(g_windowCenter.y - y);

        // Correct x angle to interval [-90;90]
        if (pitch < -90.0f)
            pitch = -90.0f;

        if (pitch > 90.0f)
            pitch = 90.0f;

        // Reset cursor
        SDL_WarpMouse(g_windowCenter.x, g_windowCenter.y);
    }

    float fTmpMoveSens = PLAYER_MOVE_SENS * dFrameInterval;

    if (g_abKeys[SDLK_SPACE]) // UP
    {
        vNewPos.z += fTmpMoveSens;
    }

    if (g_abKeys[SDLK_LCTRL]) // DOWN
    {
        vNewPos.z -= fTmpMoveSens;
    }

    // TODO: If strafing and moving reduce speed to keep total move per frame constant
    if (g_abKeys[SDLK_w]) // FORWARD
    {
        vNewPos.x += cos(DEGTORAD(yaw)) * fTmpMoveSens;
        vNewPos.y += sin(DEGTORAD(yaw)) * fTmpMoveSens;
    }

    if (g_abKeys[SDLK_s]) // BACKWARD
    {
        vNewPos.x -= cos(DEGTORAD(yaw)) * fTmpMoveSens;
        vNewPos.y -= sin(DEGTORAD(yaw)) * fTmpMoveSens;
    }

    if (g_abKeys[SDLK_a]) // LEFT
    {
        vNewPos.x += cos(DEGTORAD(yaw + 90.0f)) * fTmpMoveSens;
        vNewPos.y += sin(DEGTORAD(yaw + 90.0f)) * fTmpMoveSens;
    }

    if (g_abKeys[SDLK_d]) // RIGHT
    {
        vNewPos.x += cos(DEGTORAD(yaw - 90.0f)) * fTmpMoveSens;
        vNewPos.y += sin(DEGTORAD(yaw - 90.0f)) * fTmpMoveSens;
    }

    //
    // Physics
    //

    // gravity
    vNewPos.z -= 3;

    //
    // Perform collision detection
    //

    VECTOR3D vTrace = g_bsp.Move(vPos, vNewPos, 0);

    //printf("%.1f/%.1f/%.1f\n", vTrace.x, vTrace.y, vTrace.z);
    //printf("old: %.1f/%.1f/%.1f new: %.1f/%.1f/%.1f trace: %.1f/%.1f/%.1f\n", vOldPos.x, vOldPos.y, vOldPos.z, vPos.x, vPos.y, vPos.z, vTrace.x, vTrace.y, vTrace.z);

    vPos = vTrace;
}
