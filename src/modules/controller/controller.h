#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "thread.h"

class ICapture;
class IPano;
class Controller : public Thread
{
public:
    Controller();
    virtual ~Controller();

    void initCaptureModule(unsigned int channel[], unsigned int channelNum,
        struct cap_sink_t sink[], struct cap_src_t source[]);

    void initPanoImageModule(unsigned int inWidth,
            unsigned int inHeight,
            unsigned int inPixfmt,
            unsigned int panoWidth,
            unsigned int panoHeight,
            unsigned int panoPixfmt,
            char* algoCfgFilePath,
            bool enableOpenCL);

    void initSideImageModule(unsigned int width, unsigned int height, unsigned int pixfmt);

    void uninitModules();
    void startModules(unsigned int fps);
    void stopModules();

    void startLoop(unsigned int freq);
public:
    virtual void run();
private:
    ICapture* mCapture;
    IPano* mPano;    
};

#endif // CONTROLLER_H
