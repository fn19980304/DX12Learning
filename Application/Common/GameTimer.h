/*
*  GameTimer类
*  GameTimer.h
*  作者： Frank Luna (C)
*/

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const;  // 获得游戏运行的总时间（不记暂停）（单位为秒）
	float DeltaTime()const;  // 获得本帧与前一帧的时间差（单位为秒）

	void Reset();  // 在开始消息循环前调用
	void Start();  // 解除计时器暂停时调用
	void Stop();   // 暂停计时器时调用
	void Tick();   // 每帧都要调用

private:
	double mSecondsPerCount;  // 每一个计数值代表了多少秒
	double mDeltaTime;        // 本帧与前一帧的时间差

	__int64 mBaseTime;        // 起始时刻（一般在Reset方法调用前不变）
	__int64 mPausedTime;	  // 所有暂停时间之和
	__int64 mStopTime;        // 计时器停止的时刻
	__int64 mPrevTime;        // 暂停时前一帧的开始时刻
	__int64 mCurrTime;		  // 当前时刻

	bool mStopped;            // 是否暂停
};

#endif // GAMETIMER_H