#ifndef COMMON_H
#define COMMON_H

#include <linux/videodev2.h>

#if DEBUG
#define DEBUG_CAPTURE 1
#define DEBUG_STITCH 1
#define DEBUG_UPDATE 1
#endif

#define V4L2_BUF_COUNT 1

#define CAPTURE_VIDEO_RES_X 704
#define CAPTURE_VIDEO_RES_Y 574

#define CAPTURE_VIDEO_RES_FRONT_X   CAPTURE_VIDEO_RES_X
#define CAPTURE_VIDEO_RES_FRONT_Y   CAPTURE_VIDEO_RES_Y
#define CAPTURE_VIDEO_RES_REAR_X    CAPTURE_VIDEO_RES_X
#define CAPTURE_VIDEO_RES_REAR_Y    CAPTURE_VIDEO_RES_Y
#define CAPTURE_VIDEO_RES_LEFT_X    CAPTURE_VIDEO_RES_X
#define CAPTURE_VIDEO_RES_LEFT_Y    CAPTURE_VIDEO_RES_Y
#define CAPTURE_VIDEO_RES_RIGHT_X   CAPTURE_VIDEO_RES_X
#define CAPTURE_VIDEO_RES_RIGHT_Y   CAPTURE_VIDEO_RES_Y

#define RENDER_VIDEO_RES_PANO_X     424
#define RENDER_VIDEO_RES_PANO_Y     600

#define VIDEO_CHANNEL_FRONT	0
#define VIDEO_CHANNEL_REAR	1
#define VIDEO_CHANNEL_LEFT 	2
#define VIDEO_CHANNEL_RIGHT 3
#define VIDEO_CHANNEL_SIZE 	4

#define VIDEO_FPS_5 		5
#define VIDEO_FPS_10		10
#define VIDEO_FPS_15		15
#define VIDEO_FPS_20		20
#define VIDEO_FPS_25		25
#define VIDEO_FPS_30        30

#define OVERSTOCK_SIZE 		3
#define STAT_PERIOD_SECONDS	5*60	

typedef struct cap_sink_t {
  unsigned int pixfmt;
  unsigned int width;
  unsigned int height;
  unsigned int size;
  unsigned int crop_x;
  unsigned int crop_y;
  unsigned int crop_w;
  unsigned int crop_h;
} cap_sink_t;

typedef struct cap_src_t {
  unsigned int pixfmt;
  unsigned int width;
  unsigned int height;
  unsigned int size;
} cap_src_t;

typedef struct frame_info_t {
    unsigned int width;
    unsigned int height;
    unsigned int pixfmt;
    unsigned int size;
} frame_info_t;

typedef struct surround_image_t {
    long timestamp;
    frame_info_t info;
    void* data;
} surround_image_t;

typedef struct surround_images_t {
    long timestamp;
    surround_image_t frame[VIDEO_CHANNEL_SIZE];
} surround_images_t;

#endif // COMMON_H
