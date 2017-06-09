/*
 * Copyright 2009-2012 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

/*
 * The code contained herein is licensed under the GNU Lesser General
 * Public License.  You may obtain a copy of the GNU Lesser General
 * Public License Version 2.1 or later at the following locations:
 *
 * http://www.opensource.org/licenses/lgpl-license.html
 * http://www.gnu.org/copyleft/lgpl.html
 */

/*!
 * @file mxc_ipudev_test.c
 *
 * @brief IPU device lib test implementation
 *
 * @ingroup IPU
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <string.h>

static void stitch_cl_init(const std::string configPath,
        cv::Mat& lutFront,
        cv::Mat& lutRear,
        cv::Mat& lutLeft,
        cv::Mat& lutRight,
    	cv::Mat& mask,
    	cv::Mat& weight)
{
#if DEBUG_STITCH
    std::cout << "System Initialization:" << configPath << std::endl;
#endif
	cv::FileStorage fs(configPath, cv::FileStorage::READ);
	fs["map1"] >> lutFront;
	fs["map2"] >> lutRear;
	fs["map3"] >> lutLeft;
	fs["map4"] >> lutRight;

	fs["mask"] >> mask;
	fs["weight"] >> weight;
	fs.release();

    return;
}

static int writeInt(char* dataName, int count_per_line, cv::Mat& data)
{
    char filename[256] = {0};
    char header[256] = {0};
    sprintf(filename, "%s.cpp", dataName);
    sprintf(header, "int %s_%dx%d[%d*%d*%d] = {\n\t", 
            dataName, data.cols, data.rows, data.cols, data.rows, data.channels());

    FILE* fd = fopen(filename, "at+");
    if (fd < 0)
    {
        printf("faild to open %s\n", filename);
        return -1;
    }

    printf("start write %s\n", filename);
    fwrite(header, 1, strlen(header), fd);
    int count = 0;
    for (int i = 0; i < data.rows; ++i)
    {
        for (int j = 0; j < data.cols; ++j)
        {
            int value = data.ptr<float>(i)[j];
            char byte[32] = {0};
            count++;
            if (count%count_per_line == 0)
            {
                sprintf(byte, "%d,\n\t", value);
            }
            else
            {
                sprintf(byte, "%d,", value);
            }
            fwrite(byte, 1, strlen(byte), fd);
        }
    }

    char tail[8] = {"\n};\n"};
    fwrite(tail, 1, strlen(tail), fd);
    fclose(fd);
    printf("end write %s\n", filename);
    return 0;
}

static int writeFloat(char* dataName, int count_per_line, cv::Mat& data)
{
    char filename[256] = {0};
    char header[256] = {0};
    sprintf(filename, "%s.cpp", dataName);
    sprintf(header, "float %s_%dx%d[%d*%d*%d] = {\n\t", 
            dataName, data.cols, data.rows, data.cols, data.rows, data.channels());

    FILE* fd = fopen(filename, "at+");
    if (fd < 0)
    {
        printf("faild to open %s\n", filename);
        return -1;
    }

    printf("start write %s\n", filename);
    fwrite(header, 1, strlen(header), fd);
    int count = 0;
    for (int i = 0; i < data.rows; ++i)
    {
        for (int j = 0; j < data.cols; ++j)
        {
            float value = data.ptr<float>(i)[j];
            char byte[32] = {0};
            count++;
            if (count%count_per_line == 0)
            {
                sprintf(byte, "%0.2f,\n\t", value);
            }
            else
            {
                sprintf(byte, "%0.2f,", value);
            }
            fwrite(byte, 1, strlen(byte), fd);
        }
    }

    char tail[8] = {"\n};\n"};
    fwrite(tail, 1, strlen(tail), fd);
    fclose(fd);
    printf("end write %s\n", filename);
    return 0;
}

static int writeUChar(char* dataName, int count_per_line, cv::Mat& data)
{
    char filename[256] = {0};
    char header[256] = {0};
    sprintf(filename, "%s.cpp", dataName);
    sprintf(header, "unsigned char %s_%dx%d[%d*%d*%d] = {\n\t", 
            dataName, data.cols, data.rows, data.cols, data.rows, data.channels());

    FILE* fd = fopen(filename, "at+");
    if (fd < 0)
    {
        printf("faild to open %s\n", filename);
        return -1;
    }

    printf("start write %s\n", filename);
    fwrite(header, 1, strlen(header), fd);
    int count = 0;
    for (int i = 0; i < data.rows; ++i)
    {
        for (int j = 0; j < data.cols; ++j)
        {
            uchar value = data.ptr<uchar>(i)[j];
            char byte[16] = {0};
            count++;
            if (count%count_per_line == 0)
            {
                sprintf(byte, "%d,\n\t", value);
            }
            else
            {
                sprintf(byte, "%d,", value);
            }
            fwrite(byte, 1, strlen(byte), fd);
        }
    }

    char tail[8] = {"\n};\n"};
    fwrite(tail, 1, strlen(tail), fd);
    fclose(fd);
    printf("end write %s\n", filename);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("\n usage: generate_cl_table calibrated_file count_per_line) \n");
        return -1;
    }

    cv::Mat lutFront;
    cv::Mat lutRear;
    cv::Mat lutLeft;
    cv::Mat lutRight;
    cv::Mat mask;
    cv::Mat weight;
    stitch_cl_init(argv[1], lutFront, lutRear, lutLeft, lutRight, mask, weight);

    int count_per_line = atoi(argv[2]);

    writeInt("lookuptab_front", count_per_line, lutFront);
    writeInt("lookuptab_rear", count_per_line, lutRear);
    writeInt("lookuptab_left", count_per_line, lutLeft);
    writeInt("lookuptab_right", count_per_line, lutRight);
    writeUChar("lookuptab_mask", count_per_line, mask);
    writeFloat("lookuptab_weight", count_per_line, weight);

    return 0;
}