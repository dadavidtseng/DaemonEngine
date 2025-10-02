//----------------------------------------------------------------------------------------------------
// Curve2D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Curve2D.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/VertexUtils.hpp"

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

//----------------------------------------------------------------------------------------------------
CubicBezierCurve2D::CubicBezierCurve2D(CubicHermiteCurve2D const& cubicHermiteCurve2D)
    : m_startPosition(cubicHermiteCurve2D.m_startPosition),
      m_endPosition(cubicHermiteCurve2D.m_endPosition),
      m_guidePosition1(cubicHermiteCurve2D.m_startPosition + cubicHermiteCurve2D.m_startVelocity * (1.f / 3.f)),
      m_guidePosition2(cubicHermiteCurve2D.m_endPosition - cubicHermiteCurve2D.m_endVelocity * (1.f / 3.f))
{
}

//----------------------------------------------------------------------------------------------------
Vec2 CubicBezierCurve2D::EvaluateAtParametric(float const parametricZeroToOne) const
{
    float const x = ComputeCubicBezier1D(m_startPosition.x, m_guidePosition1.x, m_guidePosition2.x, m_endPosition.x, parametricZeroToOne);
    float const y = ComputeCubicBezier1D(m_startPosition.y, m_guidePosition1.y, m_guidePosition2.y, m_endPosition.y, parametricZeroToOne);
    return Vec2(x, y);
}

//----------------------------------------------------------------------------------------------------
float CubicBezierCurve2D::GetApproximateLength(int const numSubdivisions) const
{
    float totalLength = 0.f;
    Vec2  curPosition = EvaluateAtParametric(0.f);

    for (int i = 0; i < numSubdivisions; i++)
    {
        float const t            = static_cast<float>(i + 1) / static_cast<float>(numSubdivisions);
        Vec2        nextPosition = EvaluateAtParametric(t);
        totalLength += GetDistance2D(curPosition, nextPosition);
        curPosition = nextPosition;
    }

    return totalLength;
}

//----------------------------------------------------------------------------------------------------
Vec2 CubicBezierCurve2D::EvaluateAtApproximateDistance(float const distanceAlongCurve,
                                                       int const   numSubdivisions) const
{
    if (distanceAlongCurve <= 0.f)
    {
        return m_startPosition;
    }

    float totalLength = 0.f;
    Vec2  curPosition = m_startPosition;

    for (int i = 0; i < numSubdivisions; i++)
    {
        float const t            = static_cast<float>(i + 1) / static_cast<float>(numSubdivisions);
        Vec2        nextPosition = EvaluateAtParametric(t);
        float const thisDist     = GetDistance2D(curPosition, nextPosition);

        if (thisDist > distanceAlongCurve - totalLength)
        {
            return Interpolate(curPosition, nextPosition, (distanceAlongCurve - totalLength) / thisDist);
        }

        totalLength += thisDist;
        curPosition = nextPosition;
    }

    return m_endPosition;
}

//----------------------------------------------------------------------------------------------------
CubicHermiteCurve2D::CubicHermiteCurve2D(Vec2 const& startPosition,
                                         Vec2 const& startVelocity,
                                         Vec2 const& endPosition,
                                         Vec2 const& endVelocity)
    : m_startPosition(startPosition),
      m_endPosition(endPosition),
      m_startVelocity(startVelocity),
      m_endVelocity(endVelocity)
{
}

//----------------------------------------------------------------------------------------------------
CubicHermiteCurve2D::CubicHermiteCurve2D(CubicBezierCurve2D const& cubicBezierCurve2D)
    : m_startPosition(cubicBezierCurve2D.m_startPosition),
      m_endPosition(cubicBezierCurve2D.m_endPosition),
      m_startVelocity(3.f * (cubicBezierCurve2D.m_guidePosition1 - cubicBezierCurve2D.m_startPosition)),
      m_endVelocity(3.f * (cubicBezierCurve2D.m_endPosition - cubicBezierCurve2D.m_guidePosition2))
{
}

//----------------------------------------------------------------------------------------------------
Vec2 CubicHermiteCurve2D::EvaluateAtParametric(float const parametricZeroToOne) const
{
    float const squaredT = parametricZeroToOne * parametricZeroToOne;
    float const cubicT   = parametricZeroToOne * parametricZeroToOne * parametricZeroToOne;

    return (2.f * cubicT - 3.f * squaredT + 1.f) * m_startPosition +
        (cubicT - 2.f * squaredT + parametricZeroToOne) * m_startVelocity +
        (-2.f * cubicT + 3.f * squaredT) * m_endPosition -
        (cubicT - squaredT) * m_endVelocity;
}

float CubicHermiteCurve2D::GetApproximateLength(int const numSubdivisions) const
{
    float totalLength = 0.f;
    Vec2  curPosition = EvaluateAtParametric(0.f);

    for (int i = 0; i < numSubdivisions; i++)
    {
        float const t            = static_cast<float>(i + 1) / static_cast<float>(numSubdivisions);
        Vec2        nextPosition = EvaluateAtParametric(t);
        totalLength += GetDistance2D(curPosition, nextPosition);
        curPosition = nextPosition;
    }

    return totalLength;
}

//----------------------------------------------------------------------------------------------------
Vec2 CubicHermiteCurve2D::EvaluateAtApproximateDistance(float const distanceAlongCurve, int const numSubdivisions) const
{
    if (distanceAlongCurve <= 0.f)
    {
        return m_startPosition;
    }

    float totalLength = 0.f;
    Vec2  curPosition = EvaluateAtParametric(0.f);

    for (int i = 0; i < numSubdivisions; i++)
    {
        float const t            = static_cast<float>(i + 1) / static_cast<float>(numSubdivisions);
        Vec2        nextPosition = EvaluateAtParametric(t);
        float const thisDist     = GetDistance2D(curPosition, nextPosition);

        if (thisDist > distanceAlongCurve - totalLength)
        {
            return Interpolate(curPosition, nextPosition, (distanceAlongCurve - totalLength) / thisDist);
        }

        totalLength += thisDist;
        curPosition = nextPosition;
    }

    return m_endPosition;
}

//----------------------------------------------------------------------------------------------------
CatmullRomSpline2D::CatmullRomSpline2D(std::vector<Vec2> const& points)
{
    int const size = static_cast<int>(points.size());

    if (size == 1)
    {
        m_standAlonePoint = points[0];
    }

    for (int i = 0; i < size - 1; i++)
    {
        int const startIndex    = i;
        int const endIndex      = i + 1;
        Vec2      startPosition = points[i];
        Vec2      endPosition   = points[i + 1];
        Vec2      startVelocity = Vec2::ZERO;
        Vec2      endVelocity   = Vec2::ZERO;

        if (startIndex != 0)
        {
            startVelocity = (points[startIndex + 1] - points[startIndex - 1]) * 0.5f;
        }

        if (endIndex != static_cast<int>(points.size() - 1))
        {
            endVelocity = (points[endIndex - 1] - points[endIndex + 1]) * 0.5f;
        }

        m_curves.emplace_back(startPosition, startVelocity, endPosition, endVelocity);
    }
}

//----------------------------------------------------------------------------------------------------
Vec2 CatmullRomSpline2D::EvaluateAtParametric(float parametric) const
{
    int const size = static_cast<int>(m_curves.size());

    if (size == 0)
    {
        return m_standAlonePoint;
    }

    parametric = GetClamped(parametric, 0.f, static_cast<float>(GetNumOfCurves()));
    int index  = RoundDownToInt(parametric);

    if (index == GetNumOfCurves())
    {
        index = GetNumOfCurves() - 1;
    }

    return m_curves[index].EvaluateAtParametric(parametric - static_cast<float>(index));
}

//----------------------------------------------------------------------------------------------------
float CatmullRomSpline2D::GetApproximateLength(int const numSubdivisions) const
{
    float totalLength = 0.f;

    for (CubicHermiteCurve2D const& curve : m_curves)
    {
        totalLength += curve.GetApproximateLength(numSubdivisions);
    }

    return totalLength;
}

//----------------------------------------------------------------------------------------------------
Vec2 CatmullRomSpline2D::EvaluateAtApproximateDistance(float const distanceAlongCurve,
                                                       int const   numSubdivisions) const
{
    int const size = static_cast<int>(m_curves.size());

    if (size == 0)
    {
        return m_standAlonePoint;
    }

    if (distanceAlongCurve <= 0.f)
    {
        return m_curves[0].m_startPosition;
    }

    float totalLength = 0.f;

    for (int i = 0; i < size; i++)
    {
        float const thisLength = m_curves[i].GetApproximateLength(numSubdivisions);
        float const nextLength = totalLength + thisLength;

        if (nextLength > distanceAlongCurve)
        {
            return m_curves[i].EvaluateAtApproximateDistance(distanceAlongCurve - totalLength, numSubdivisions);
        }

        totalLength = nextLength;
    }

    return m_curves[size - 1].m_endPosition;
}

//----------------------------------------------------------------------------------------------------
void CatmullRomSpline2D::ResetAllPoints(std::vector<Vec2> const& points)
{
    m_curves.clear();
    int const size = static_cast<int>(points.size());

    if (size == 1)
    {
        m_standAlonePoint = points[0];
    }

    for (int i = 0; i < size - 1; i++)
    {
        int const startIndex    = i;
        int const endIndex      = i + 1;
        Vec2      startPosition = points[i];
        Vec2      endPosition   = points[i + 1];
        Vec2      startVelocity = Vec2::ZERO;
        Vec2      endVelocity   = Vec2::ZERO;

        if (startIndex != 0)
        {
            startVelocity = (points[startIndex + 1] - points[startIndex - 1]) * 0.5f;
        }

        if (endIndex != size - 1)
        {
            endVelocity = (points[endIndex - 1] - points[endIndex + 1]) * 0.5f;
        }

        m_curves.emplace_back(startPosition, startVelocity, endPosition, endVelocity);
    }
}

//----------------------------------------------------------------------------------------------------
int CatmullRomSpline2D::GetNumOfPoints() const
{
    return static_cast<int>(m_curves.size()) + 1;
}

//----------------------------------------------------------------------------------------------------
int CatmullRomSpline2D::GetNumOfCurves() const
{
    return static_cast<int>(m_curves.size());
}

//----------------------------------------------------------------------------------------------------
Vec2 CatmullRomSpline2D::GetPointAtIndex(int const index) const
{
    int const size = static_cast<int>(m_curves.size());

    if (size == 0)
    {
        return m_standAlonePoint;
    }

    if (index >= size)
    {
        return m_curves[size - 1].m_endPosition;
    }

    if (index <= 0)
    {
        return m_curves[0].m_startPosition;
    }

    return m_curves[index].m_startPosition;
}

//----------------------------------------------------------------------------------------------------
CubicHermiteCurve2D const& CatmullRomSpline2D::GetCubicHermiteCurveAtIndex(int const index) const
{
    return m_curves[index];
}

//----------------------------------------------------------------------------------------------------
Vec2 CatmullRomSpline2D::GetVelocityAtIndex(int const index) const
{
    int const size = static_cast<int>(m_curves.size());

    if (size == 0)
    {
        return Vec2::ZERO;
    }

    if (index >= size || index <= 0)
    {
        return Vec2::ZERO;
    }

    return m_curves[index].m_startVelocity;
}

//----------------------------------------------------------------------------------------------------
void CatmullRomSpline2D::AddVertsForCurve2D(std::vector<Vertex_PCU>& verts,
                                            float const              thickness,
                                            Rgba8 const&             color,
                                            int const                numSubdivisions) const
{
    // draw line segments between points
    int const num = GetNumOfPoints();

    // draw the curve
    for (int i = 0; i < num - 1; i++)
    {
        CubicHermiteCurve2D const& curve = GetCubicHermiteCurveAtIndex(i);

        for (int j = 0; j < numSubdivisions; j++)
        {
            float const t  = static_cast<float>(j) / static_cast<float>(numSubdivisions);
            float const nt = static_cast<float>(j + 1) / static_cast<float>(numSubdivisions);

            AddVertsForLineSegment2D(verts, curve.EvaluateAtParametric(t), curve.EvaluateAtParametric(nt), thickness, false, color);
        }
    }
}