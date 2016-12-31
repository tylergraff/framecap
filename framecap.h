// Copyright Tyler Graff 2016
// tyler@prolaag.com
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>       /* getopt_long() */

#include <fcntl.h>        /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define MAXFRAME (4096*2160*3) // 4K max frame size in bytes

#define CLEAR(x) memset(&(x), 0, sizeof(x))

/* H264 with start codes */
#ifndef V4L2_PIX_FMT_H264
#define V4L2_PIX_FMT_H264     v4l2_fourcc('H', '2', '6', '4')
#endif

enum io_method {
  IO_METHOD_READ,
  IO_METHOD_MMAP,
  IO_METHOD_USERPTR,
};

struct buffer {
  void   *start;
  size_t  length;
};



