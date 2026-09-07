#pragma once
// ANGLE owner. Included in ABI order by mapedit/runtime.hpp.
class ANGLE {
public:
    uint8_t value;

    ANGLE();
    ANGLE(uint8_t direction);
    ANGLE(float x,float y);
    ANGLE(float x,float y,int* radius);
    ANGLE(const ANGLE* other);

    int operator==(const ANGLE* other);
    int operator!=(const ANGLE* other);
    int operator<(const ANGLE* other);
    int operator<=(const ANGLE* other);
    int operator>(const ANGLE* other);
    ANGLE Difference(const ANGLE* other);
    ANGLE GetInversed();
    void Inverse();
    const ANGLE* operator=(const ANGLE* other);
    const ANGLE operator+(const ANGLE* other);
    const ANGLE operator-(const ANGLE* other);
    int Int();
    float Sin();
    float Cos();
    float SinY();
    float CosY();
    float RotateX(float x,float y);
    float RotateY(float x,float y);
    void Write(STREAM* res);
    void Read(STREAM* res);
    static void Init();
};
static_assert(sizeof(ANGLE)==1, "MapEdit ANGLE size");

ANGLE Decart2Polar(int x,int y,int* radius);
ANGLE Decart2Polar(float x,float y);

