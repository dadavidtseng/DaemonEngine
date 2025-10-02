[Root Directory](../../../CLAUDE.md) > [Code](../../) > [Engine](../) > **Math**

# Math Module Documentation

## Module Responsibilities

The Math module provides a comprehensive mathematical foundation for 3D graphics, game physics, and geometric operations. It includes vector mathematics, matrix operations, geometric primitives, utility functions, and optimized algorithms essential for game engine operations.

## Entry and Startup

### Primary Headers
- `MathUtils.hpp` - Core mathematical utilities and constants
- `Vec2.hpp`, `Vec3.hpp`, `Vec4.hpp` - Vector mathematics
- `Mat44.hpp` - 4x4 matrix operations for transformations
- `AABB2.hpp`, `AABB3.hpp` - Axis-aligned bounding boxes

### Usage Pattern
```cpp
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/MathUtils.hpp"

// Vector operations
Vec3 position(10.f, 20.f, 30.f);
Vec3 direction = Vec3::FORWARD;
Vec3 result = position + direction * 5.0f;

// Matrix transformations
Mat44 transform = Mat44::CreateTranslation3D(position);
transform.Append(Mat44::CreateRotationDegreesAboutZ(45.0f));
```

## External Interfaces

### Vector Mathematics
```cpp
// 2D Vector operations
class Vec2 {
    float x, y;
    static const Vec2 ZERO, ONE, UNIT_NORTH, UNIT_SOUTH, UNIT_EAST, UNIT_WEST;
    
    float GetLength() const;
    Vec2  GetNormalized() const;
    float GetAngleDegrees() const;
    Vec2  GetRotated90Degrees() const;
};

// 3D Vector operations  
class Vec3 {
    float x, y, z;
    static const Vec3 ZERO, ONE, FORWARD, UP, RIGHT;
    
    float GetLength() const;
    Vec3  GetNormalized() const;
    Vec3  CrossProduct(Vec3 const& other) const;
    float DotProduct(Vec3 const& other) const;
};
```

### Matrix Operations
```cpp
class Mat44 {
    float m_values[16]; // Column-major 4x4 matrix
    
    static Mat44 CreateTranslation3D(Vec3 const& translationXYZ);
    static Mat44 CreateRotationDegreesAboutX(float rotationDegreesAboutX);
    static Mat44 CreateScale3D(Vec3 const& nonUniformScale3D);
    static Mat44 CreatePerspectiveProjection(float fovYDegrees, float aspect, 
                                           float zNear, float zFar);
    static Mat44 CreateOrthographicProjection(float left, float right, 
                                            float bottom, float top, 
                                            float zNear, float zFar);
    
    void Append(Mat44 const& matrixToAppend);
    Vec3 TransformVectorQuantity3D(Vec3 const& vectorQuantity) const;
    Vec3 TransformPosition3D(Vec3 const& position) const;
};
```

### Geometric Primitives
```cpp
// 2D Shapes
class AABB2 {
    Vec2 mins, maxs;
    bool IsPointInside(Vec2 const& point) const;
    Vec2 GetCenter() const;
    Vec2 GetDimensions() const;
};

class OBB2 {
    Vec2 center;
    Vec2 halfDimensions;
    float orientationDegrees;
};

class Disc2 {
    Vec2 center;
    float radius;
};

// 3D Shapes
class AABB3 {
    Vec3 mins, maxs;
    bool IsPointInside(Vec3 const& point) const;
    Vec3 GetCenter() const;
    Vec3 GetDimensions() const;
};

class Sphere3 {
    Vec3 center;
    float radius;
};
```

## Key Dependencies and Configuration

### Internal Dependencies
- Core module for basic types and utilities
- Minimal external dependencies for maximum performance

### Mathematical Constants
```cpp
// Defined in MathUtils.hpp
constexpr float PI = 3.14159265f;
constexpr float TWO_PI = 6.28318531f;
constexpr float HALF_PI = 1.57079633f;
constexpr float SQRT_2 = 1.41421356f;
constexpr float SQRT_3 = 1.73205081f;
```

### Utility Functions
```cpp
// Angle conversions
float ConvertDegreesToRadians(float degrees);
float ConvertRadiansToDegrees(float radians);

// Interpolation
float Interpolate(float start, float end, float fractionTowardEnd);
Vec2 Interpolate(Vec2 const& start, Vec2 const& end, float fractionTowardEnd);
Vec3 Interpolate(Vec3 const& start, Vec3 const& end, float fractionTowardEnd);

// Clamping and ranges
float Clamp(float value, float minValue, float maxValue);
int   ClampInt(int value, int minValue, int maxValue);
```

## Data Models

### Vector Types
```cpp
// Integer vectors for discrete coordinates
class IntVec2 {
    int x, y;
    static const IntVec2 ZERO, ONE, UNIT_NORTH, UNIT_SOUTH, UNIT_EAST, UNIT_WEST;
};

class IntVec3 {
    int x, y, z;
    static const IntVec3 ZERO, ONE, UNIT_FORWARD, UNIT_UP, UNIT_RIGHT;
};
```

### Range Types
```cpp
class FloatRange {
    float minimum, maximum;
    float GetRandomInRange(RandomNumberGenerator& rng) const;
    bool IsInRange(float value) const;
};

class IntRange {
    int minimum, maximum;
    int GetRandomInRange(RandomNumberGenerator& rng) const;
    bool IsInRange(int value) const;
};
```

### Geometric Analysis
```cpp
// Raycast utilities
class RaycastUtils {
    static RaycastResult3D RaycastVsAABB3(Vec3 rayStart, Vec3 rayDirection, 
                                         AABB3 const& bounds);
    static RaycastResult3D RaycastVsSphere3(Vec3 rayStart, Vec3 rayDirection, 
                                          Sphere3 const& sphere);
};

// Euler angle representation
class EulerAngles {
    float m_yawDegrees;
    float m_pitchDegrees;  
    float m_rollDegrees;
    
    Mat44 GetAsMatrix_XFwd_YLeft_ZUp() const;
    Vec3 GetAsVector_XFwd_YLeft_ZUp() const;
};
```

## Testing and Quality

### Mathematical Accuracy
- All operations maintain floating-point precision
- Matrix operations use column-major layout for DirectX compatibility
- Vector normalization includes zero-length protection
- Trigonometric functions use optimized implementations

### Current Testing Approach
- Visual verification through renderer debug output
- Unit vector validation in physics simulations
- Matrix transformation verification in camera systems
- Geometric intersection testing through collision detection

### Performance Considerations
- Inline functions for frequently used operations
- SIMD-friendly data layout where possible
- Optimized trigonometric lookup tables for common angles
- Cache-friendly memory access patterns

### Recommended Testing Additions
- Comprehensive unit tests for all mathematical operations
- Numerical precision validation tests
- Performance benchmarks for critical path operations
- Cross-platform floating-point consistency testing

## FAQ

### Q: What coordinate system does the math library use?
A: Right-handed coordinate system with Y-up for 3D world space, compatible with DirectX conventions.

### Q: Are matrix operations optimized for performance?
A: Yes, matrices use column-major layout, inline functions, and avoid unnecessary allocations in critical paths.

### Q: How do I handle numerical precision issues?
A: Use appropriate epsilon values for floating-point comparisons, available in MathUtils.hpp.

### Q: What's the difference between Vec3::GetNormalized() and normalizing in place?
A: GetNormalized() returns a new normalized vector, preserving the original. Use it to avoid modifying source data.

### Q: How do I convert between different angle representations?
A: Use EulerAngles class for yaw/pitch/roll, or direct trigonometric functions in MathUtils.hpp.

## Related Files

### Vector Mathematics
- `Vec2.cpp` - 2D vector implementation
- `Vec3.cpp` - 3D vector implementation  
- `Vec4.cpp` - 4D vector implementation
- `IntVec2.cpp` - Integer 2D vectors
- `IntVec3.cpp` - Integer 3D vectors

### Matrix and Transformations
- `Mat44.cpp` - 4x4 matrix operations
- `EulerAngles.cpp` - Euler angle conversions

### Geometric Primitives
- `AABB2.cpp` - 2D axis-aligned bounding boxes
- `AABB3.cpp` - 3D axis-aligned bounding boxes
- `OBB2.cpp` - 2D oriented bounding boxes
- `OBB3.cpp` - 3D oriented bounding boxes
- `Sphere3.cpp` - 3D sphere operations
- `Capsule2.cpp` - 2D capsule geometry
- `Cylinder3.cpp` - 3D cylinder geometry
- `Disc2.cpp` - 2D disc/circle geometry
- `Plane2.cpp` - 2D plane representation
- `Plane3.cpp` - 3D plane representation
- `Triangle2.cpp` - 2D triangle operations
- `LineSegment2.cpp` - 2D line segment operations

### Utilities and Advanced Features
- `MathUtils.cpp` - Core mathematical utilities
- `RaycastUtils.cpp` - Geometric intersection testing
- `RandomNumberGenerator.cpp` - Pseudo-random number generation
- `FloatRange.cpp` - Floating-point range operations
- `IntRange.cpp` - Integer range operations
- `Curve2D.cpp` - 2D curve and spline operations

## Changelog

- 2025-10-01: **Math Module Header Organization** - Improved code structure and consistency
  - Added `#pragma once` include guards to 30+ math header files for consistency
  - Enhanced header organization across all geometric primitives (AABB2/3, OBB2/3, Sphere3, etc.)
  - Improved IntVec3 implementation with better operator definitions and utility functions
  - Standardized header structure across Vec2, Vec3, Vec4 implementations
  - Updated all geometric primitive headers (Capsule2, Cylinder3, Disc2, Triangle2, etc.)
  - Cleaned up forward declarations and includes in MathUtils, RaycastUtils
  - Enhanced EulerAngles, Mat44, Plane2/3 header organization
  - Improved consistency in FloatRange, LineSegment2 implementations
- 2025-09-06 19:54:50: Initial module documentation created
- Recent developments: IntVec3 introduction, AABB3 enhancements for 3D collision detection