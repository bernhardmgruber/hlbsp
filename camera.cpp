#include "camera.h"
#include "main.h"
#include <math.h>

CCamera::CCamera()
{
    mode = CAMERA_MODE_FLY_THROUGH;
    pPlayer = NULL;

    vPos.x = 0.0f;
    vPos.y = 0.0f;
    vPos.z = 0.0f;

    yaw = 0.0f;
    pitch = 0.0f;

    fMoveSens = CAMERA_MOVE_SENS;
    fLookSens = CAMERA_LOOK_SENS;
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

void CCamera::SetPosition(VECTOR3D v)
{
    vPos = v;
}

VECTOR2D CCamera::GetViewAngles()
{
    VECTOR2D vec;
    vec.x = pitch;
    vec.y = yaw;
    return vec;
}

VECTOR3D CCamera::GetViewVector()
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

void CCamera::SetViewAngles(float fx, float fz)
{
    pitch = fx;
    yaw = fz;
}

void CCamera::SetPosition(VECTOR2D v)
{
    pitch = v.x;
    yaw = v.y;
}

float CCamera::GetMoveSens()
{
    return fMoveSens;
}

void CCamera::SetMoveSens(float fNewMoveSens)
{
    fMoveSens = fNewMoveSens;
}

void CCamera::Update(double dFrameInterval)
{
    if(mode == CAMERA_MODE_PLAYER_EGO)
    {
        // if a player is bound, get his position and view angles
        vPos = pPlayer->GetPosition();
        VECTOR2D view = pPlayer->GetViewAngles();
        pitch = view.x;
        yaw = view.y;
    }
    else
    {
        // else update camera object
        if(g_bCaptureMouse)
        {
            //POINT pt;
            //GetCursorPos(&pt);
            int x, y;
            SDL_GetMouseState(&x, &y);

            // Update rotation based on mouse input
            yaw += fLookSens * (float)(int)(nMouseOriginX - x);

            // Correct z angle to interval [0;360]
            if(yaw >= 360.0f)
                yaw -= 360.0f;

            if(yaw < 0.0f)
                yaw += 360.0f;

            // Update up down view
            pitch += fLookSens * (float)(int)(nMouseOriginY - y);

            // Correct x angle to interval [-90;90]
            if (pitch < -90.0f)
                pitch = -90.0f;

            if (pitch > 90.0f)
                pitch = 90.0f;

            // Reset cursor
            SDL_WarpMouse(nMouseOriginX, nMouseOriginY);
        }

        float fTmpMoveSens = fMoveSens * dFrameInterval;

        if (g_abKeys[SDLK_SPACE]) // UP
        {
            vPos.z += fTmpMoveSens;
        }

        if (g_abKeys[SDLK_LCTRL]) // DOWN
        {
            vPos.z -= fTmpMoveSens;
        }

        // TODO: If strafing and moving reduce speed to keep total move per frame constant
        if (g_abKeys[SDLK_w]) // FORWARD
        {
            vPos.x += cos(DEGTORAD(yaw)) * fTmpMoveSens;
            vPos.y += sin(DEGTORAD(yaw)) * fTmpMoveSens;
        }

        if (g_abKeys[SDLK_s]) // BACKWARD
        {
            vPos.x -= cos(DEGTORAD(yaw)) * fTmpMoveSens;
            vPos.y -= sin(DEGTORAD(yaw)) * fTmpMoveSens;
        }

        if (g_abKeys[SDLK_a]) // LEFT
        {
            vPos.x += cos(DEGTORAD(yaw + 90.0f)) * fTmpMoveSens;
            vPos.y += sin(DEGTORAD(yaw + 90.0f)) * fTmpMoveSens;
        }

        if (g_abKeys[SDLK_d]) // RIGHT
        {
            vPos.x += cos(DEGTORAD(yaw - 90.0f)) * fTmpMoveSens;
            vPos.y += sin(DEGTORAD(yaw - 90.0f)) * fTmpMoveSens;
        }
    }
}

void CCamera::Look()
{
    // In BSP v30 the z axis points up and we start looking parallel to x axis.
    // Look Up/Down
    glRotatef(-pitch - 90.0f, 1.0f, 0.0f, 0.0f);
    // Look Left/Right
    glRotatef(-yaw + 90.0f, 0.0f, 0.0f, 1.0f);
    // Move
    glTranslatef(-vPos.x, -vPos.y, -vPos.z);
}

void CCamera::SetMode(int iMode)
{
    mode = iMode;
}

void CCamera::BindPlayer(CPlayer* pPl)
{
    pPlayer = pPl;
    SetMode(CAMERA_MODE_PLAYER_EGO);
}
