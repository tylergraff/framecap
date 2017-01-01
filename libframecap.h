// ---------------------------------------------------------------------------
// LibFrameCap
// Copyright 2016 Tyler Graff
// tagraff@gmail.com
//
// LibFrameCap provides a simple way to capture frames from a v4l2 device.
// Configuration such as framerate and resolution is not supported; the capture
// device must be configured using a separate utility (like v4l2-util) before
// using this library.
//
// To use LibFrameCap:
// 1.) #include this file ("libframecap.h")
//
// 2.) Define a function to handle frames as they are captured. Prototype is:
//     int your_frame_handler(void* usr, char* frame, int len)
//      <usr>   is a pointer to a struct that holds your state
//      <frame> is whatever frame data was returned by the camera
//      <len>   is the length of the frame in bytes
//
// 3.) Calling the following function will result in your_frame_handler() being
//     called every time a frame is captured by the device:
//     lfc_capture(char* fname, void* usr, OnFrame cb)
//      <fname> is the v4l2 device name
//      <usr>   is a pointer to your state struct
//      <cb>    is a pointer to your frame-handler function
//
//     lfc_capture() will return when in the following circumstances:
//       a.) Your frame-handler returns a nonzero value
//       b.) An error occurs
//     lfc_capture() will only return a nonzero value if an error occurred.
//
// LibFrameCap is single-threaded, so your frame handler does not need to
// be threadsafe.
//
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

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

static int lfc_ioctl(int fd, int request, void *arg)
{
  int r;
  do {
    r = ioctl(fd, request, arg);
  } while (-1 == r && EINTR == errno);

  return r;
}

static int lfc_exit(LibFrameCap* lfc, char* error)
{
  enum v4l2_buf_type type;
  int                ii;

  if(error)
    fprintf(stderr, "%s\n", error);

  // Stop capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  lfc_ioctl(lfc->fd, VIDIOC_STREAMOFF, &type);

  // un-mmap() buffers
  for (ii = 0; ii < 2; ii++)
  {
    munmap(lfc->fbuf[ii].start, lfc->fbuf[ii].len);
    lfc->fbuf[ii].start = NULL;
  }
  // Close v4l2 device
  close(lfc->fd);

  return -1;
}

// Initialize libframecap with a user-supplied function pointer to call on
// every frame and a user context pointer to pass in to that function
// Start capture loop. Returns when error occurs, or user-callback returns != 0
static int lfc_capture(LibFrameCap* lfc, char* fname, void* usr, OnFrame cb)
{
  struct v4l2_capability     cap;
  struct v4l2_cropcap        cropcap;
  struct v4l2_crop           crop;
  struct v4l2_format         fmt;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer         buf;
  enum   v4l2_buf_type       type;
  struct timeval             timeout;
  fd_set                     fds;
  int                        ii, min, r;

  lfc->fd = 0;
  lfc->fbuf[0].start = NULL;

  lfc->fd = open(fname, O_RDWR | O_NONBLOCK, 0);
  if(lfc->fd < 0)
    return lfc_exit(lfc, "Error: Cannot open device");

  // Determine if fd is a V4L2 Device
  if(0 != lfc_ioctl(lfc->fd, VIDIOC_QUERYCAP, &cap))
    return lfc_exit(lfc, "Error: device is not v4l2 compatible");

  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    return lfc_exit(lfc, "Error: Device does not support video capture");

  if(!(cap.capabilities & V4L2_CAP_STREAMING))
    return lfc_exit(lfc, "Error: Device does not support streaming IO");

  // Set crop, ignore ioctl errors
  memset(&cropcap, 0, sizeof(cropcap));
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  lfc_ioctl(lfc->fd, VIDIOC_CROPCAP, &cropcap);
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  crop.c    = cropcap.defrect; // reset to default
  lfc_ioctl(lfc->fd, VIDIOC_S_CROP, &crop);

  // Preserve original settings as set by v4l2-ctl for example
  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == lfc_ioctl(lfc->fd, VIDIOC_G_FMT, &fmt))
    return lfc_exit(lfc, "Error: VIDIOC_G_FMT");

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
  if(-1 == lfc_ioctl(lfc->fd, VIDIOC_REQBUFS, &req))
    return lfc_exit(lfc, "Error: %s does not support mmap");

  // Error if we didn't get 2 buffers
  if(req.count < 2)
    return lfc_exit(lfc, "Error: Insufficient device buffer memory");

  // mmap() the buffers into userspace memory
  for (ii = 0 ; ii < req.count; ii++)
  {
    memset(&buf, 0, sizeof(buf));
    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory  = V4L2_MEMORY_MMAP;
    buf.index   = ii;
    if(-1 == lfc_ioctl(lfc->fd, VIDIOC_QUERYBUF, &buf))
      return lfc_exit(lfc, "Error: VIDIOC_QUERYBUF");

    lfc->fbuf[ii].len = buf.length;
    lfc->fbuf[ii].start = mmap(NULL, buf.length,
      PROT_READ | PROT_WRITE, MAP_SHARED, lfc->fd, buf.m.offset);
    if(MAP_FAILED == lfc->fbuf[ii].start)
      return lfc_exit(lfc, "Error: Failed to map device frame buffers");
  }

  // Set up buffers
  for (ii = 0; ii < 2; ii++)
  {
    memset(&buf, 0, sizeof(buf));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = ii;

    if (-1 == lfc_ioctl(lfc->fd, VIDIOC_QBUF, &buf))
      return lfc_exit(lfc, "Error: VIDIOC_QBUF");
  }

  // Start capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == lfc_ioctl(lfc->fd, VIDIOC_STREAMON, &type))
    return lfc_exit(lfc, "Error: VIDIOC_STREAMON");

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
      return lfc_exit(lfc, "Error: select returned error");
    }
    if (0 == r)
    {
      fprintf(stderr, "Warning: Timeout (60s) waiting for frame\n");
      continue;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == lfc_ioctl(lfc->fd, VIDIOC_DQBUF, &buf))
    {
      if(EAGAIN == errno)
        continue;
      return lfc_exit(lfc, "Error: VIDIOC_DQBUF");
    }

    if(buf.index > 2)
      return lfc_exit(lfc, "Error: driver buffer index out of bounds");

    // user callback returns nonzero to break capture loop
    if(cb)
      if(cb(usr, lfc->fbuf[buf.index].start, buf.bytesused))
        break;

    // Set up for next frame
    if (-1 == lfc_ioctl(lfc->fd, VIDIOC_QBUF, &buf))
      return lfc_exit(lfc, "Error: VIDIOC_QBUF");
  }

  lfc_exit(lfc, NULL); // simply close everything
  return 0;
}
