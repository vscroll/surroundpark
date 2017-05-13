#ifndef CAPTUREWORKERBASE_H
#define CAPTUREWORKERBASE_H

#include <queue>
#include "common.h"
#include "thread.h"

class CaptureWorkerBase: public Thread
{
public:
    CaptureWorkerBase();
    virtual ~CaptureWorkerBase();

    virtual void setCapCapacity(struct cap_sink_t sink[], struct cap_src_t source[], unsigned int channelNum);
    virtual int openDevice(unsigned int channel[], unsigned int channelNum) = 0;
    virtual void closeDevice() = 0;
    virtual surround_images_t* popOneFrame();
    virtual unsigned int getFrameCount();
    virtual unsigned int getVideoChannelNum() { return mVideoChannelNum; }
    virtual int getResolution(unsigned int channelIndex, unsigned int* width, unsigned int* height);
    virtual int getFPS(unsigned int* fps);
protected:
    struct cap_sink_t mSink[VIDEO_CHANNEL_SIZE];
    struct cap_src_t mSource[VIDEO_CHANNEL_SIZE];

    int mVideoFd[VIDEO_CHANNEL_SIZE];
    unsigned int mVideoChannelNum;
    unsigned int mChannel[VIDEO_CHANNEL_SIZE];

    pthread_mutex_t mMutexQueue;
    std::queue<surround_images_t*> mSurroundImagesQueue;

    double mLastCallTime;
    unsigned int mRealFPS;
    double mStartStatTime;
    double mStatDuration;
    unsigned long mRealFrameCount;
};

#endif // CAPTUREWORKERBASE_H
