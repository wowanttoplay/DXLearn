#include "GameTimer.h"

#include "BaseWindow.h"

GameTimer::GameTimer() :
mSecondsPerCount(0.0), mDeltaTime(-1.0),
mBaseTime(0), mPausedTime(0), mPrevTime(0), mCurrTime(0), mStopped(false)
{
    __int64 countPerSec;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countPerSec);
    mSecondsPerCount = 1.0 / static_cast<double>(countPerSec);
}

float GameTimer::TotalTime() const
{
    if (mStopped)
    {
        return static_cast<float>(((mStopTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
    }
    else
    {
        return static_cast<float>(((mCurrTime - mPausedTime) - mBaseTime) * mSecondsPerCount);
    }
}

float GameTimer::DeltaTime() const
{
    return mDeltaTime;
}

void GameTimer::Reset()
{
    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mBaseTime  = currTime;
    mPrevTime = currTime;
    mStopTime = 0;
    mStopped = false;
}

void GameTimer::Start()
{
    __int64 startTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

    if (mStopped)
    {
        mPausedTime += (startTime - mStopTime);

        mPrevTime = startTime;
        mStopTime = 0;
        mStopped = false;
    }
}

void GameTimer::Stop()
{
    if (!mStopped)
    {
        mStopped = true;

        __int64 currTime;
        QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
        mStopTime = currTime;
    }
}

void GameTimer::Tick()
{
    if (mStopped)
    {
        mDeltaTime = 0.0;
        return;
    }

    __int64 currTime;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mCurrTime = currTime;

    mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;

    mPrevTime = currTime;

    // Force nonnegative.  The DXSDK's CDXUTTimer mentions that if the 
    // processor goes into a power save mode or we get shuffled to another
    // processor, then mDeltaTime can be negative.
    if (mDeltaTime < 0.0)
    {
     mDeltaTime = 0.0;   
    }
}


