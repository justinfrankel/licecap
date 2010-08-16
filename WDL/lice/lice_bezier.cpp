#include "lice.h"
#include <math.h>

// Returns Bezier x, y for a given t in [0,1].
void LICE_Bezier(float ctrl_x1, float ctrl_x2, float ctrl_x3, 
    float ctrl_y1, float ctrl_y2, float ctrl_y3, float t, float* pX, float* pY)
{
    float a = (1.0f - t) * (1.0f - t);
    float b = 2.0f * (1.0f - t) * t;
    float c = t * t;
    *pX = a * ctrl_x1 + b * ctrl_x2 + c * ctrl_x3;
    *pY = a * ctrl_y1 + b * ctrl_y2 + c * ctrl_y3;
}

// Returns Bezier y for a given x in [x1, x3] (for rasterizing).
float LICE_Bezier_GetY(float ctrl_x1, float ctrl_x2, float ctrl_x3, float ctrl_y1, float ctrl_y2, float ctrl_y3, float x)
{
    float a = ctrl_x1 - 2.0f * ctrl_x2 + ctrl_x3;
    float b = 2.0f * (ctrl_x2 - ctrl_x1);
    float c = ctrl_x1 - x;
    if (a == 0.0f && b == 0.0f) {
        return ctrl_y2;
    }

    float t;
    if (a == 0.0f) {
        t = -c / b;
    }
    else {
        t = (-b + sqrt(b * b - 4.0f * a * c)) / (2.0f * a);
    }

    a = (1.0f - t) * (1.0f - t);
    b = 2.0f * (1.0f - t) * t;
    c = t * t;
    return a * ctrl_y1 + b * ctrl_y2 + c * ctrl_y3;
}