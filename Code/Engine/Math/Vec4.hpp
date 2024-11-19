//----------------------------------------------------------------------------------------------------
// Vec4.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once

//----------------------------------------------------------------------------------------------------
struct Vec4
{
    // NOTE: Breaking "m_" naming rule for public members
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 0.f;

    static Vec4 ZERO;
    static Vec4 ONE;

    // Construction/Destruction
    ~Vec4() = default;                              // Destructor
    Vec4() = default;                               // Default constructor
    Vec4(Vec4 const& copyFrom) = default;           // Copy constructor
    explicit Vec4(float initialX, float initialY, float initialZ, float initialW); // Custom constructor

    // Accessors (const methods)
    float GetLength() const;                        // Returns the magnitude of the vector
    float GetLengthSquared() const;                 // Returns the squared magnitude
    Vec4  GetNormalized() const;                    // Returns a normalized vector
    Vec4  GetClamped(float maxLength) const;        // Returns a clamped vector with a max length

    // Mutators (non-const methods)
    void  SetLength(float newLength);               // Sets the length of the vector
    void  Normalize();                              // Normalizes the vector
    void  ClampLength(float maxLength);             // Clamps the length of the vector to a max

    // Operators (const)
    bool operator==(Vec4 const& compare) const;     // Equality operator
    bool operator!=(Vec4 const& compare) const;     // Inequality operator
    Vec4 operator+(Vec4 const& vecToAdd) const;     // Addition operator
    Vec4 operator-(Vec4 const& vecToSubtract) const;// Subtraction operator
    Vec4 operator-() const;                         // Unary negation operator
    Vec4 operator*(float uniformScale) const;       // Scalar multiplication
    Vec4 operator/(float inverseScale) const;       // Scalar division

    // Operators (self-mutating / non-const)
    void  operator+=(Vec4 const& vecToAdd);         // Addition-assignment
    void  operator-=(Vec4 const& vecToSubtract);    // Subtraction-assignment
    void  operator*=(float uniformScale);           // Multiplication-assignment
    void  operator/=(float uniformDivisor);         // Division-assignment
    Vec4& operator=(Vec4 const& copyFrom);          // Assignment operator

    // Standalone "friend" functions
    friend Vec4 operator*(float uniformScale, Vec4 const& vecToScale); // Scalar multiplication
};
