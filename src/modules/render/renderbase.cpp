#include "renderbase.h"
#include <string.h>
#include <iostream>
#include "common.h"

RenderBase::RenderBase()
{
    mRenderDevice = NULL;

    mRealFPS = 0;
    mStartStatTime = 0;
    mStatDuration = 0;
    mRealFrameCount = 0;
}

RenderBase::~RenderBase()
{

}

int RenderBase::openDevice(unsigned int dstLeft,
		unsigned int dstTop,
		unsigned int dstWidth,
		unsigned int dstHeight)
{
    if (NULL == mRenderDevice)
    {
        mRenderDevice = new RenderDevice(0, true);
    }

    if (mRenderDevice->openDevice(dstLeft, dstTop, dstWidth, dstHeight) < 0)
    {
	    return -1;
    }    

    return 0;
}

void RenderBase::closeDevice()
{
    if (NULL != mRenderDevice)
    {
        mRenderDevice->closeDevice();
        delete mRenderDevice;
        mRenderDevice = NULL;
    }
}

void RenderBase::drawImage(struct render_surface_t* surface, bool alpha)
{
    if (NULL != mRenderDevice)
    {
        mRenderDevice->drawImage(surface, alpha);
    }
}

void RenderBase::drawMultiImages(struct render_surface_t surfaces[], unsigned int num, bool alpha)
{
    if (NULL != mRenderDevice)
    {
        mRenderDevice->drawMultiImages(surfaces, num, alpha);
    }
}

unsigned int RenderBase::statFPS()
{
    if (mRealFrameCount == 0)
    {
        mStartStatTime = clock();
    }

    mRealFrameCount++;
    mStatDuration = (clock() - mStartStatTime)/CLOCKS_PER_SEC;
    if (mStatDuration < 1)
    {
        mStatDuration = 1;
    }

    if (mStatDuration > STAT_PERIOD_SECONDS)
    {
        mRealFrameCount = 0;
    }

    mRealFPS = mRealFrameCount/mStatDuration;
    return mRealFPS;
}
