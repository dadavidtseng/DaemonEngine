//----------------------------------------------------------------------------------------------------
// Curve2D.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include <vector>

//-Forward-Declaration--------------------------------------------------------------------------------
class CubicHermiteCurve2D;

//----------------------------------------------------------------------------------------------------
class CubicBezierCurve2D
{
public:
    CubicBezierCurve2D();
    CubicBezierCurve2D(Vec2 const& startPos, Vec2 const& guidePos1, Vec2 const& guidePos2, Vec2 const& endPos);
    explicit CubicBezierCurve2D(CubicHermiteCurve2D const& cubicHermiteCurve2D);
    Vec2     EvaluateAtParametric(float parametricZeroToOne) const;
    float    GetApproximateLength(int numSubdivisions = 64) const;
    Vec2     EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;

public:
    Vec2 m_startPos;
    Vec2 m_endPos;
    Vec2 m_guidePos1;
    Vec2 m_guidePos2;
};

class CubicHermiteCurve2D
{
public:
    CubicHermiteCurve2D();
    CubicHermiteCurve2D(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel);
    explicit CubicHermiteCurve2D(CubicBezierCurve2D const& cubicBezierCurve2D);
    Vec2     EvaluateAtParametric(float parametricZeroToOne) const;
    float    GetApproximateLength(int numSubdivisions = 64) const;
    Vec2     EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;

    Vec2 m_startPos;
    Vec2 m_endPos;
    Vec2 m_startVel;
    Vec2 m_endVel;
};

class CatmullRomSpline2D
{
public:
    CatmullRomSpline2D();
    explicit CatmullRomSpline2D(std::vector<Vec2> const& points);

    Vec2  EvaluateAtParametric(float parametric) const;
    float GetApproximateLength(int numSubdivisions = 64) const;
    Vec2  EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions = 64) const;
    void  ResetAllPoints(std::vector<Vec2> const& points);

    int                        GetNumOfPoints() const;
    int                        GetNumOfCurves() const;
    Vec2                       GetPointAtIndex(int index) const;
    CubicHermiteCurve2D const& GetCubicHermiteCurveAtIndex(int index) const;
    Vec2                       GetVelocityAtIndex(int index) const;

    void AddVertsForCurve2D(std::vector<Vertex_PCU>& verts, float thickness, Rgba8 const& color, int numSubdivisions = 64) const;

private:
    std::vector<CubicHermiteCurve2D> m_curves;

    Vec2 m_standAlonePoint;
};;
