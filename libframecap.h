#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>        /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>




// ---------------------------------------------------------------------------
// LibFrameCap
// Wraps v4l2 and provides callback on each frame

typedef int (*OnFrame)(void*,char*,int);

struct buf {
  void*  start;
  size_t len;
};

typedef struct
{
  struct buf fbuf[2];
  OnFrame    on_frame;
  char*      fname;
  int        fd;
  int        isv4l2;
  void*      usr;
} LibFrameCap;

static int lfc_die(char* error)
{
  fprintf(stderr, "%s\n", error);
  return -1;
}

static int xioctl(int fd, int request, void *arg)
{
  int r;
  do {
    r = ioctl(fd, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}

// Clean up
static void lfc_close(LibFrameCap* lfc)
{
  enum v4l2_buf_type type;
  int                ii;

  // Stop capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(lfc->fd, VIDIOC_STREAMOFF, &type);

  // un-mmap() buffers
  for (ii = 0; ii < 2; ii++)
  {
    munmap(lfc->fbuf[ii].start, lfc->fbuf[ii].len);
    lfc->fbuf[ii].start = NULL;
  }
  // Close v4l2 device
  close(lfc->fd);
}


// Initialize libframecap with a user-supplied function pointer to call on
// every frame and a user context pointer to pass in to that function
static int lfc_open(LibFrameCap* lfc, char* fname)
{
  struct v4l2_capability     cap;
  struct v4l2_cropcap        cropcap;
  struct v4l2_crop           crop;
  struct v4l2_format         fmt;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer         buf;

  unsigned int               ii, min;

  lfc->fd = 0;
  lfc->fbuf[0].start = NULL;

  lfc->fd = open(fname, O_RDWR | O_NONBLOCK, 0);
  if(lfc->fd < 0)
    return lfc_die("Error: Cannot open device");

  // Determine if fd is a V4L2 Device
  if(0 != xioctl(lfc->fd, VIDIOC_QUERYCAP, &cap))
    return lfc_die("Error: device is not v4l2 compatible");

  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    return lfc_die("Error: Device does not support video capture");

  if(!(cap.capabilities & V4L2_CAP_STREAMING))
    return lfc_die("Error: Device does not support streaming IO");

  // Set crop, ignore ioctl errors
  memset(&cropcap, 0, sizeof(cropcap));
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  xioctl(lfc->fd, VIDIOC_CROPCAP, &cropcap);
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  crop.c    = cropcap.defrect; // reset to default
  xioctl(lfc->fd, VIDIOC_S_CROP, &crop);

  // Preserve original settings as set by v4l2-ctl for example
  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == xioctl(lfc->fd, VIDIOC_G_FMT, &fmt))
    return lfc_die("Error: VIDIOC_G_FMT");

  // Buggy driver paranoia
  min = fmt.fmt.pix.width * 2;
  if(fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if(fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  // Request 4 mmap'd buffers
  memset(&req, 0, sizeof(req));
  req.count  = 2;
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if(-1 == xioctl(lfc->fd, VIDIOC_REQBUFS, &req))
    return lfc_die("Error: %s does not support mmap");

  // Error if we didn't get 2 buffers
  if(req.count < 2)
    return lfc_die("Error: Insufficient device buffer memory");

  // mmap() the buffers into userspace memory
  for (ii = 0 ; ii < req.count; ii++)
  {
    memset(&buf, 0, sizeof(buf));
    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory  = V4L2_MEMORY_MMAP;
    buf.index   = ii;
    if(-1 == xioctl(lfc->fd, VIDIOC_QUERYBUF, &buf))
      return lfc_die("Error: VIDIOC_QUERYBUF");

    lfc->fbuf[ii].len = buf.length;
    lfc->fbuf[ii].start = mmap(NULL, buf.length,
      PROT_READ | PROT_WRITE, MAP_SHARED, lfc->fd, buf.m.offset);
    if(MAP_FAILED == lfc->fbuf[ii].start)
      return lfc_die("Error: Failed to map device frame buffers");
  }
  return 0;
}

// Start capture loop. Returns when <n> frames have been captured, error occurs,
// or user-callback returns < 0
// n=0 means capture forever
static int lfc_capture(LibFrameCap* lfc, void* usr, OnFrame on_frame)
{
  enum   v4l2_buf_type type;
  struct v4l2_buffer   buf;
  struct timeval       timeout;
  fd_set               fds;
  int                  ii, r;

  // Set up buffers
  for (ii = 0; ii < 2; ii++)
  {
    memset(&buf, 0, sizeof(buf));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = ii;

    if (-1 == xioctl(lfc->fd, VIDIOC_QBUF, &buf))
      return lfc_die("Error: VIDIOC_QBUF");
  }

  // Start capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == xioctl(lfc->fd, VIDIOC_STREAMON, &type))
    return lfc_die("Error: VIDIOC_STREAMON");

  for (;;)
  {
    FD_ZERO(&fds);
    FD_SET(lfc->fd, &fds);
    timeout.tv_sec  = 60;
    timeout.tv_usec = 0;

    r = select(lfc->fd + 1, &fds, NULL, NULL, &timeout);
    if (-1 == r)
    {
      if (EINTR == errno)
        continue;
      return lfc_die("Error: select returned error");
    }
    if (0 == r)
    {
      fprintf(stderr, "Warning: Timeout (60s) waiting for frame\n");
      continue;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(lfc->fd, VIDIOC_DQBUF, &buf))
    {
      if(EAGAIN == errno)
        continue;
      return lfc_die("Error: VIDIOC_DQBUF");
    }

    if(buf.index > 2)
      return lfc_die("Error: driver buffer index out of bounds");

    // user frame handler returns nonzero to break capture loop
    if(on_frame)
      if(on_frame(usr, lfc->fbuf[buf.index].start, buf.bytesused))
        break;

    // Set up for next frame
    if (-1 == xioctl(lfc->fd, VIDIOC_QBUF, &buf))
      return lfc_die("Error: VIDIOC_QBUF");
  }

  return 0;
}
