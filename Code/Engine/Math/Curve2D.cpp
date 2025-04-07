//----------------------------------------------------------------------------------------------------
// Curve2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Curve2D.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"

//----------------------------------------------------------------------------------------------------
CubicBezierCurve2D::CubicBezierCurve2D(Vec2 const& startPosition,
                                       Vec2 const& guidePosition1,
                                       Vec2 const& guidePosition2,
                                       Vec2 const& endPosition)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_guidePosition1(guidePosition1),
      m_guidePosition2(guidePosition2)
{
}

CubicBezierCurve2D::CubicBezierCurve2D(CubicHermiteCurve2D const& cubicHermiteCurve2D)
    : m_startPosition(cubicHermiteCurve2D.m_startPos)
      , m_endPosition(cubicHermiteCurve2D.m_endPos)
      , m_guidePosition1(cubicHermiteCurve2D.m_startPos + cubicHermiteCurve2D.m_startVel * (1.f / 3.f))
      , m_guidePosition2(cubicHermiteCurve2D.m_endPos - cubicHermiteCurve2D.m_endVel * (1.f / 3.f))
{
    // v1 = 3 * (b - a)
    // v2 = 3 * (d - c)
}

Vec2 CubicBezierCurve2D::EvaluateAtParametric(float const parametricZeroToOne) const
{
    float const x = ComputeCubicBezier1D(m_startPosition.x, m_guidePosition1.x, m_guidePosition2.x, m_endPosition.x, parametricZeroToOne);
    float const y = ComputeCubicBezier1D(m_startPosition.y, m_guidePosition1.y, m_guidePosition2.y, m_endPosition.y, parametricZeroToOne);
    return Vec2(x, y);
}

float CubicBezierCurve2D::GetApproximateLength(int const numSubdivisions) const
{
    float totalLength = 0.f;
    Vec2  curPos      = EvaluateAtParametric(0.f);
    for (int i = 0; i < numSubdivisions; i++)
    {
        float t       = (float)(i + 1) / (float)numSubdivisions;
        Vec2  nextPos = EvaluateAtParametric(t);
        totalLength += GetDistance2D(curPos, nextPos);
        curPos = nextPos;
    }
    return totalLength;
}

Vec2 CubicBezierCurve2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions /*= 64 */) const
{
    if (distanceAlongCurve <= 0.f)
    {
        return m_startPosition;
    }
    float totalLength = 0.f;
    Vec2  curPos;
    Vec2  nextPos;
    curPos = m_startPosition;
    for (int i = 0; i < numSubdivisions; i++)
    {
        float t        = (float)(i + 1) / (float)numSubdivisions;
        nextPos        = EvaluateAtParametric(t);
        float thisDist = GetDistance2D(curPos, nextPos);
        if (thisDist > distanceAlongCurve - totalLength)
        {
            return Interpolate(curPos, nextPos, (distanceAlongCurve - totalLength) / thisDist);
        }
        totalLength += thisDist;
        curPos = nextPos;
    }
    return m_endPosition;
}

CubicHermiteCurve2D::CubicHermiteCurve2D(Vec2 const& startPos, Vec2 const& startVel, Vec2 const& endPos, Vec2 const& endVel)
    : m_startPos(startPos),
      m_endPos(endPos),
      m_startVel(startVel),
      m_endVel(endVel)
{
}

CubicHermiteCurve2D::CubicHermiteCurve2D(CubicBezierCurve2D const& cubicBezierCurve2D)
    : m_startPos(cubicBezierCurve2D.m_startPosition),
      m_endPos(cubicBezierCurve2D.m_endPosition),
      m_startVel(3.f * (cubicBezierCurve2D.m_guidePosition1 - cubicBezierCurve2D.m_startPosition)),
      m_endVel(3.f * (cubicBezierCurve2D.m_endPosition - cubicBezierCurve2D.m_guidePosition2))
{
}

CubicHermiteCurve2D::CubicHermiteCurve2D()
{
}

Vec2 CubicHermiteCurve2D::EvaluateAtParametric(float parametricZeroToOne) const
{
    float squaredT = parametricZeroToOne * parametricZeroToOne;
    float cubicT   = parametricZeroToOne * parametricZeroToOne * parametricZeroToOne;
    return (2.f * cubicT - 3.f * squaredT + 1.f) * m_startPos
        + (cubicT - 2.f * squaredT + parametricZeroToOne) * m_startVel
        + (-2.f * cubicT + 3.f * squaredT) * m_endPos
        - (cubicT - squaredT) * m_endVel;
    /*Vec2 guidePos1 = m_startPos + m_startVel * (1.f / 3.f);
    Vec2 guidePos2 = m_endPos - m_endVel * (1.f / 3.f);

    float x = ComputeCubicBezier1D( m_startPos.x, guidePos1.x, guidePos2.x, m_endPos.x, parametricZeroToOne );
    float y = ComputeCubicBezier1D( m_startPos.y, guidePos1.y, guidePos2.y, m_endPos.y, parametricZeroToOne );
    return Vec2( x, y );*/
}

float CubicHermiteCurve2D::GetApproximateLength(int numSubdivisions /*= 64 */) const
{
    float totalLength = 0.f;
    Vec2  curPos;
    Vec2  nextPos;
    curPos = EvaluateAtParametric(0.f);
    for (int i = 0; i < numSubdivisions; i++)
    {
        float t = (float)(i + 1) / (float)numSubdivisions;
        nextPos = EvaluateAtParametric(t);
        totalLength += GetDistance2D(curPos, nextPos);
        curPos = nextPos;
    }
    return totalLength;
}

Vec2 CubicHermiteCurve2D::EvaluateAtApproximateDistance(float distanceAlongCurve, int numSubdivisions /*= 64 */) const
{
    if (distanceAlongCurve <= 0.f)
    {
        return m_startPos;
    }
    float totalLength = 0.f;
    Vec2  curPos;
    Vec2  nextPos;
    curPos = EvaluateAtParametric(0.f);
    for (int i = 0; i < numSubdivisions; i++)
    {
        float t        = (float)(i + 1) / (float)numSubdivisions;
        nextPos        = EvaluateAtParametric(t);
        float thisDist = GetDistance2D(curPos, nextPos);
        if (thisDist > distanceAlongCurve - totalLength)
        {
            return Interpolate(curPos, nextPos, (distanceAlongCurve - totalLength) / thisDist);
        }
        totalLength += thisDist;
        curPos = nextPos;
    }
    return m_endPos;
}

CatmullRomSpline2D::CatmullRomSpline2D(std::vector<Vec2> const& points)
{
    int size = (int)points.size();
    if (size == 1)
    {
        m_standAlonePoint = points[0];
    }
    for (int i = 0; i < size - 1; i++)
    {
        int  startIndex = i;
        int  endIndex   = i + 1;
        Vec2 startPos   = points[i];
        Vec2 endPos     = points[i + 1];
        Vec2 startVel   = Vec2();
        Vec2 endVel     = Vec2();
        if (startIndex != 0)
        {
            startVel = (points[startIndex + 1] - points[startIndex - 1]) * 0.5f;
        }
        if (endIndex != (int)(points.size() - 1))
        {
            endVel = (points[endIndex - 1] - points[endIndex + 1]) * 0.5f;
        }
        m_curves.emplace_back(startPos, startVel, endPos, endVel);
    }
}

CatmullRomSpline2D::CatmullRomSpline2D()
{
}

Vec2 CatmullRomSpline2D::EvaluateAtParametric(float parametric) const
{
    if ((int)m_curves.size() == 0)
    {
        return m_standAlonePoint;
    }
    parametric = GetClamped(parametric, 0.f, (float)GetNumOfCurves());
    int index  = RoundDownToInt(parametric);
    if (index == GetNumOfCurves())
    {
        index = GetNumOfCurves() - 1;
    }
    return m_curves[index].EvaluateAtParametric(parametric - (float)index);
}

float CatmullRomSpline2D::GetApproximateLength(int numSubdivisions /*= 64 */) const
{
    float totalLength = 0.f;
    for (auto& curve : m_curves)
    {
        totalLength += curve.GetApproximateLength(numSubdivisions);
    }
    return totalLength;
}

Vec2 CatmullRomSpline2D::EvaluateAtApproximateDistance(float const distanceAlongCurve,
                                                       int const   numSubdivisions /*= 64 */) const
{
    int size = (int)m_curves.size();
    if (size == 0)
    {
        return m_standAlonePoint;
    }
    if (distanceAlongCurve <= 0.f)
    {
        return m_curves[0].m_startPos;
    }
    float totalLength = 0.f;
    for (int i = 0; i < size; i++)
    {
        float thisLength = m_curves[i].GetApproximateLength(numSubdivisions);
        float nextLength = totalLength + thisLength;
        if (nextLength > distanceAlongCurve)
        {
            return m_curves[i].EvaluateAtApproximateDistance(distanceAlongCurve - totalLength, numSubdivisions);
        }
        totalLength = nextLength;
    }
    return m_curves[size - 1].m_endPos;
}

void CatmullRomSpline2D::ResetAllPoints(std::vector<Vec2> const& points)
{
    m_curves.clear();
    int size = (int)points.size();
    if (size == 1)
    {
        m_standAlonePoint = points[0];
    }
    for (int i = 0; i < size - 1; i++)
    {
        int  startIndex = i;
        int  endIndex   = i + 1;
        Vec2 startPos   = points[i];
        Vec2 endPos     = points[i + 1];
        Vec2 startVel   = Vec2();
        Vec2 endVel     = Vec2();
        if (startIndex != 0)
        {
            startVel = (points[startIndex + 1] - points[startIndex - 1]) * 0.5f;
        }
        if (endIndex != (int)(points.size() - 1))
        {
            endVel = (points[endIndex - 1] - points[endIndex + 1]) * 0.5f;
        }
        m_curves.emplace_back(startPos, startVel, endPos, endVel);
    }
}

int CatmullRomSpline2D::GetNumOfPoints() const
{
    return (int)m_curves.size() + 1;
}

int CatmullRomSpline2D::GetNumOfCurves() const
{
    return (int)m_curves.size();
}

Vec2 CatmullRomSpline2D::GetPointAtIndex(int index) const
{
    int size = (int)m_curves.size();
    if (size == 0)
    {
        return m_standAlonePoint;
    }
    if (index >= size)
    {
        return m_curves[size - 1].m_endPos;
    }
    else if (index <= 0)
    {
        return m_curves[0].m_startPos;
    }
    else
    {
        return m_curves[index].m_startPos;
    }
}

CubicHermiteCurve2D const& CatmullRomSpline2D::GetCubicHermiteCurveAtIndex(int index) const
{
    return m_curves[index];
}

Vec2 CatmullRomSpline2D::GetVelocityAtIndex(int index) const
{
    int size = (int)m_curves.size();
    if (size == 0)
    {
        return Vec2();
    }
    if (index >= size || index <= 0)
    {
        return Vec2();
    }
    else
    {
        return m_curves[index].m_startVel;
    }
}

void CatmullRomSpline2D::AddVertsForCurve2D(std::vector<Vertex_PCU>& verts, float thickness, Rgba8 const& color, int numSubdivisions /*= 64 */) const
{
    // draw line segments between points
    int num = GetNumOfPoints();

    // draw the curve
    for (int k = 0; k < num - 1; k++)
    {
        CubicHermiteCurve2D const& curve = GetCubicHermiteCurveAtIndex(k);
        for (int i = 0; i < numSubdivisions; i++)
        {
            float t  = (float)i / numSubdivisions;
            float nt = (float)(i + 1) / numSubdivisions;
            AddVertsForLineSegment2D( verts, curve.EvaluateAtParametric( t ), curve.EvaluateAtParametric( nt ), thickness,false, color );
        }
    }
}
