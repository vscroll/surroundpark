#include "glshaderyuv.h"
#include "ICapture.h"
#include "util.h"
#include <iostream>
#include <string.h>

#define TEST 0

#if TEST
#if 1
//this data from official G2D demo
extern unsigned char yuyv_352x288[];
#else
//this data converting from ipu maybe have some error. cannot work well.
extern unsigned char Front_uyvy_352x288[];
#endif
#endif

static const char* gVideoSamplerVar[][GLShaderYUV::YUV_CHN_NUM] = {
    "s_frontY", "s_frontU", "s_frontV",
    "s_rearY", "s_rearU", "s_rearV",
    "s_leftY", "s_leftU", "s_leftV",
    "s_rightY", "s_rightU", "s_rightV"};

static const char* gFocusVideoSamplerVar[GLShaderYUV::YUV_CHN_NUM] = {
    "s_focusY", "s_focusU", "s_focusV"};

#define STRINGIFY(A)  #A
#include "panorama2d.vert"
#include "panorama2d.frag"

GLShaderYUV::GLShaderYUV(ICapture* capture)
{
    mCapture = capture;

    mLastCallTime = 0;
}

GLShaderYUV::~GLShaderYUV()
{

}

const char* GLShaderYUV::getVertShader()
{
    return gVShaderStr;
}

const char* GLShaderYUV::getFragShader()
{
    return gFShaderStr;
}

int GLShaderYUV::initConfig()
{
    char procPath[1024] = {0};
    if (Util::getAbsolutePath(procPath, 1024) < 0)
    {
        return -1;
    }

    char algoCfgPathName[1024] = {0};
    sprintf(algoCfgPathName, "%sFishToPanoYUV.xml", procPath);
	cv::FileStorage fs(algoCfgPathName, cv::FileStorage::READ);
	fs["map1"] >> mLookupTab[VIDEO_CHANNEL_FRONT];
	fs["map2"] >> mLookupTab[VIDEO_CHANNEL_REAR];
	fs["map3"] >> mLookupTab[VIDEO_CHANNEL_LEFT];
	fs["map4"] >> mLookupTab[VIDEO_CHANNEL_RIGHT];

	fs["mask"] >> mMask;
	fs["weight"] >> mWeight;
	fs.release();

    std::cout << "GLShaderYUV::initConfig"
            << ", lookupTab:" << mLookupTab[0].cols << "x" << mLookupTab[0].rows << " type:" << mLookupTab[0].type()
            << std::endl;
    std::cout << "GLShaderYUV::initConfig"
            << ", mask:" << mMask.cols << "x" << mMask.rows << " type:" << mMask.type()
            << std::endl;
    std::cout << "GLShaderYUV::initConfig"
            << ", weight:" << mWeight.cols << "x" << mWeight.rows << " type:" << mWeight.type()
            << std::endl;

    return 0;
}

void GLShaderYUV::initVertex()
{
    // Get the attribute locations
    mUserData.positionLoc = glGetAttribLocation(mProgramObject, "a_position");
    mUserData.texCoordLoc = glGetAttribLocation(mProgramObject, "a_texCoord");
}

void GLShaderYUV::initTexture()
{
    GLint lookupTabFront = glGetUniformLocation (mProgramObject, "lookupTabFront");
    GLint lookupTabRear = glGetUniformLocation (mProgramObject, "lookupTabRear");
    GLint lookupTabLeft = glGetUniformLocation (mProgramObject, "lookupTabLeft");
    GLint lookupTabRight = glGetUniformLocation (mProgramObject, "lookupTabRight");
    GLint mask = glGetUniformLocation (mProgramObject, "mask");
    GLint weight = glGetUniformLocation (mProgramObject, "weight");

    int size;
    float* array;
    size = mLookupTab[VIDEO_CHANNEL_FRONT].rows * mLookupTab[VIDEO_CHANNEL_FRONT].cols;
    array = (float*)(mLookupTab[VIDEO_CHANNEL_FRONT].data);
    glUniform1fv(lookupTabFront, size, array);

    size = mLookupTab[VIDEO_CHANNEL_REAR].rows * mLookupTab[VIDEO_CHANNEL_REAR].cols;
    array = (float*)(mLookupTab[VIDEO_CHANNEL_REAR].data);
    glUniform1fv(lookupTabRear, size, array);

    size = mLookupTab[VIDEO_CHANNEL_LEFT].rows * mLookupTab[VIDEO_CHANNEL_LEFT].cols;
    array = (float*)(mLookupTab[VIDEO_CHANNEL_LEFT].data);
    glUniform1fv(lookupTabLeft, size, array);

    size = mLookupTab[VIDEO_CHANNEL_RIGHT].rows * mLookupTab[VIDEO_CHANNEL_RIGHT].cols;
    array = (float*)(mLookupTab[VIDEO_CHANNEL_RIGHT].data);
    glUniform1fv(lookupTabRight, size, array);

    size = mMask.rows * mMask.cols;
    glUniform1iv(mask, size, (int*)mMask.data);

    size = mWeight.rows * mWeight.cols;
    glUniform1fv(weight, size, (float*)mWeight.data);
    checkGlError("initTexture");

    // Get the sampler location
    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        for (int j = 0; j < YUV_CHN_NUM; ++j)
        {
            mUserData.videoSamplerLoc[i][j] = glGetUniformLocation(mProgramObject, gVideoSamplerVar[i][j]);
        }
    }

    for (int j = 0; j < YUV_CHN_NUM; ++j)
    {
        mUserData.focusVideoSamplerLoc[j] = glGetUniformLocation(mProgramObject, gFocusVideoSamplerVar[j]);
    }

    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        for (int j = 0; j < YUV_CHN_NUM; ++j)
        {
            glGenTextures(1, &mUserData.videoTexId[i][j]);
        }
    }


    for (int j = 0; j < YUV_CHN_NUM; ++j)
    {
        glGenTextures(1, &mUserData.focusVideoTexId[j]);
    }

    checkGlError("initTexture");

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
}

void GLShaderYUV::draw()
{
    while (true)
    {
#if TEST
#if 1
        unsigned char* buffer = yuyv_352x288;
        int width = 352;
        int height = 288;
#else
        unsigned char* buffer = Front_uyvy_352x288;
        int width = 352;
        int height = 288;
#endif
        unsigned char y[width*height] = {0};
        unsigned char u[width/2*height] = {0};
        unsigned char v[width/2*height] = {0};
#if 1
        Util::yuyv_to_yuv(width, height, buffer, y, u, v);
#else
        Util::uyvy_to_yuv(width, height, buffer, y, u, v);
#endif
        loadTexture(mUserData.videoTexId[VIDEO_CHANNEL_FRONT][0], y, width, height);
        loadTexture(mUserData.videoTexId[VIDEO_CHANNEL_FRONT][1], u, width/2, height);
        loadTexture(mUserData.videoTexId[VIDEO_CHANNEL_FRONT][2], v, width/2, height);

        glDraw();
#else
        drawOnce();
#endif
    }
}

void GLShaderYUV::drawOnce()
{
#if DEBUG_STITCH
    clock_t start0 = clock();
    double elapsed_to_last = 0;
    if (mLastCallTime != 0)
    {
        elapsed_to_last = (double)(start0 - mLastCallTime)/CLOCKS_PER_SEC;
    }
    mLastCallTime = start0;
#endif

    if (NULL == mCapture)
    {
        return;
    }

#if DEBUG_STITCH
    clock_t start1 = clock();
#endif

    long elapsed = 0;
    surround_images_t* surroundImage = mCapture->popOneFrame();
    if (NULL != surroundImage)
    {
        elapsed = Util::get_system_milliseconds() - surroundImage->timestamp;
        if (elapsed < 400)
        {
            // bind the textures
            unsigned char* buffer;
            int width;
            int height;

            for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
            {
                buffer = (unsigned char*)(surroundImage->frame[i].data);
                width = surroundImage->frame[i].info.width;
                height = surroundImage->frame[i].info.height;
                unsigned char y[width*height] = {0};
                unsigned char u[width/2*height] = {0};
                unsigned char v[width/2*height] = {0};
                Util::yuyv_to_yuv(width, height, buffer, y, u, v);
                loadTexture(mUserData.videoTexId[i][0], y, width, height);
                loadTexture(mUserData.videoTexId[i][1], u, width/2, height);
                loadTexture(mUserData.videoTexId[i][2], v, width/2, height);
            }
        }

        delete surroundImage;
        surroundImage = NULL;
    }

    surround_image_t* sideImage = mCapture->popOneFrame4FocusSource();
    mFocusChannelIndex = mCapture->getFocusChannelIndex();
    if (NULL != sideImage)
    {
        unsigned char* buffer = (unsigned char*)(sideImage->data);
        int width = sideImage->info.width;
        int height = sideImage->info.height;
        unsigned char y[width*height] = {0};
        unsigned char u[width/2*height] = {0};
        unsigned char v[width/2*height] = {0};
        Util::yuyv_to_yuv(width, height, buffer, y, u, v);
        loadTexture(mUserData.focusVideoTexId[0], y, width, height);
        loadTexture(mUserData.focusVideoTexId[1], u, width/2, height);
        loadTexture(mUserData.focusVideoTexId[2], v, width/2, height);

        delete sideImage;
        sideImage = NULL;
    }

    glDraw();

#if DEBUG_STITCH
    clock_t start2 = clock();
#endif

#if DEBUG_STITCH

    std::cout << "GLShaderYUV::drawOnce"
            << ", elapsed to last time:" << elapsed_to_last
            << ", elapsed to capture:" << (double)elapsed/1000
            << ", render:" << (double)(start2-start1)/CLOCKS_PER_SEC
            << std::endl;
#endif

}

void GLShaderYUV::glDraw()
{
    static GLfloat squareVertices[] = {  
        -1.0f, -1.0f,  
        1.0f, -1.0f,  
        -1.0f,  1.0f,  
        1.0f,  1.0f,  
    };  
  
    static GLfloat coordVertices[] = {  
        0.0f, 1.0f,  
        1.0f, 1.0f,  
        0.0f,  0.0f,  
        1.0f,  0.0f,  
    };
      
    // Set the viewport
    glViewport(0, 0, mESContext.width, mESContext.height);
   
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Load the vertex position
    glVertexAttribPointer ( mUserData.positionLoc, 2, GL_FLOAT, 
                           GL_FALSE, 2 * sizeof(GLfloat), squareVertices );
    // Load the texture coordinate
    glVertexAttribPointer ( mUserData.texCoordLoc, 2, GL_FLOAT,
                           GL_FALSE, 2 * sizeof(GLfloat), coordVertices );

    glEnableVertexAttribArray(mUserData.positionLoc);
    glEnableVertexAttribArray(mUserData.texCoordLoc);

    //Front
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_FRONT][0]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_FRONT][0], 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_FRONT][1]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_FRONT][1], 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_FRONT][2]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_FRONT][2], 2);

    //Rear
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_REAR][0]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_REAR][0], 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_REAR][1]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_REAR][1], 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_REAR][2]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_REAR][2], 5);

    //Left
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_LEFT][0]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_LEFT][0], 6);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_LEFT][1]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_LEFT][1], 7);

    glActiveTexture(GL_TEXTURE8);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_LEFT][2]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_LEFT][2], 8);

    //right
    glActiveTexture(GL_TEXTURE9);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_RIGHT][0]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_RIGHT][0], 9);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_RIGHT][1]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_RIGHT][1], 10);

    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_2D, mUserData.videoTexId[VIDEO_CHANNEL_RIGHT][2]);
    glUniform1i(mUserData.videoSamplerLoc[VIDEO_CHANNEL_RIGHT][2], 11);

    //focus
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, mUserData.focusVideoTexId[0]);
    glUniform1i(mUserData.focusVideoSamplerLoc[0], 12);

    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_2D, mUserData.focusVideoTexId[1]);
    glUniform1i(mUserData.focusVideoSamplerLoc[1], 13);

    glActiveTexture(GL_TEXTURE14);
    glBindTexture(GL_TEXTURE_2D, mUserData.focusVideoTexId[2]);
    glUniform1i(mUserData.focusVideoSamplerLoc[2], 14);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    eglSwapBuffers(mESContext.eglDisplay, mESContext.eglSurface);
}

void GLShaderYUV::shutdown()
{
    // Delete texture object
    for (int i = 0; i < VIDEO_CHANNEL_SIZE; ++i)
    {
        for (int j = 0; j < YUV_CHN_NUM; ++j)
        {
            glDeleteTextures(1, &mUserData.videoTexId[i][j]);
        }
    }

    for (int j = 0; j < YUV_CHN_NUM; ++j)
    {
        glDeleteTextures(1, &mUserData.focusVideoTexId[j]);
    }

    GLShader::shutdown();
}

GLboolean GLShaderYUV::loadTexture(GLuint textureId, unsigned char *buffer, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    checkGlError("loadTexture");

    return TRUE;
}

