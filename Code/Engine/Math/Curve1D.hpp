//----------------------------------------------------------------------------------------------------
// Curve1D.hpp
// Generic 1D curve evaluation utilities for mathematical interpolation
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
//----------------------------------------------------------------------------------------------------
#include <vector>

//----------------------------------------------------------------------------------------------------
// Curve1D - Base class for all 1D curve evaluation
// Provides virtual interface for mapping input values to output values
// Useful for non-linear transformations in procedural generation, animations, etc.
//----------------------------------------------------------------------------------------------------
class Curve1D
{
public:
	virtual ~Curve1D() = default;

	// Pure virtual function - returns output value for input parameter t
	// Subclasses implement specific curve behavior (linear, spline, bezier, etc.)
	virtual float Evaluate(float t) const = 0;
};

//----------------------------------------------------------------------------------------------------
// LinearCurve1D - Linear interpolation between start and end values
// Derived from Curve1D per professor's PDF specification
//----------------------------------------------------------------------------------------------------
class LinearCurve1D : public Curve1D
{
public:
	LinearCurve1D() = default;
	explicit LinearCurve1D(float startT, float endT, float startValue, float endValue);

	// Evaluate curve at input t
	// If t < startT: returns startValue (clamped)
	// If t >= endT: returns endValue (clamped)
	// Otherwise: linear interpolation between start and end
	float Evaluate(float t) const override;

	// Accessors for curve editor
	float GetStartT() const { return m_startT; }
	float GetEndT() const { return m_endT; }
	float GetStartValue() const { return m_startValue; }
	float GetEndValue() const { return m_endValue; }

	// Setters for curve editor
	void SetStartT(float t) { m_startT = t; }
	void SetEndT(float t) { m_endT = t; }
	void SetStartValue(float v) { m_startValue = v; }
	void SetEndValue(float v) { m_endValue = v; }

private:
	float m_startT = 0.0f;      // Input range start
	float m_endT = 1.0f;        // Input range end
	float m_startValue = 0.0f;  // Output at startT
	float m_endValue = 1.0f;    // Output at endT
};

//----------------------------------------------------------------------------------------------------
// PiecewiseCurve1D - Multiple linear segments with different t ranges
// Provides flexible curve shapes through user-defined control points
// Derived from Curve1D per specification
//----------------------------------------------------------------------------------------------------
class PiecewiseCurve1D : public Curve1D
{
public:
	// Key-value pair for curve control points
	// (t, value) represents a point on the curve
	struct ControlPoint
	{
		float t;      // Input position [0, 1] or [-1, 1]
		float value;  // Output value at this t
	};

	PiecewiseCurve1D() = default;
	explicit PiecewiseCurve1D(std::vector<ControlPoint> const& points);

	// Evaluate piecewise curve at input t
	// Finds subcurve segment containing t and evaluates it
	// If t is outside all segments, clamps to nearest endpoint
	float Evaluate(float t) const override;

	// Control point management for curve editor
	void AddPoint(float t, float value);
	void RemovePoint(int index);
	void SetPoint(int index, float t, float value);
	ControlPoint GetPoint(int index) const;
	int GetNumPoints() const { return static_cast<int>(m_points.size()); }
	void ClearPoints() { m_points.clear(); }
	void SetPoints(std::vector<ControlPoint> const& points);
	std::vector<ControlPoint> const& GetPoints() const { return m_points; }

private:
	std::vector<ControlPoint> m_points;  // Sorted by t value for binary search

	// Helper: Ensure points are sorted by t
	void SortPoints();
};
