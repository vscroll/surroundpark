#include "renderimpl.h"
#include "renderpanoworker.h"
#include "rendersideworker.h"

RenderImpl::RenderImpl()
{
    mSideWorker = new RenderSideWorker();
    mPanoWorker = new RenderPanoWorker();
}

RenderImpl::~RenderImpl()
{
}

void RenderImpl::setCaptureModule(ICapture* capture)
{
    if (NULL != mSideWorker)
    {
        mSideWorker->setCaptureModule(capture);
    }
}

void RenderImpl::setSideImageRect(
        unsigned int left,
		unsigned int top,
		unsigned int width,
		unsigned int height)
{
    if (NULL != mSideWorker)
    {
        mSideWorker->setSideImageRect(left, top, width, height);
    }
}

void RenderImpl::setChannelMarkRect(
        unsigned int left,
		unsigned int top,
		unsigned int width,
		unsigned int height)
{
    if (NULL != mSideWorker)
    {
        mSideWorker->setChannelMarkRect(left, top, width, height);
    }
}

void RenderImpl::setPanoImageModule(IPanoImage* panoImage)
{
    if (NULL != mPanoWorker)
    {
        mPanoWorker->setPanoImageModule(panoImage);
    }
}

void RenderImpl::setPanoImageRect(
        unsigned int left,
		unsigned int top,
		unsigned int width,
		unsigned int height)
{
    if (NULL != mPanoWorker)
    {
        mPanoWorker->setPanoImageRect(left, top, width, height);
    }
}

int RenderImpl::start(unsigned int fps)
{
    if (NULL == mSideWorker
        || NULL == mPanoWorker)
    {
        return -1;
    }

    unsigned int sideLeft;
    unsigned int sideTop;
    unsigned int sideWidth;
    unsigned int sideHeight;
    mSideWorker->getSideImageRect(&sideLeft, &sideTop, &sideWidth, &sideHeight);
    if (mSideWorker->openDevice(sideLeft, sideTop, sideWidth, sideHeight) < 0)
    {
        mSideWorker->closeDevice();
    }
    mSideWorker->start(1000/fps);

    unsigned int panoLeft;
    unsigned int panoTop;
    unsigned int panoWidth;
    unsigned int panoHeight;
    mPanoWorker->getPanoImageRect(&panoLeft, &panoTop, &panoWidth, &panoHeight);
    if (mPanoWorker->openDevice(panoLeft, panoTop, panoWidth, panoHeight) < 0)
    {
        mPanoWorker->closeDevice();
    }
    mPanoWorker->start(1000/fps);

    return 0;
}

void RenderImpl::stop()
{
    if (NULL != mSideWorker)
    {
        mSideWorker->stop();
        mSideWorker->closeDevice();
    }

    if (NULL != mPanoWorker)
    {
        mPanoWorker->stop();
        mPanoWorker->closeDevice();
    }
}
