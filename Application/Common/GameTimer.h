/*
*  GameTimer��
*  GameTimer.h
*  ���ߣ� Frank Luna (C)
*/

#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime()const;  // �����Ϸ���е���ʱ�䣨������ͣ������λΪ�룩
	float DeltaTime()const;  // ��ñ�֡��ǰһ֡��ʱ����λΪ�룩

	void Reset();  // �ڿ�ʼ��Ϣѭ��ǰ����
	void Start();  // �����ʱ����ͣʱ����
	void Stop();   // ��ͣ��ʱ��ʱ����
	void Tick();   // ÿ֡��Ҫ����

private:
	double mSecondsPerCount;  // ÿһ������ֵ�����˶�����
	double mDeltaTime;        // ��֡��ǰһ֡��ʱ���

	__int64 mBaseTime;        // ��ʼʱ�̣�һ����Reset��������ǰ���䣩
	__int64 mPausedTime;	  // ������ͣʱ��֮��
	__int64 mStopTime;        // ��ʱ��ֹͣ��ʱ��
	__int64 mPrevTime;        // ��ͣʱǰһ֡�Ŀ�ʼʱ��
	__int64 mCurrTime;		  // ��ǰʱ��

	bool mStopped;            // �Ƿ���ͣ
};

#endif // GAMETIMER_H