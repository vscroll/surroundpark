// author: Andre Silva 
// email: andreluizeng@yahoo.com.br

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include "common.h"
#include "ICapture.h"
#include "captureimpl.h"
#include "imageshm.h"
#include "util.h"
#include <linux/input.h>

class FocusSourceSHMWriteWorker : public Thread
{
public:
    FocusSourceSHMWriteWorker(ICapture* capture);
    virtual ~FocusSourceSHMWriteWorker();

public:
    virtual void run();

private:
    ICapture* mCapture;
    ImageSHM* mImageSHM;
};

FocusSourceSHMWriteWorker::FocusSourceSHMWriteWorker(ICapture* capture)
{
    mCapture = capture;
    mImageSHM = new ImageSHM();
    mImageSHM->create((key_t)SHM_FOCUS_SOURCE_ID, SHM_FOCUS_SOURCE_SIZE);
}

FocusSourceSHMWriteWorker::~FocusSourceSHMWriteWorker()
{
    if (NULL != mImageSHM)
    {
        mImageSHM->destroy();
        delete mImageSHM;
        mImageSHM = NULL;
    }
}

void FocusSourceSHMWriteWorker::run()
{
    if (NULL == mCapture
        || NULL == mImageSHM)
    {
        return;
    }

    surround_image_t* focusSource = mCapture->popOneFrame4FocusSource();
    if (NULL != focusSource)
    {
        unsigned int channel = mCapture->getFocusChannelIndex();
#if 0
        clock_t start = clock();
#endif
        mImageSHM->writeFocusSource(focusSource, channel);
#if 0
        std::cout << "FocusSourceSHMWriteWorker run: " << (double)(clock()-start)/CLOCKS_PER_SEC
                << " channel:" << channel
                << " width:" << sideImage->width
                << " height:" << sideImage->height
                << " size:" << sideImage->size
                << " timestamp:" << sideImage->timestamp
                << std::endl;
#endif
    }
}

class AllSourcesSHMWriteWorker : public Thread
{
public:
    AllSourcesSHMWriteWorker(ICapture* capture);
    virtual ~AllSourcesSHMWriteWorker();

public:
    virtual void run();

private:
    ICapture* mCapture;
    ImageSHM* mImageSHM;
    clock_t mLastCallTime;
};

AllSourcesSHMWriteWorker::AllSourcesSHMWriteWorker(ICapture* capture)
{
    mCapture = capture;
    mImageSHM = new ImageSHM();
    mImageSHM->create((key_t)SHM_ALL_SOURCES_ID, SHM_ALL_SOURCES_SIZE);
    mLastCallTime = 0;
}

AllSourcesSHMWriteWorker::~AllSourcesSHMWriteWorker()
{
    if (NULL != mImageSHM)
    {
        mImageSHM->destroy();
        delete mImageSHM;
        mImageSHM = NULL;
    }
}

void AllSourcesSHMWriteWorker::run()
{
#if DEBUG_CAPTURE
    clock_t start = clock();
    double elapsed_to_last = 0;
    if (mLastCallTime != 0)
    {
        elapsed_to_last = (double)(start - mLastCallTime)/CLOCKS_PER_SEC;
    }
    mLastCallTime = start;
#endif

    if (NULL == mCapture
        || NULL == mImageSHM)
    {
        return;
    }

    surround_images_t* sources = mCapture->popOneFrame();
    if (NULL != sources)
    {
#if DEBUG_CAPTURE
        clock_t start = clock();
#endif
        mImageSHM->writeAllSources(sources);
#if DEBUG_CAPTURE
        std::cout << "AllSourcesSHMWriteWorker run: " 
                << " thread id:" << getTID()
                << ", elapsed to last time:" << elapsed_to_last
                << ", elapsed to capture:" << (double)(Util::get_system_milliseconds() - sources->timestamp)/1000
                << ", runtime:" << (double)(clock()-start)/CLOCKS_PER_SEC
                << std::endl;
#endif
    }
}

int main (int argc, char **argv)
{
    ICapture* capture = new CaptureImpl();
    unsigned int channel[VIDEO_CHANNEL_SIZE] = {4,2,3,5};
    struct cap_sink_t sink[VIDEO_CHANNEL_SIZE];
    struct cap_src_t source[VIDEO_CHANNEL_SIZE];
    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        sink[i].pixfmt = V4L2_PIX_FMT_UYVY;
        sink[i].width = CAPTURE_VIDEO_RES_X;
        sink[i].height = CAPTURE_VIDEO_RES_Y;
        sink[i].size = sink[i].width*sink[i].height*2;
        sink[i].crop_x = 0;
        sink[i].crop_y = 0;
        sink[i].crop_w = CAPTURE_VIDEO_RES_X;
        sink[i].crop_h = CAPTURE_VIDEO_RES_Y;

        source[i].pixfmt = V4L2_PIX_FMT_UYVY;
        source[i].width = CAPTURE_VIDEO_RES_X;
        source[i].height = CAPTURE_VIDEO_RES_Y;
        source[i].size = source[i].width*source[i].height*2;
    }

    capture->setCapCapacity(sink, source, VIDEO_CHANNEL_SIZE);
    int focusChannelIndex = VIDEO_CHANNEL_FRONT;
    struct cap_src_t focusSource;
    focusSource.pixfmt = sink[0].pixfmt;
    focusSource.width = sink[0].width;
    focusSource.height = sink[0].height;
    focusSource.size = sink[0].size;
    capture->setFocusSource(focusChannelIndex, &focusSource);
    capture->openDevice(channel, VIDEO_CHANNEL_SIZE);
    capture->start(VIDEO_FPS_15);

    //focus source
    FocusSourceSHMWriteWorker* focusSourceSHMWriteWorker = new FocusSourceSHMWriteWorker(capture);
    focusSourceSHMWriteWorker->start(VIDEO_FPS_15);

    //all source
    AllSourcesSHMWriteWorker* allSourcesSHMWriteWorker = new AllSourcesSHMWriteWorker(capture);
    allSourcesSHMWriteWorker->start(VIDEO_FPS_15);

    // touch screen event
    int eventFd = open("/dev/input/event0", O_RDONLY);
    struct input_event event[64] = {0};

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(eventFd, &fds);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    while (true)
    {
        //touch screen to switch focus channel
        if (eventFd > 0)
        {
            int r = select (eventFd + 1, &fds, NULL, NULL, &tv);
            if (-1 == r) {
                usleep(100);
            }

            if (0 == r) {
                usleep(100);
            }

            int rd = read(eventFd, event, sizeof(struct input_event) * 64);
            if (rd >= (int) sizeof(struct input_event))
            {
                for (int i = 0; i < rd / sizeof(struct input_event); i++)
                {
                    if (event[i].type == EV_KEY
                            && event[i].code == BTN_TOUCH
                            && event[i].value == 0)
                    {
                        focusChannelIndex++;
                        if (focusChannelIndex >= VIDEO_CHANNEL_SIZE)
                        {
                            focusChannelIndex = VIDEO_CHANNEL_FRONT;
                        }
                        capture->setFocusSource(focusChannelIndex, &focusSource);
                        break;
                    }
                }
            }
        }

        usleep(100);
    }

    focusSourceSHMWriteWorker->stop();
    delete focusSourceSHMWriteWorker;
    focusSourceSHMWriteWorker = NULL;

    allSourcesSHMWriteWorker->stop();
    delete allSourcesSHMWriteWorker;
    allSourcesSHMWriteWorker = NULL;

    capture->closeDevice();
    capture->stop();
    delete capture;
    capture = NULL;

    return 0;
}
