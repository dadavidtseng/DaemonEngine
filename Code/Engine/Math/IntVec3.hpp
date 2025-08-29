//----------------------------------------------------------------------------------------------------
// IntVec3.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

struct Vec3;
struct IntVec2;

//----------------------------------------------------------------------------------------------------
struct IntVec3
{
    // NOTE: this is one of the few cases where we break both the "m_" naming rule AND the avoid-public-members rule
    int            x = 0;
    int            y = 0;
    int            z = 0;
    static IntVec3 ZERO;
    static IntVec3 ONE;
    static IntVec3 NEGATIVE_ONE;

    // Construction/Destruction
    IntVec3()                        = default;    // default constructor (do nothing)
    ~IntVec3()                       = default;    // destructor (do nothing)
    IntVec3(IntVec3 const& copyFrom) = default;    // copy constructor (from another IntVec3)
    explicit IntVec3(int initialX, int initialY, int initialZ);   // explicit constructor (from x, y, z)
    explicit IntVec3(float initialX, float initialY, float initialZ);
    explicit IntVec3(Vec3 const& vec3);

    // Accessors (const methods)
    float   GetLength() const;
    int     GetLengthSquared() const;
    int     GetTaxicabLength() const;
    IntVec2 GetXY() const;

    // Mutators (non-const methods)
    void SetFromText(char const* text);

    // Operators (const)
    bool operator==(IntVec3 const& compare) const;         // IntVec3 == IntVec3
    bool operator!=(IntVec3 const& compare) const;         // IntVec3 != IntVec3
    bool operator<(IntVec3 const& compare) const;          // IntVec3 < IntVec3 (for ordering/containers)
    IntVec3 const operator+(IntVec3 const& vecToAdd) const;       // IntVec3 + IntVec3
    IntVec3 const operator-(IntVec3 const& vecToSubtract) const;  // IntVec3 - IntVec3
    IntVec3 operator-() const;                             // -IntVec3, i.e. "unary negation"
    IntVec3 const operator*(int uniformScale) const;              // IntVec3 * int

    // Operators (self-mutating / non-const)
    void operator+=(IntVec3 const& vecToAdd);         // IntVec3 += IntVec3
    void operator-=(IntVec3 const& vecToSubtract);    // IntVec3 -= IntVec3
    void operator*=(int uniformScale);                // IntVec3 *= int
    IntVec3& operator=(IntVec3 const& copyFrom);      // IntVec3 = IntVec3

    // Standalone "friend" functions that are conceptually, but not actually, part of IntVec3::
    friend IntVec3 const operator*(int uniformScale, IntVec3 const& vecToScale); // int * IntVec3
};

IntVec3 Interpolate(IntVec3 const& start, IntVec3 const& end, float t);