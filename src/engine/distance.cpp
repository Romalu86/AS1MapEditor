#include "mapedit/runtime.hpp"

float __cdecl NearDistance(float d1,float d2)
{
    d1=fabsf(d1);
    d2=fabsf(d2);
    if (d1>=d2)
        return d1+d2/2.0f;
    return d2+d1/2.0f;
}

int __cdecl NearDistance(int d1,int d2)
{
    d1=abs(d1);
    d2=abs(d2);
    if (d1>d2)
        return d1+d2/2;
    return d2+d1/2;
}

int Max(int arg1,int arg2)
{
    return arg1>arg2 ? arg1 : arg2;
}

int Min(int arg1,int arg2)
{
    return arg1<arg2 ? arg1 : arg2;
}
