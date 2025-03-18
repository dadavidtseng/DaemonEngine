//-----------------------------------------------------------------------------------------------
// FloatRange.hpp
//-----------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------
#pragma once

//-----------------------------------------------------------------------------------------------
struct FloatRange
{
    float m_min = 0.f;
    float m_max = 0.f;

    static FloatRange ZERO;
    static FloatRange ONE;
    static FloatRange ZERO_TO_ONE;

    // Construction / Destruction
    FloatRange()  = default;
    ~FloatRange() = default;
    explicit FloatRange(float min, float max);

    // Accessors (const methods)
    bool  IsOnRange(float value) const;
    bool  IsOverlappingWith(FloatRange const& other) const;
    void  ClampToRange(float& value) const;
    float GetLength() const;
    float GetMidpoint() const;

    // Mutators (non-const methods)
    void  ExpandToInclude(float value);

    // Operators (const)
    bool        operator==(FloatRange const& compare) const;
    bool        operator!=(FloatRange const& compare) const;

    // Operators (self-mutating / non-const)
    FloatRange& operator=(FloatRange const& copyFrom);
};
