// Capsule2.hpp
#pragma once
#include "Engine/Math/Vec2.hpp"

//-----------------------------------------------------------------------------------------------
struct Capsule2
{
	Vec2  m_start = Vec2(0.f, 0.f);
	Vec2  m_end = Vec2(0.f, 0.f);
	float m_radius = 0.f;

	Capsule2();
	explicit Capsule2(const Vec2& start, const Vec2& end, float radius);

	// Mutators (non-const methods)
	void Translate(const Vec2& translation);
	void SetCenter(const Vec2& newCenter);
	void RotateAboutCenter(const Vec2& rotationDeltaDegrees);

	// Accessors (const methods)
	bool IsPointInside(Vec2 const& point) const;
	Vec2 GetNearestPoint(const Vec2& point) const;
	Vec2 const GetCenter() const;
};