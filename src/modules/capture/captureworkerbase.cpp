#include "captureworkerbase.h"
#include <string.h>
#include <stdio.h>

CaptureWorkerBase::CaptureWorkerBase()
{
    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        mVideoFd[i] = -1;

	    memset(&mSink[i], 0, sizeof(mSink[i]));
	    memset(&mSource[i], 0, sizeof(mSource[i]));

        mChannel[i] = 0;
    }
    
    mVideoChannelNum = 0;

    pthread_mutex_init(&mMutexQueue, NULL);

    //focus source
    mFocusChannelIndex = -1;
    memset(&mFocusSource, 0, sizeof(mFocusSource));
    pthread_mutex_init(&mMutexFocusSourceQueue, NULL);

    mLastCallTime = 0;

    mRealFPS = 0;
    mStartStatTime = 0;
    mStatDuration = 0;
    mRealFrameCount = 0;
}

CaptureWorkerBase::~CaptureWorkerBase()
{
}

void CaptureWorkerBase::setFocusSource(int focusChannelIndex, struct cap_src_t* focusSource)
{
    if (focusChannelIndex < 0
        || focusChannelIndex >= VIDEO_CHANNEL_SIZE)
    {
        return;
    }

    mFocusChannelIndex = focusChannelIndex;
    memcpy(&mFocusSource, focusSource, sizeof(mFocusSource));
}

void CaptureWorkerBase::setCapCapacity(struct cap_sink_t sink[], struct cap_src_t source[], unsigned int channelNum)
{
    mVideoChannelNum = channelNum <= VIDEO_CHANNEL_SIZE ? channelNum: VIDEO_CHANNEL_SIZE;
    for (unsigned int i = 0; i < mVideoChannelNum; ++i)
    {
	    memcpy(&mSink[i], &sink[i], sizeof(mSink[i]));
	    memcpy(&mSource[i], &source[i], sizeof(mSource[i]));
    }	
}

surround_images_t* CaptureWorkerBase::popOneFrame()
{

    struct surround_images_t* surroundImage = NULL;
    pthread_mutex_lock(&mMutexQueue);
    if (mSurroundImagesQueue.size() > 0)
    {
        surroundImage = mSurroundImagesQueue.front();
        mSurroundImagesQueue.pop();
    }
    pthread_mutex_unlock(&mMutexQueue);

    return surroundImage;
}

surround_image_t* CaptureWorkerBase::popOneFrame4FocusSource()
{
    struct surround_image_t* surroundImage = NULL;
    pthread_mutex_lock(&mMutexFocusSourceQueue);
    if (mFocuseSourceQueue.size() > 0)
    {
        surroundImage = mFocuseSourceQueue.front();
        mFocuseSourceQueue.pop();
    }
    pthread_mutex_unlock(&mMutexFocusSourceQueue);

    return surroundImage;
}

int CaptureWorkerBase::getResolution(unsigned int channelIndex, unsigned int* width, unsigned int* height)
{
    if (channelIndex >= VIDEO_CHANNEL_SIZE)
    {
        return -1;
    }

    *width = mSource[channelIndex].width;
    *height = mSource[channelIndex].height;

    return 0;
}

int CaptureWorkerBase::getFPS(unsigned int* fps)
{
#if 0
    unsigned int interval = getInterval();
    if (interval > 0)
    {
        *fps = 1000/interval;
	return 0;
    }
#else
    if (mRealFPS > 0)
    {
        *fps = mRealFPS;
	    return 0;
    }

#endif
    return -1;
}