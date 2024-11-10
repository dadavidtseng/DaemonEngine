//-----------------------------------------------------------------------------------------------
// VertexUtils.cpp
//

//-----------------------------------------------------------------------------------------------
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/Disc2.hpp"
#include "Engine/Math/LineSegment2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/Triangle2.hpp"

//-----------------------------------------------------------------------------------------------
void TransformVertexArrayXY3D(const int   numVerts,
                              Vertex_PCU* verts,
                              const float uniformScaleXY,
                              const float rotationDegreesAboutZ,
                              Vec2 const& translationXY)
{
	for (int vertIndex = 0; vertIndex < numVerts; ++vertIndex)
	{
		Vec3& position = verts[vertIndex].m_position;

		TransformPositionXY3D(position, uniformScaleXY, rotationDegreesAboutZ, translationXY);
	}
}

void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color)
{
	constexpr int   NUM_SIDES        = 32;
	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart     = CosDegrees(startDegrees);
		float sinStart     = SinDegrees(startDegrees);
		float cosEnd       = CosDegrees(endDegrees);
		float sinEnd       = SinDegrees(endDegrees);

		Vec3 centerPos(discCenter.x, discCenter.y, 0.f);
		Vec3 startOuterPos(discCenter.x + discRadius * cosStart, discCenter.y + discRadius * sinStart, 0.f);
		Vec3 endOuterPos(discCenter.x + discRadius * cosEnd, discCenter.y + discRadius * sinEnd, 0.f);

		verts.emplace_back(centerPos, color);
		verts.emplace_back(startOuterPos, color);
		verts.emplace_back(endOuterPos, color);
	}
}


void AddVertsForDisc2D(std::vector<Vertex_PCU>& verts, const Disc2& disc, Rgba8 const& color)
{
	AddVertsForDisc2D(verts, disc.m_position, disc.m_radius, color);
}


//-----------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, Vec2 const& start, Vec2 const& end, Rgba8 const& color)
{
	// Create vertices for the start and end of the line segment
	verts.emplace_back(Vec3(start.x, start.y, 0.f), color, Vec2(0.f, 0.f));
	verts.emplace_back(Vec3(end.x, end.y, 0.f), color, Vec2(0.f, 0.f));
}

//-----------------------------------------------------------------------------------------------
void AddVertsForLineSegment2D(std::vector<Vertex_PCU>& verts, const LineSegment2& lineSegment, Rgba8 const& color)
{
	// Use the start and end of the line segment to add vertices
	AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, color);
}

void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, Vec2 const& ccw0, Vec2 const& ccw1, Vec2 const& ccw2,
                           Rgba8 const&             color)
{
	verts.emplace_back(Vec3(ccw0.x, ccw0.y, 0.f), color, Vec2(0.f, 0.f));
	verts.emplace_back(Vec3(ccw1.x, ccw1.y, 0.f), color, Vec2(0.f, 0.f));
	verts.emplace_back(Vec3(ccw2.x, ccw2.y, 0.f), color, Vec2(0.f, 0.f));
}

void AddVertsForTriangle2D(std::vector<Vertex_PCU>& verts, const Triangle2& triangle, Rgba8 const& color)
{
	AddVertsForTriangle2D(verts, triangle.m_positionCounterClockwise[0], triangle.m_positionCounterClockwise[1],
	                      triangle.m_positionCounterClockwise[2], color);
}

void AddVertsForAABB2D(std::vector<Vertex_PCU>& verts, const AABB2& alignedBox, Rgba8 const& color)
{
	Vec2 bottomLeft  = alignedBox.m_mins;
	Vec2 bottomRight = Vec2(alignedBox.m_maxs.x, alignedBox.m_mins.y);
	Vec2 topRight    = alignedBox.m_maxs;
	Vec2 topLeft     = Vec2(alignedBox.m_mins.x, alignedBox.m_maxs.y);


	verts.emplace_back(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2(0.f, 0.f));
	verts.emplace_back(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(1.f, 0.f));
	verts.emplace_back(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(0.f, 1.f));

	verts.emplace_back(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(1.f, 0.f));
	verts.emplace_back(Vec3(topRight.x, topRight.y, 0.f), color, Vec2(1.f, 1.f));
	verts.emplace_back(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(0.f, 1.f));
}

//-----------------------------------------------------------------------------------------------
void AddVertsForOBB2D(std::vector<Vertex_PCU>& verts, const OBB2& orientedBox, Rgba8 const& color)
{
	Vec2 cornerPoints[4];
	orientedBox.GetCornerPoints(cornerPoints);

	AddVertsForTriangle2D(verts, cornerPoints[0], cornerPoints[1], cornerPoints[2], color);
	AddVertsForTriangle2D(verts, cornerPoints[0], cornerPoints[2], cornerPoints[3], color);
}

void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Vec2 const& boneStart, Vec2 const& boneEnd, float radius,
                          Rgba8 const&             color)
{
	Vec2 direction = boneEnd - boneStart;
	direction.Normalize();

	Vec2 perpendicular = Vec2(-direction.y, direction.x) * radius;
	Vec2 bottomLeft    = boneStart - perpendicular;
	Vec2 bottomRight   = boneStart + perpendicular;
	Vec2 topLeft       = boneEnd - perpendicular;
	Vec2 topRight      = boneEnd + perpendicular;

	verts.emplace_back(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2(0.f, 0.f));
	verts.emplace_back(Vec3(bottomRight.x, bottomRight.y, 0.f), color, Vec2(1.f, 0.f));
	verts.emplace_back(Vec3(topRight.x, topRight.y, 0.f), color, Vec2(1.f, 1.f));

	verts.emplace_back(Vec3(topRight.x, topRight.y, 0.f), color, Vec2(1.f, 1.f));
	verts.emplace_back(Vec3(topLeft.x, topLeft.y, 0.f), color, Vec2(0.f, 1.f));
	verts.emplace_back(Vec3(bottomLeft.x, bottomLeft.y, 0.f), color, Vec2(0.f, 0.f));

	const Vec2  rotatedDirection = direction.GetRotatedMinus90Degrees();
	const float rotationAngle    = Atan2Degrees(rotatedDirection.y, rotatedDirection.x); // ????????

	const Vec2& halfDiscCenterStart = boneStart;
	const Vec2& halfDiscCenterEnd   = boneEnd;

	AddVertsForHalfDisc2D(verts, halfDiscCenterStart, radius, color, false, rotationAngle); // ???
	AddVertsForHalfDisc2D(verts, halfDiscCenterEnd, radius, color, true, rotationAngle);    // ???
}

void AddVertsForCapsule2D(std::vector<Vertex_PCU>& verts, Capsule2 const& capsule, Rgba8 const& color)
{
	AddVertsForCapsule2D(verts, capsule.m_start, capsule.m_end, capsule.m_radius, color);
}

void AddVertsForHalfDisc2D(std::vector<Vertex_PCU>& verts, Vec2 const& discCenter, float discRadius, Rgba8 const& color,
                           bool                     isTopHalf, float   rotationDegrees)
{
	constexpr int   NUM_SIDES        = 32;
	constexpr float DEGREES_PER_SIDE = 180.f / static_cast<float>(NUM_SIDES); // ????180?

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees   = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);

		if (!isTopHalf)
		{
			startDegrees += 180.f;
			endDegrees += 180.f;
		}

		startDegrees += rotationDegrees;
		endDegrees += rotationDegrees;

		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd   = CosDegrees(endDegrees);
		float sinEnd   = SinDegrees(endDegrees);

		Vec3 centerPos(discCenter.x, discCenter.y, 0.f);
		Vec3 startOuterPos(discCenter.x + discRadius * cosStart, discCenter.y + discRadius * sinStart, 0.f);
		Vec3 endOuterPos(discCenter.x + discRadius * cosEnd, discCenter.y + discRadius * sinEnd, 0.f);

		verts.emplace_back(centerPos, color);
		verts.emplace_back(startOuterPos, color);
		verts.emplace_back(endOuterPos, color);
	}
}

void AddVertsForArrow2D(std::vector<Vertex_PCU>& verts, Vec2 const& tailPos, Vec2 const& tipPos, float arrowSize, float thickness, Rgba8 const& color)
{
	// 計算主線段的方向與單位向量
	Vec2 direction           = tipPos - tailPos;
	Vec2 normalizedDirection = -direction.GetNormalized();

	// 計算箭頭的左右方向，分別為 45 度和 -45 度旋轉
	Vec2 arrowDirectionLeft  = normalizedDirection.GetRotatedDegrees(45.f);
	Vec2 arrowDirectionRight = normalizedDirection.GetRotatedDegrees(-45.f);

	// 計算箭頭左右兩邊的終點
	Vec2 leftArrow  = tipPos + arrowDirectionLeft * arrowSize;
	Vec2 rightArrow = tipPos + arrowDirectionRight * arrowSize;

	// 計算垂直於主線段方向的向量，用於調整粗細
	Vec2 mainPerpendicular = normalizedDirection.GetRotated90Degrees() * (thickness * 0.5f);

	// 主線段的四個頂點
	Vec3 tailLeft  = Vec3((tailPos + mainPerpendicular).x, (tailPos + mainPerpendicular).y, 0.0f);
	Vec3 tailRight = Vec3((tailPos - mainPerpendicular).x, (tailPos - mainPerpendicular).y, 0.0f);
	Vec3 tipLeft   = Vec3((tipPos + mainPerpendicular).x, (tipPos + mainPerpendicular).y, 0.0f);
	Vec3 tipRight  = Vec3((tipPos - mainPerpendicular).x, (tipPos - mainPerpendicular).y, 0.0f);

	// 增加主線段的頂點 (用兩個三角形表示粗線)
	verts.push_back(Vertex_PCU(tailLeft, color));
	verts.push_back(Vertex_PCU(tipLeft, color));
	verts.push_back(Vertex_PCU(tipRight, color));

	verts.push_back(Vertex_PCU(tailLeft, color));
	verts.push_back(Vertex_PCU(tipRight, color));
	verts.push_back(Vertex_PCU(tailRight, color));

	// 計算箭頭左側的垂直向量
	Vec2 leftPerpendicular = arrowDirectionLeft.GetRotated90Degrees() * (thickness * 0.5f);

	// 箭頭左側的四個頂點
	Vec3 leftTipLeft  = Vec3((tipPos + leftPerpendicular).x, (tipPos + leftPerpendicular).y, 0.0f);
	Vec3 leftTipRight = Vec3((tipPos - leftPerpendicular).x, (tipPos - leftPerpendicular).y, 0.0f);
	Vec3 leftEndLeft  = Vec3((leftArrow + leftPerpendicular).x, (leftArrow + leftPerpendicular).y, 0.0f);
	Vec3 leftEndRight = Vec3((leftArrow - leftPerpendicular).x, (leftArrow - leftPerpendicular).y, 0.0f);

	// 增加箭頭左側的頂點
	verts.push_back(Vertex_PCU(leftTipLeft, color));
	verts.push_back(Vertex_PCU(leftEndLeft, color));
	verts.push_back(Vertex_PCU(leftEndRight, color));

	verts.push_back(Vertex_PCU(leftTipLeft, color));
	verts.push_back(Vertex_PCU(leftEndRight, color));
	verts.push_back(Vertex_PCU(leftTipRight, color));

	// 計算箭頭右側的垂直向量
	Vec2 rightPerpendicular = arrowDirectionRight.GetRotated90Degrees() * (thickness * 0.5f);

	// 箭頭右側的四個頂點
	Vec3 rightTipLeft  = Vec3((tipPos + rightPerpendicular).x, (tipPos + rightPerpendicular).y, 0.0f);
	Vec3 rightTipRight = Vec3((tipPos - rightPerpendicular).x, (tipPos - rightPerpendicular).y, 0.0f);
	Vec3 rightEndLeft  = Vec3((rightArrow + rightPerpendicular).x, (rightArrow + rightPerpendicular).y, 0.0f);
	Vec3 rightEndRight = Vec3((rightArrow - rightPerpendicular).x, (rightArrow - rightPerpendicular).y, 0.0f);

	// 增加箭頭右側的頂點
	verts.push_back(Vertex_PCU(rightTipLeft, color));
	verts.push_back(Vertex_PCU(rightEndLeft, color));
	verts.push_back(Vertex_PCU(rightEndRight, color));

	verts.push_back(Vertex_PCU(rightTipLeft, color));
	verts.push_back(Vertex_PCU(rightEndRight, color));
	verts.push_back(Vertex_PCU(rightTipRight, color));
}
