#include "3dmath.h"
#include <math.h>

VECTOR3D operator-(VECTOR3D v1, VECTOR3D v2)
{
    VECTOR3D resultVector;
    resultVector.x = v1.x - v2.x;
    resultVector.y = v1.y - v2.y;
    resultVector.z = v1.z - v2.z;
    return resultVector;
}

VECTOR3D operator+(VECTOR3D v1, VECTOR3D v2)
{
    VECTOR3D resultVector;
    resultVector.x = v1.x + v2.x;
    resultVector.y = v1.y + v2.y;
    resultVector.z = v1.z + v2.z;
    return resultVector;
}

VECTOR3D operator*(float f, VECTOR3D v)
{
    v.x *= f;
    v.y *= f;
    v.z *= f;
    return v;
}

VECTOR3D operator*(VECTOR3D v, float f)
{
    v.x *= f;
    v.y *= f;
    v.z *= f;
    return v;
}

VECTOR3D operator/(VECTOR3D v, float f)
{
    v.x /= f;
    v.y /= f;
    v.z /= f;
    return v;
}

float Length(VECTOR3D v)
{
    return sqrt((v.x*v.x) + (v.y*v.y) + (v.z*v.z));
}

VECTOR3D Normalize(VECTOR3D v)
{
    return v / Length(v);
}

float DotProduct(VECTOR3D v1, VECTOR3D v2)
{
    return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

VECTOR3D CrossProduct(VECTOR3D v1, VECTOR3D v2)
{
    VECTOR3D perpendicularVector;
    perpendicularVector.x = (v2.y * v1.z) - (v2.z * v1.y);
    perpendicularVector.y = (v2.z * v1.x) - (v2.x * v1.z);
    perpendicularVector.z = (v2.x * v1.y) - (v2.y * v1.x);
    return perpendicularVector;
}

bool PointInBox(VECTOR3D vPoint, short vMin[3], short vMax[3])
{
    if(((float)vMin[0] <= vPoint.x && vPoint.x <= (float)vMax[0] &&
        (float)vMin[1] <= vPoint.y && vPoint.y <= (float)vMax[1] &&
        (float)vMin[2] <= vPoint.z && vPoint.z <= (float)vMax[2]) ||
       ((float)vMin[0] >= vPoint.x && vPoint.x >= (float)vMax[0] &&
        (float)vMin[1] >= vPoint.y && vPoint.y >= (float)vMax[1] &&
        (float)vMin[2] >= vPoint.z && vPoint.z >= (float)vMax[2]))
        return true;
    else
        return false;
}

bool PointInPlane(VECTOR3D vPoint, VECTOR3D vNormal, float fDist)
{
    if(fabs(DotProduct(vPoint, vNormal) - fDist) < EPSILON)
        return true;
    else
        return false;
}
