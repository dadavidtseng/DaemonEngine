//----------------------------------------------------------------------------------------------------
// Curve1D.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Engine/Math/Curve1D.hpp"
//----------------------------------------------------------------------------------------------------
#include "Engine/Math/MathUtils.hpp"
#include <algorithm>

//----------------------------------------------------------------------------------------------------
// LinearCurve1D Implementation
//----------------------------------------------------------------------------------------------------

LinearCurve1D::LinearCurve1D(float startT, float endT, float startValue, float endValue)
	: m_startT(startT)
	, m_endT(endT)
	, m_startValue(startValue)
	, m_endValue(endValue)
{
}

//----------------------------------------------------------------------------------------------------
float LinearCurve1D::Evaluate(float t) const
{
	// Per professor's PDF specification (page 4):
	// if t < start: v = 0 (CORRECTED: should return startValue for terrain shaping)
	// if t >= end: v = 1 (CORRECTED: should return endValue for terrain shaping)
	// else: v = interpolate(start, end, t)

	if (t < m_startT)
	{
		return m_startValue;  // Clamp to start value
	}

	if (t >= m_endT)
	{
		return m_endValue;  // Clamp to end value
	}

	// Linear interpolation between start and end
	// Calculate fraction: how far along are we from startT to endT?
	float const fraction = (t - m_startT) / (m_endT - m_startT);
	return Interpolate(m_startValue, m_endValue, fraction);
}

//----------------------------------------------------------------------------------------------------
// PiecewiseCurve1D Implementation
//----------------------------------------------------------------------------------------------------

PiecewiseCurve1D::PiecewiseCurve1D(std::vector<ControlPoint> const& points)
	: m_points(points)
{
	SortPoints();
}

//----------------------------------------------------------------------------------------------------
float PiecewiseCurve1D::Evaluate(float t) const
{
	// Per professor's PDF specification (page 5):
	// find curve c where key[i].t < t < key[i + 1].t
	// v = c.evaluate(fraction_within(key[i].t, key[i + 1].t, t))

	if (m_points.empty())
	{
		return 0.0f;  // No points, return default
	}

	if (m_points.size() == 1)
	{
		return m_points[0].value;  // Single point, return its value
	}

	// Clamp to first point if t is before all segments
	if (t <= m_points.front().t)
	{
		return m_points.front().value;
	}

	// Clamp to last point if t is after all segments
	if (t >= m_points.back().t)
	{
		return m_points.back().value;
	}

	// Find the segment containing t using binary search
	// We want the largest i where points[i].t <= t
	for (size_t i = 0; i < m_points.size() - 1; ++i)
	{
		if (t >= m_points[i].t && t < m_points[i + 1].t)
		{
			// Found the segment: [points[i], points[i+1]]
			// Create a LinearCurve1D for this segment and evaluate
			LinearCurve1D segment(
				m_points[i].t,
				m_points[i + 1].t,
				m_points[i].value,
				m_points[i + 1].value
			);

			return segment.Evaluate(t);
		}
	}

	// Should never reach here if points are sorted and t is in range
	// But just in case, return last point value
	return m_points.back().value;
}

//----------------------------------------------------------------------------------------------------
void PiecewiseCurve1D::AddPoint(float t, float value)
{
	m_points.push_back({ t, value });
	SortPoints();
}

//----------------------------------------------------------------------------------------------------
void PiecewiseCurve1D::RemovePoint(int index)
{
	if (index >= 0 && index < static_cast<int>(m_points.size()))
	{
		m_points.erase(m_points.begin() + index);
	}
}

//----------------------------------------------------------------------------------------------------
void PiecewiseCurve1D::SetPoint(int index, float t, float value)
{
	if (index >= 0 && index < static_cast<int>(m_points.size()))
	{
		m_points[index].t = t;
		m_points[index].value = value;
		SortPoints();
	}
}

//----------------------------------------------------------------------------------------------------
PiecewiseCurve1D::ControlPoint PiecewiseCurve1D::GetPoint(int index) const
{
	if (index >= 0 && index < static_cast<int>(m_points.size()))
	{
		return m_points[index];
	}
	return { 0.0f, 0.0f };
}

//----------------------------------------------------------------------------------------------------
void PiecewiseCurve1D::SetPoints(std::vector<ControlPoint> const& points)
{
	m_points = points;
	SortPoints();
}

//----------------------------------------------------------------------------------------------------
void PiecewiseCurve1D::SortPoints()
{
	// Sort points by t value for correct piecewise evaluation
	std::sort(m_points.begin(), m_points.end(),
		[](ControlPoint const& a, ControlPoint const& b) {
			return a.t < b.t;
		});
}
