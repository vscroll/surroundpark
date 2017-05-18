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
#include <linux/input.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "util.h"
#include "imxipu.h"
#include <linux/ipu.h>

#define USE_CPU 1

class SideImageSHMWriteWorker : public Thread
{
public:
    SideImageSHMWriteWorker(ICapture* capture);
    virtual ~SideImageSHMWriteWorker();

public:
    virtual void run();

private:
    ICapture* mCapture;
    ImageSHM* mImageSHM;
};

SideImageSHMWriteWorker::SideImageSHMWriteWorker(ICapture* capture)
{
    mCapture = capture;
    mImageSHM = new ImageSHM();
    mImageSHM->create((key_t)SHM_SIDE_ID, SHM_SIDE_SIZE);
}

SideImageSHMWriteWorker::~SideImageSHMWriteWorker()
{
    if (NULL != mImageSHM)
    {
        mImageSHM->destroy();
        delete mImageSHM;
        mImageSHM = NULL;
    }
}

void SideImageSHMWriteWorker::run()
{
    if (NULL == mCapture
        || NULL == mImageSHM)
    {
        return;
    }

    surround_image_t* sideImage = mCapture->popOneFrame4FocusSource();
    if (NULL != sideImage)
    {
        struct image_shm_header_t header = {};
        header.channel = mCapture->getFocusChannelIndex();
        header.width = sideImage->info.width;
        header.height = sideImage->info.height;
        header.pixfmt = sideImage->info.pixfmt;
        header.size = sideImage->info.size;
        header.timestamp = sideImage->timestamp;
        unsigned char* frame = (unsigned char*)sideImage->data;
#if 0
        clock_t start = clock();
#endif
        if (NULL != frame)
        {
            mImageSHM->writeImage(&header, frame, header.size);
        }
#if 0
        std::cout << "SideImageSHMWriteWorker run: " << (double)(clock()-start)/CLOCKS_PER_SEC
                << " channel:" << header.channel
                << " width:" << header.width
                << " height:" << header.height
                << " size:" << header.size
                << " timestamp:" << header.timestamp
                << std::endl;
#endif
    }
}

static void write2File(int channel, void* image)
{
    IplImage* frame = (IplImage*)image;
    char outImageName[256] = {0};
    IplImage* outImage = cvCreateImage(cvGetSize(frame),frame->depth,frame->nChannels);
    // 将原图拷贝过来
    cvCopy(frame,outImage,NULL);

    timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    tm now;
    localtime_r(&time.tv_sec, &now);

    //设置保存的图片名称和格式
    memset(outImageName, 0, sizeof(outImageName));
    sprintf(outImageName, "cam%d_%04d%02d%02d_%02d%02d%02d.jpg", channel,
            now.tm_year + 1900, now.tm_mon+1, now.tm_mday, 
            now.tm_hour, now.tm_min, now.tm_sec);
    //保存图片
    cvSaveImage(outImageName, outImage, 0);
}

int main (int argc, char **argv)
{
    ICapture* capture = new CaptureImpl();
    unsigned int channel[VIDEO_CHANNEL_SIZE] = {4,2,3,5};
    struct cap_sink_t sink[VIDEO_CHANNEL_SIZE];
    struct cap_src_t source[VIDEO_CHANNEL_SIZE];
    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        sink[i].pixfmt = PIX_FMT_UYVY;
        sink[i].width = 704;
        sink[i].height = 574;
        sink[i].size = sink[i].width*sink[i].height*2;
        sink[i].crop_x = 0;
        sink[i].crop_y = 0;
        sink[i].crop_w = 704;
        sink[i].crop_h = 574;

        source[i].pixfmt = PIX_FMT_UYVY;
        source[i].width = 704;
        source[i].height = 574;
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

    SideImageSHMWriteWorker* sideImageSHMWriteWorker = new SideImageSHMWriteWorker(capture);
    sideImageSHMWriteWorker->start(VIDEO_FPS_15);

    //IPU for color convert
    int ipuFd = open("/dev/mxc_ipu", O_RDWR, 0);
    struct IMXIPU::buffer outIPUBuf = {0};
    IMXIPU::allocIPUBuf(ipuFd, &outIPUBuf, 704*574*3);
    capture->enableCapture();

    // touch screen event
    int eventFd = open("/dev/input/event0", O_RDONLY);
    struct input_event event[64] = {0};

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(eventFd, &fds);

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    int x = -1;
    int y = -1;

    //side image region
    int side_left = 424;
    int side_top = 0;
    while (true)
    {
        //touch screen to switch focus channel
        if (eventFd > 0)
        {
            int r = select (eventFd + 1, &fds, NULL, NULL, &tv);
            if (-1 == r) {
                usleep(10);
            }

            if (0 == r) {
                usleep(10);
            }

            int rd = read(eventFd, event, sizeof(struct input_event) * 64);
            if (rd >= (int) sizeof(struct input_event))
            {
                for (int i = 0; i < rd / sizeof(struct input_event); i++)
                {
                    if (event[i].type == EV_ABS
                            && event[i].code == ABS_MT_POSITION_X)
                    {
                        x = event[i].value;
                    }

                    if (event[i].type == EV_ABS
                            && event[i].code == ABS_MT_POSITION_Y)
                    {
                        y = event[i].value;
                    }

                    if (event[i].type == EV_KEY
                            && event[i].code == BTN_TOUCH
                            && event[i].value == 0
                            && x >= side_left
                            && y >= side_top)
                    {
                        focusChannelIndex++;
                        if (focusChannelIndex >= VIDEO_CHANNEL_SIZE)
                        {
                            focusChannelIndex = VIDEO_CHANNEL_FRONT;
                        }
                        capture->setFocusSource(focusChannelIndex, &focusSource);
                        continue;
                    }

                    // capture for calibration
                    if (event[i].type == EV_KEY
                            && event[i].code == BTN_TOUCH
                            && event[i].value == 0
                            && x < side_left)
                    {
                        clock_t start = clock();

                        surround_image_t* sideImage = capture->captureOneFrame4FocusSource();
                        if (NULL == sideImage)
                        {
                            continue;
                        }
#if USE_CPU
                        unsigned char frame_buffer[sideImage->info.width*sideImage->info.height*3];
                        Util::uyvy_to_rgb24(sideImage->info.width, sideImage->info.height, (unsigned char*)(sideImage->data), frame_buffer);
                        cv::Mat image = cv::Mat(sideImage->info.height, sideImage->info.width, CV_8UC3, frame_buffer);
#else
                        struct ipu_task task;
                        memset(&task, 0, sizeof(struct ipu_task));
                        task.input.width  = sideImage->info.width;
                        task.input.height = sideImage->info.height;
                        task.input.crop.pos.x = 0;
                        task.input.crop.pos.y = 0;
                        task.input.crop.w = sideImage->info.width;
                        task.input.crop.h = sideImage->info.height;
                        task.input.format = V4L2_PIX_FMT_UYVY;
                        task.input.deinterlace.enable = 1;
                        task.input.deinterlace.motion = 2;

                        task.output.width = sideImage->info.width;
                        task.output.height = sideImage->info.height;
                        task.output.crop.pos.x = 0;
                        task.output.crop.pos.y = 0;
                        task.output.crop.w = sideImage->info.width;
                        task.output.crop.h = sideImage->info.height;
                        
                        ///for colour cast
                        task.output.format = V4L2_PIX_FMT_BGR24;

                        task.input.paddr = (int)sideImage->data;
                        task.output.paddr = (int)outIPUBuf.offset;
                        if (ioctl(ipuFd, IPU_QUEUE_TASK, &task) != IPU_CHECK_OK)
                        {
                            continue;
                        }
                        cv::Mat image = cv::Mat(sideImage->info.height, sideImage->info.width, CV_8UC3, outIPUBuf.start);
#endif
                        IplImage iplImage = IplImage(image);
                        write2File(focusChannelIndex, &iplImage);

#if USE_CPU
#else
                        IMXIPU::freeIPUBuf(ipuFd, &outIPUBuf);
#endif
                        std::cout << "calibration:" << (double)(clock() - start)/CLOCKS_PER_SEC
                                << " channel:" << focusChannelIndex
                                << " width:" << sideImage->info.width
                                << " height:" << sideImage->info.height
                                << " size:" << sideImage->info.size
                                << std::endl;
                    }
                }
            }
        }

        usleep(10);
    }

    sideImageSHMWriteWorker->stop();
    delete sideImageSHMWriteWorker;
    sideImageSHMWriteWorker = NULL;

    capture->closeDevice();
    capture->stop();
    delete capture;
    capture = NULL;

    return 0;
}