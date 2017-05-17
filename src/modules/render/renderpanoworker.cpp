#include "renderpanoworker.h"
#include <string.h>
#include <iostream>
#include "common.h"
#include "imageshm.h"
#include "util.h"

RenderPanoWorker::RenderPanoWorker()
{
    mPanoImage = NULL;
}

RenderPanoWorker::~RenderPanoWorker()
{

}

void RenderPanoWorker::init(IPanoImage* panoImage)
{
    mPanoImage = panoImage;
    if (NULL == panoImage)
    {
        mPanoSHM = new ImageSHM();
        mPanoSHM->create((key_t)SHM_PANO_ID, SHM_PANO_SIZE);
    }
}

unsigned char gFakeData[SHM_PANO_DATA_SIZE] = {0};
void RenderPanoWorker::run()
{
#if DEBUG_UPDATE
    clock_t start = clock();
    double elapsed_to_last = 0;
    if (mLastCallTime != 0)
    {
        elapsed_to_last = (double)(start - mLastCallTime)/CLOCKS_PER_SEC;
    }
    mLastCallTime = start;
#endif

    surround_image_t* panoImage = NULL;
    if (NULL != mPanoImage)
    {
    }
    else
    {
        //one source from share memory
        if (NULL != mPanoSHM)
        {
#if 0
            unsigned char imageBuf[SHM_PANO_SIZE] = {};
            if (mPanoSHM->readImage(imageBuf, sizeof(imageBuf)) < 0)
            {
                return;
            }
            image_shm_header_t* header = (image_shm_header_t*)imageBuf;
            panoImage = new surround_image_t();
            panoImage->info.width = header->width;
            panoImage->info.height = header->height;
            panoImage->info.pixfmt = header->pixfmt;
            panoImage->info.size = header->size;
            panoImage->timestamp = header->timestamp;
            panoImage->data = imageBuf + sizeof(image_shm_header_t);
#else
            panoImage = new surround_image_t();
            panoImage->info.width = 424;
            panoImage->info.height = 600;
            panoImage->info.pixfmt = PIX_FMT_UYVY;
            panoImage->info.size = SHM_PANO_DATA_SIZE;
            panoImage->timestamp = Util::get_system_milliseconds();
            panoImage->data = gFakeData;
#endif
        }
    }

    if (NULL == panoImage
        || NULL == panoImage->data)
    {
        return;
    }

#if DEBUG_UPDATE
    clock_t start_draw = clock();
#endif

    drawImage((unsigned char*)panoImage->data,
            panoImage->info.pixfmt,
            panoImage->info.width,
            panoImage->info.height,
            panoImage->info.size);

#if DEBUG_UPDATE
    std::cout << "RenderPanoWorker::run"
            << " thread id:" << getTID()
            << ", elapsed to last time:" << elapsed_to_last
            << ", elapsed to capture:" << Util::get_system_milliseconds() - panoImage->timestamp
            << ", draw:" << (double)(clock() - start_draw)/CLOCKS_PER_SEC
            << " width:"  << panoImage->info.width
            << " height:" << panoImage->info.height
            << " size:" << panoImage->info.size
            << std::endl;
#endif

    delete panoImage;
}
