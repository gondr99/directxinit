//***************************************************************************************
// GameTimer.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include <windows.h>
#include "GameTimer.h"

GameTimer::GameTimer()
: mSecondsPerCount(0.0), mDeltaTime(-1.0), mBaseTime(0), 
  mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false), mStopTime(0)
{
	__int64 countsPerSec;
	QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
	//���� Ÿ�̸ӷκ��� ���� �ð��� ƽ�� ���� �� ����ϴ� �Լ�(win32), �ʴ� ��ī��Ʈ�� ���������� �����´�
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

/// <summary>
/// Reset�� ����� �ĺ��� ���ݱ����� �ð��� �����Ѵ�. �� pause������ ���� ���� ������ �ʴ´�.
/// </summary>
/// <returns></returns>
float GameTimer::TotalTime()const
{
	// If we are stopped, do not count the time that has passed since we stopped.
	// Moreover, if we previously already had a pause, the distance 
	// mStopTime - mBaseTime includes paused time, which we do not want to count.
	// To correct this, we can subtract the paused time from mStopTime:  
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mStopTime    mCurrTime
	
	//���� �������̶�� 
	if( mStopped )
	{
		//������ �ð����� �����ִ� �ð��� ���ְ� �ű⼭ �ٽ� baseTime�� ���ָ� �ȴ�.
		return (float)(((mStopTime - mPausedTime)-mBaseTime)*mSecondsPerCount);
	}

	// The distance mCurrTime - mBaseTime includes paused time,
	// which we do not want to count.  To correct this, we can subtract 
	// the paused time from mCurrTime:  
	//
	//  (mCurrTime - mPausedTime) - mBaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  mBaseTime       mStopTime        startTime     mCurrTime
	
	else
	{
		//�������� �ƴ϶�� ����ð����� �����ִ� �ð��� ���� �ű�� ���̽�Ÿ���� ���ָ� �ȴ�.
		return (float)(((mCurrTime-mPausedTime)-mBaseTime)*mSecondsPerCount);
	}
}

float GameTimer::DeltaTime()const
{
	return (float)mDeltaTime;
}

void GameTimer::Reset()
{
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	//���µǸ� ���̽��ð��� �����Ѵ�. 
	mBaseTime = currTime;
	mPrevTime = currTime;
	mStopTime = 0;
	mStopped  = false;
}

void GameTimer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);


	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  mBaseTime       mStopTime        startTime     

	if( mStopped )
	{
		//����ð����� �����Ǿ��� �ð��� ����ŭ�� pausedŸ�ӿ� �־��ش�.
		mPausedTime += (startTime - mStopTime);	

		mPrevTime = startTime; //����Ÿ���� �����ð����� ������ ���� Ǯ���ش�.
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	//�̹� �������¶�� �ƹ��ϵ� ���� �ʰ� �������°� �ƴѰ�쿡�� �۾�����
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		//����ð��� �����Ǿ��� �ð����� ���
		mStopTime = currTime;
		mStopped  = true;
	}
}

void GameTimer::Tick()
{
	if( mStopped )
	{
		mDeltaTime = 0.0;
		return;
	}

	//���� �������� �ð��� ��´�.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// ����ð��� ���� �ð� ���� ��Ÿ�ð��� ���Ѵ�.
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// ���� ������ ����� ���� ������ ���� �ð� �־��ش�.
	mPrevTime = mCurrTime;

	// ������ ������ �ʵ��� ������ �����Ѵ�.
	// ���μ����� �������� �������ų� ������ �ٸ� ���μ����� ��Ű�� �Ǹ� ��ŸŸ���� ������ ó���� �� �ִ�.
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

