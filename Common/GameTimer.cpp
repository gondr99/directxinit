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
	//성능 타이머로부터 현재 시간의 틱을 얻을 때 사용하는 함수(win32), 초당 몇카운트가 나오는지를 가져온다
	mSecondsPerCount = 1.0 / (double)countsPerSec;
}

/// <summary>
/// Reset이 실행된 후부터 지금까지의 시간을 리턴한다. 단 pause상태일 때는 값을 더하지 않는다.
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
	
	//현재 정지중이라면 
	if( mStopped )
	{
		//정지된 시간에서 멈춰있던 시간은 빼주고 거기서 다시 baseTime을 빼주면 된다.
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
		//정지중이 아니라면 현재시간에서 멈춰있던 시간을 빼고 거기다 베이스타임을 빼주면 된다.
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
	//리셋되면 베이스시간을 리셋한다. 
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
		//현재시간에서 정지되었던 시간으 뺀만큼을 paused타임에 넣어준다.
		mPausedTime += (startTime - mStopTime);	

		mPrevTime = startTime; //이전타임을 현지시간으로 돌리고 정지 풀어준다.
		mStopTime = 0;
		mStopped  = false;
	}
}

void GameTimer::Stop()
{
	//이미 정지상태라면 아무일도 하지 않고 정지상태가 아닌경우에만 작업시작
	if( !mStopped )
	{
		__int64 currTime;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		//현재시간을 정지되었던 시간으로 기록
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

	//현재 프레임의 시간을 얻는다.
	__int64 currTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
	mCurrTime = currTime;

	// 현재시간과 이전 시간 사의 델타시간을 구한다.
	mDeltaTime = (mCurrTime - mPrevTime)*mSecondsPerCount;

	// 다음 프레임 계싼을 위해 이전에 현재 시간 넣어준다.
	mPrevTime = mCurrTime;

	// 음수가 나오지 않도록 강제로 설정한다.
	// 프로세서가 절전모드로 들어가버리거나 실행이 다른 프로세서와 엉키게 되면 델타타임이 음수로 처리될 수 있다.
	if(mDeltaTime < 0.0)
	{
		mDeltaTime = 0.0;
	}
}

