#ifndef CAPTURE4IMPL_H
#define CAPTURE4IMPL_H

#include <QObject>
#include <QTimer>
#include <QThread>

#include "ICapture.h"
#include "capture4workerbase.h"

class Capture4Worker;
class Capture4Impl : public QObject, public ICapture
{
    Q_OBJECT
public:
    explicit Capture4Impl(QObject *parent = 0);
    virtual ~Capture4Impl();

    virtual int openDevice();
    virtual int closeDevice();
    virtual int start(int fps);
    virtual int stop();
    virtual surround_images_t* popOneFrame();
signals:

public slots:

private:
    QTimer mVideoCaptureTimer;
    Capture4WorkerBase *mCaptureWorker;
    QThread mCaptureThread;
    int mFPS;
};

#endif // CAPTURE4IMPL_H
