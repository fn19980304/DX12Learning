/*
*  实用数学工具类实现
*  MathHelper.cpp
*  作者： Frank Luna (C)
*/

#include "MathHelper.h"
#include <float.h>
#include <cmath>

using namespace DirectX;

const float MathHelper::Infinity = FLT_MAX;
const float MathHelper::Pi = 3.1415926535f;

float MathHelper::AngleFromXY(float x, float y)
{
	float theta = 0.0f;

	// 第一或者第四象限
	if (x >= 0.0f)
	{
		// 若x = 0，且y > 0，那么atanf(y/x) = +pi/2
		//          且y < 0，那么atanf(y/x) = -pi/2
		theta = atanf(y / x);       // 范围[-pi/2, +pi/2]

		if (theta < 0.0f)
			theta += 2.0f * Pi;     // 范围[0, 2*pi).
	}

	// 第二或者第三象限
	else
		theta = atanf(y / x) + Pi;  // 范围[0, 2*pi).

	return theta;
}

XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// 循环执行直到找到在半球上或者里的一个点
	while (true)
	{
		// 在立方体[-1,1]^3内随机产生一个点
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// 忽略单位球外的点，以此获得一个单位球上的均匀分布
		// 否则点就会更多地聚集在立方体角附近的球面上

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		return XMVector3Normalize(v);
	}
}

XMVECTOR MathHelper::RandHemisphereUnitVec3(XMVECTOR n)
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// 循环执行直到找到在半球上或者里的一个点
	while (true)
	{
		// 在立方体[-1,1]^3内随机产生一个点
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// 忽略单位球外的点，以此获得一个单位球上的均匀分布
		// 否则点就会更多地聚集在立方体角附近的球面上

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		// 忽略下半球的点
		if (XMVector3Less(XMVector3Dot(n, v), Zero))
			continue;

		return XMVector3Normalize(v);
	}
}