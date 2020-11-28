/*
*  GameTimer��ʵ��
*  GameTimer.cpp
*/

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);  // ��ȡ���ܼ�ʱ����Ƶ��
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::TotalTime()const
{
	// ������Reset��������ǰ����ʱ�䣬�Ҹ���ʱ�䲢����¼�κ���ͣʱ��
	
	// �����������ͣ״̬������Ա���ͣʻʱ�̵���ǰʱ�̵����ʱ��
	// ��֮ǰ�Ѿ�����ͣ���������Ҳ��Ӧ�ü�¼��ͣʱ��
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// �����ʱ��������ͣ״̬��ͬ��֮ǰ����ͣ�����������¼�Ƕ�ʱ��
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
	
	else
	{
		return (float)(((mCurrTime-mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	// ��ñ�֡��ǰһ֡��ʱ���

	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	// ���ã���ʼ����

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);  // ��õ�ǰʱ��ֵ

    // �ڵ�һ֡����֮ǰû���κεĶ���֡����ǰһ��ʱ�����������
	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	// �����ͣ
	
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	// �ۼ�Stop��Start����֮�����ͣʱ�̼��
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	if( mStopped )
	{
		mPausedTime += (startTime - mStopTime);  // �ۼ���ͣʱ�䣨�ܣ�	

		// ������ʱ��ʱ��ǰһ֡��ʱ��mPrevTime����Ч��
		// ���洢������ͣʱ��ǰһ֡�Ŀ�ʼʱ�̣���Ҫ������Ϊ��ǰʱ�̣����ʱ�䣩
		mPrevTime = startTime;  
		mStopTime = 0;  // ��������ͣ״̬
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	// ��ͣ��ʱ��

	// ����ʱ�Ѿ�������ͣ״̬��ʲô������
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;  // �Ӵ˿̿�ʼ��¼��ͣʱ��
		mStopped  = true;      
	}
}

void GameTimer::Tick()
{
	// ����ÿ֮֡���ʱ����

	// ��������ͣ������¼ʱ����
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

	// ��ñ�֡��ʼ��ʾ��ʱ��
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// ��֡��ǰһ֡��ʱ���
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// Ϊ���㱾֡����һ֡��ʱ�����׼��
	mPrevTime = mCurrTime;

	// ���Ǽ����ʱ���Ӧ��Ϊ�Ǹ�ֵ��������������ܵ�����Ϊ��ֵ
	// 1. ���������ڽ���ģʽ
	// 2. �ڼ�����֡ʱ���Ĺ������л�����һ����������QueryPerformanceFrequency�������ý����ͬ�� 
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

