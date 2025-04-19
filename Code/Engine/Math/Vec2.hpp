//----------------------------------------------------------------------------------------------------
// Vec2.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

struct IntVec2;

//----------------------------------------------------------------------------------------------------
struct Vec2
{
    // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
    float x = 0.f;
    float y = 0.f;

    static Vec2 ZERO;
    static Vec2 HALF;
    static Vec2 ONE;

    // Construction / Destruction
    Vec2()                     = default;               // default constructor (do nothing)
    ~Vec2()                    = default;               // destructor (do nothing)
    Vec2(Vec2 const& copyFrom) = default;               // copy constructor (from another vec2)
    explicit Vec2(float initialX, float initialY);      // explicit constructor (from x, y)
    explicit Vec2(int initialX, int initialY);
    explicit Vec2(IntVec2 const& intVec2);

    // Static methods (e.g. creation functions)
    static Vec2 MakeFromPolarRadians(float orientationRadians, float length = 1.f);
    static Vec2 MakeFromPolarDegrees(float orientationDegrees, float length = 1.f);

    // Accessors (const methods)
    float GetLength() const;
    float GetLengthSquared() const;
    float GetOrientationRadians() const;
    float GetOrientationDegrees() const;
    Vec2  GetRotated90Degrees() const;
    Vec2  GetRotatedMinus90Degrees() const;
    Vec2  GetRotatedRadians(float deltaRadians) const;
    Vec2  GetRotatedDegrees(float deltaDegrees) const;
    Vec2  GetClamped(float maxLength) const;
    Vec2  GetNormalized() const;
    Vec2  GetReflected(Vec2 const& normalOfSurfaceToReflectOffOf) const;

    // Mutators (non-const methods)
    void  SetOrientationRadians(float newOrientationRadians);
    void  SetOrientationDegrees(float newOrientationDegrees);
    void  SetPolarRadians(float newOrientationRadians, float newLength);
    void  SetPolarDegrees(float newOrientationDegrees, float newLength);
    void  Rotate90Degrees();
    void  RotateMinus90Degrees();
    void  RotateRadians(float deltaRadians);
    void  RotateDegrees(float deltaDegrees);
    void  SetLength(float newLength);
    void  ClampLength(float maxLength);
    void  Normalize();
    float NormalizeAndGetPreviousLength();
    void  Reflect(Vec2 const& normalOfSurfaceToReflectOffOf);
    void  SetFromText(char const* text);		        // Parses “6,4” or “ -.3 , 0.05 ” to (x,y)

    // Operators (const)
    bool operator==(Vec2 const& compare) const;         // vec2 == vec2
    bool operator!=(Vec2 const& compare) const;         // vec2 != vec2
    Vec2 operator+(Vec2 const& vecToAdd) const;         // vec2 + vec2
    Vec2 operator-(Vec2 const& vecToSubtract) const;    // vec2 - vec2
    Vec2 operator-() const;                             // -vec2, i.e. "unary negation"
    Vec2 operator*(float uniformScale) const;           // vec2 * float
    Vec2 operator/(float inverseScale) const;           // vec2 / float

    // Operators (self-mutating / non-const)
    void  operator+=(Vec2 const& vecToAdd);         // vec2 += vec2
    void  operator-=(Vec2 const& vecToSubtract);    // vec2 -= vec2
    void  operator*=(float uniformScale);           // vec2 *= float
    void  operator/=(float uniformDivisor);         // vec2 /= float
    Vec2& operator=(Vec2 const& copyFrom);          // vec2 = vec2
    Vec2  operator*(Vec2 const& vecToMultiply) const;

    // Standalone "friend" functions that are conceptually, but not actually, part of Vec2::
    friend Vec2 operator*(float uniformScale, Vec2 const& vecToScale); // float * vec2
};

Vec2 Interpolate(Vec2 const& start, Vec2 const& end, float t);
