/*
*  GameTimer类实现
*  GameTimer.cpp
*/

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);  // 获取性能计时器的频率
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

float GameTimer::TotalTime()const
{
	// 返回在Reset方法调用前的总时间，且该总时间并不记录任何暂停时间
	
	// 如果正处于暂停状态，则忽略本次停驶时刻到当前时刻的这段时间
	// 若之前已经有暂停的情况，则也不应该记录暂停时间
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime

	if( mStopped )
	{
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// 如果此时不处于暂停状态，同理，之前有暂停的情况，不记录那段时间
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
	// 获得本帧与前一帧的时间差

	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	// 重置（初始化）

	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);  // 获得当前时刻值

    // 在第一帧画面之前没有任何的动画帧，即前一个时间戳并不存在
	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	// 解除暂停
	
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

	// 累加Stop和Start方法之间的暂停时刻间隔
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	if( mStopped )
	{
		mPausedTime += (startTime - mStopTime);  // 累加暂停时间（总）	

		// 重启计时器时，前一帧的时间mPrevTime是无效的
		// 它存储的是暂停时刻前一帧的开始时刻，需要重置它为当前时刻（解除时间）
		mPrevTime = startTime;  
		mStopTime = 0;  // 不再是暂停状态
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	// 暂停计时器

	// 若此时已经处于暂停状态则什么都不做
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		mStopTime = currTime;  // 从此刻开始记录暂停时间
		mStopped  = true;      
	}
}

void GameTimer::Tick()
{
	// 计算每帧之间的时间间隔

	// 若处于暂停，不记录时间间隔
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

	// 获得本帧开始显示的时刻
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// 本帧与前一帧的时间差
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// 为计算本帧与下一帧的时间差做准备
	mPrevTime = mCurrTime;

	// 我们计算的时间差应该为非负值，有两种情况可能导致其为负值
	// 1. 处理器处于节能模式
	// 2. 在计算两帧时间差的过程中切换到另一个处理器（QueryPerformanceFrequency方法调用结果不同） 
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

