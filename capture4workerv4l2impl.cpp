#include "capture4workerv4l2impl.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <fcntl.h>
#include <errno.h>
#include "util.h"

Capture4WorkerV4l2Impl::Capture4WorkerV4l2Impl(QObject *parent, int videoChannelNum) :
    Capture4WorkerBase(parent, videoChannelNum)
{

    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        mWidth[i] = 704;
        mHeight[i] = 576;
        mVideoFd[i] = -1;
        mV4l2Buf[i] = NULL;
    }
}

void Capture4WorkerV4l2Impl::openDevice()
{
    for (int i = 0; i < mVideoChannelNum; ++i)
    {
        char devName[16] = {0};
        sprintf(devName, "/dev/video%d", i);
        mVideoFd[i] = open(devName, O_RDWR/* | O_NONBLOCK*/);
        if (mVideoFd[i] <= 0)
        {
            return;
        }

        V4l2::getVideoCap(mVideoFd[i]);
        V4l2::setVideoFmt(mVideoFd[i], mWidth[i], mHeight[i]);
        V4l2::getVideoFmt(mVideoFd[i], &mWidth[i], &mHeight[i]);
        V4l2::setFps(mVideoFd[i], 15);
        V4l2::getFps(mVideoFd[i]);
        if (-1 == V4l2::initV4l2Buf(mVideoFd[i], &mV4l2Buf[i]))
        {
            return;
        }

        if (-1 == V4l2::startCapture(mVideoFd[i]))
        {
            return;
        }
    }
}

void Capture4WorkerV4l2Impl::closeDevice()
{
    for (int i = 0; i < mVideoChannelNum; ++i)
    {
        if (mVideoFd[i] > 0)
        {
            V4l2::stoptCapture(mVideoFd[i]);

            close(mVideoFd[i]);
            mVideoFd[i] = -1;
        }
    }
}

void Capture4WorkerV4l2Impl::onCapture()
{
    double start = (double)clock();
    int size = 0;
    int elapsed = 0;
    int convert_time = 0;
    if (qAbs(mLastTimestamp) > 0.00001f)
    {
        elapsed = (int)(start - mLastTimestamp)/1000;
    }
    mLastTimestamp = start;

    IplImage *pIplImage[VIDEO_CHANNEL_SIZE] = {NULL};
    unsigned char flag = 1;
    double timestamp = (double)clock();
    for (int i = 0; i < mVideoChannelNum; ++i)
    {
        if (mVideoFd[i] == -1)
        {
            return;
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(mVideoFd[i], &fds);

        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        int r = select (mVideoFd[i] + 1, &fds, NULL, NULL, &tv);
        if (-1 == r) {
            if (EINTR == errno)
                return;
        }

        if (0 == r) {
            qDebug() << "Capture1WorkerV4l2Impl::onCapture"
                     << " select timeout";
            return;
        }

        int imageSize = mWidth[i]*mHeight[i]*3;
        struct v4l2_buffer buf;
        if (-1 != V4l2::readFrame(mVideoFd[i], &buf))
        {
            if (buf.index < V4l2::V4L2_BUF_COUNT)
            {
                unsigned char frame_buffer[imageSize];
                mMutexCapture.lock();
                unsigned char* buffer = (unsigned char*)(mV4l2Buf[i][buf.index].start);
#if DEBUG
                double convert_start = (double)clock();
#endif
                Util::yuyv_to_rgb24(mWidth[i], mHeight[i], buffer, frame_buffer);
                mMutexCapture.unlock();

#if DEBUG
                convert_time = (int)(clock() - convert_start)/1000;
                qDebug() << "Capture4WorkerV4l2Impl::onCapture"
                         << ", channel:" << i
                         << ", yuv to rgb:" << convert_time;
#endif

                pIplImage[i] = cvCreateImage(cvSize(mWidth[i], mHeight[i]), IPL_DEPTH_8U, 3);
                if (NULL != pIplImage)
                {
                    memcpy(pIplImage[i]->imageData, frame_buffer, imageSize);
                    //write2File(pIplImage[i]);
                    //cvReleaseImage(&pIplImage[i]);

                    flag = flag << 1;
                }
            }
        }

        V4l2::v4l2QBuf(mVideoFd[i], &buf);
    }

    //integrity
    if (flag == (1 << mVideoChannelNum))
    {
        surround_image4_t* surroundImage = new surround_image4_t();
        surroundImage->timestamp = timestamp;
        for (int i = 0; i < mVideoChannelNum; ++i)
        {
            surroundImage->image[i] = pIplImage[i];
        }

        mMutexQueue.lock();
        mSurroundImageQueue.append(surroundImage);
        size = mSurroundImageQueue.size();
        mMutexQueue.unlock();
    }
    else
    {
        for (int i = 0; i < mVideoChannelNum; ++i)
        {
            if (NULL != pIplImage[i])
            {
                cvReleaseImage(&pIplImage[i]);
            }
        }
    }

#if DEBUG
    qDebug() << "Capture4WorkerV4l2Impl::onCapture"
             << ", channel:" << mVideoChannelNum
             << ", flag:" << flag
             << ", size:" << size
             << ", elapsed to last time:" << elapsed
             << ", yuv to rgb:" << convert_time
             << ", capture:" << (int)(clock()-timestamp)/1000;
#endif
}
