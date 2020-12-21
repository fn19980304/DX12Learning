/*
*  ʵ����ѧ������ʵ��
*  MathHelper.cpp
*  ���ߣ� Frank Luna (C)
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

	// ��һ���ߵ�������
	if (x >= 0.0f)
	{
		// ��x = 0����y > 0����ôatanf(y/x) = +pi/2
		//          ��y < 0����ôatanf(y/x) = -pi/2
		theta = atanf(y / x);       // ��Χ[-pi/2, +pi/2]

		if (theta < 0.0f)
			theta += 2.0f * Pi;     // ��Χ[0, 2*pi).
	}

	// �ڶ����ߵ�������
	else
		theta = atanf(y / x) + Pi;  // ��Χ[0, 2*pi).

	return theta;
}

XMVECTOR MathHelper::RandUnitVec3()
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// ѭ��ִ��ֱ���ҵ��ڰ����ϻ������һ����
	while (true)
	{
		// ��������[-1,1]^3���������һ����
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// ���Ե�λ����ĵ㣬�Դ˻��һ����λ���ϵľ��ȷֲ�
		// �����ͻ����ؾۼ���������Ǹ�����������

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		return XMVector3Normalize(v);
	}
}

XMVECTOR MathHelper::RandHemisphereUnitVec3(XMVECTOR n)
{
	XMVECTOR One = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR Zero = XMVectorZero();

	// ѭ��ִ��ֱ���ҵ��ڰ����ϻ������һ����
	while (true)
	{
		// ��������[-1,1]^3���������һ����
		XMVECTOR v = XMVectorSet(MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), MathHelper::RandF(-1.0f, 1.0f), 0.0f);

		// ���Ե�λ����ĵ㣬�Դ˻��һ����λ���ϵľ��ȷֲ�
		// �����ͻ����ؾۼ���������Ǹ�����������

		if (XMVector3Greater(XMVector3LengthSq(v), One))
			continue;

		// �����°���ĵ�
		if (XMVector3Less(XMVector3Dot(n, v), Zero))
			continue;

		return XMVector3Normalize(v);
	}
}