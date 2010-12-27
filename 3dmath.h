#ifndef _3DMATH_
#define _3DMATH_

#define EPSILON 0.001f

#define DEGTORAD(x) ((x) * M_PI / 180.0)
#define RADTODEG(x) ((x) * 180.0 / M_PI)

// structure for a float double
typedef struct _VECTOR2D
{
    float x, y;
} VECTOR2D;

// structure for a float triple
typedef struct _VECTOR3D
{
    float x, y, z;
} VECTOR3D;

VECTOR3D operator-(VECTOR3D v1, VECTOR3D v2);                   // Vector substraction
VECTOR3D operator+(VECTOR3D v1, VECTOR3D v2);                   // Vector addition
VECTOR3D operator*(float f, VECTOR3D v);                        // Vector multiplication with a scalar
VECTOR3D operator*(VECTOR3D v, float f);                        // Vector multiplication with a scalar
VECTOR3D operator/(VECTOR3D v, float f);                        // Vector division through a scalar
float Length(VECTOR3D v);                                       // Returns the length of a vector
VECTOR3D Normalize(VECTOR3D v);                                 // Returns the normalized vector
float DotProduct(VECTOR3D v1, VECTOR3D v2);                     // Returns the dot product of the two vectors given
VECTOR3D CrossProduct(VECTOR3D v1, VECTOR3D v2);                // Returns the cross product vector of the two vectors given
bool PointInBox(VECTOR3D vPoint, short vMin[3], short vMax[3]); // Returns a bool spezifing whether or not a point is in the defined box
bool PointInPlane(VECTOR3D vPoint, VECTOR3D vNormal, float fDist);

#endif
