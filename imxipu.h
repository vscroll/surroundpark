#ifndef IMXIPU_H
#define IMXIPU_H

struct ipu_task;
class IMXIPU
{
public:
    IMXIPU();

    struct buffer
    {
        void* start;
        unsigned int offset;
        unsigned int length;
        unsigned int width;
        unsigned int height;
        unsigned int pixfmt;
    };

    static int allocIPUBuf(int fd, struct buffer* ipu_buf,  unsigned int frame_size);
    static int freeIPUBuf(int fd, struct buffer* ipu_buf);
    static int IPUTask(int fd, struct ipu_task* task);
};

#endif // IMXIPU_H
