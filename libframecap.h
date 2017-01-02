// ---------------------------------------------------------------------------
// LibFrameCap
// Copyright 2016 Tyler Graff
// tagraff@gmail.com
//
// LibFrameCap: A header-only, no-dynamic-allocation library that provides a
// simple API to capture frames from a v4l2 device. Configuration such as
// framerate and resolution is NOT supported; the capture device must be
// configured using a separate utility (like v4l2-util) beforehand.
//
// To use LibFrameCap:
// 1.) #include this file ("libframecap.h")
//
// 2.) Define a function to handle frames as they are captured. Prototype is:
//     int your_handler(void* usr, char* frame, int len, int w_pix, int h_pix)
//      <usr>   is a pointer to a struct that holds your state
//      <frame> is whatever frame data was returned by the camera
//      <len>   is the length of the frame in bytes
//      <w_pix> is the number of pixels the frame is wide
//      <h_pix> is the number of pixels the frame is high
// 3.) Calling the following function will result in your_handler() being
//     called every time a frame is captured by the device:
//     lfc_capture(char* fname, void* usr, LFC_FrameHandler fh)
//      <fname> is the v4l2 device name
//      <usr>   is a pointer to your state struct
//      <fh>    is a pointer to your frame-handler function
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
#ifndef LFC_GUARD
#define LFC_GUARD

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

// Number of memory-mapped framebuffers to use. Minimum is 1. 2 or more allows
// v4l2 driver to buffer frames while a frame is being processed by your
// handler. Increasing this number will let the driver store more frames at a
// time, but this will not affect steady-state frame throughput.
#define LFC_FBUFS (2)

// Control printing of LibFrameCap's error messages to stderr.
// 0: do not print any messages
// 1: print all messages
#define LFC_VERBOSE (1)


// Frame Handler Function Prototype
typedef int (*LFC_FrameHandler)(void*,char*,int,int,int);


// Wrap ioctl() with an automatic retry on EINTR
static int lfc_ioctl(int fd, int req, void *arg)
{
  int r;
  while(r = ioctl(fd, req, arg), r == -1 && EINTR == errno);
  return r;
}

// Initialize libframecap with a user-supplied function pointer to call on
// every frame and a user context pointer to pass in to that function
// Start capture loop. Returns when error occurs, or user-callback returns != 0
static int lfc_capture(char* fname, void* usr, LFC_FrameHandler fh)
{
  struct v4l2_capability     cap;
  struct v4l2_cropcap        cropcap;
  struct v4l2_crop           crop;
  struct v4l2_format         fmt;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer         buf;
  enum   v4l2_buf_type       type;
  struct timeval timeout;
  void*  fbuf[LFC_FBUFS];  // frame buffer
  int    flen[LFC_FBUFS];  // frame length
  char*  errmsg;
  int    fd, ii, min, r, w_pix, h_pix;
  fd_set fds;

  fd = open(fname, O_RDWR | O_NONBLOCK, 0);
  if(fd < 0)
    {errmsg = "Error: Cannot open device"; goto lfc_exit;}

  // Determine if fd is a V4L2 Device
  if(0 != lfc_ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {errmsg = "Error: device is not v4l2 compatible"; goto lfc_exit;}


  if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {errmsg = "Error: Device does not support video capture"; goto lfc_exit;}

  if(!(cap.capabilities & V4L2_CAP_STREAMING))
   { errmsg = "Error: Device does not support streaming IO"; goto lfc_exit;}

  // Set crop, ignore ioctl errors
  cropcap = (struct v4l2_cropcap){0};
  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  lfc_ioctl(fd, VIDIOC_CROPCAP, &cropcap);
  crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  crop.c    = cropcap.defrect; // reset to default
  lfc_ioctl(fd, VIDIOC_S_CROP, &crop);

  // Preserve original settings as set by v4l2-ctl for example
  fmt = (struct v4l2_format){0};
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if(-1 == lfc_ioctl(fd, VIDIOC_G_FMT, &fmt))
   {errmsg = "Error: VIDIOC_G_FMT"; goto lfc_exit;}

  // Buggy driver paranoia
  min = fmt.fmt.pix.width * 2;
  if(fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if(fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  // Store off image resolution to pass to callback
  w_pix = fmt.fmt.pix.width;
  h_pix = fmt.fmt.pix.height;

  // Request memory-mapped buffers
  req = (struct v4l2_requestbuffers){0};
  req.count  = LFC_FBUFS;
  req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;
  if(-1 == lfc_ioctl(fd, VIDIOC_REQBUFS, &req))
    {errmsg = "Error: %s does not support mmap"; goto lfc_exit;}

  if(req.count != LFC_FBUFS)
    {errmsg = "Error: Device buffer count mismatch"; goto lfc_exit;}

  // mmap() the buffers into userspace memory
  for (ii = 0 ; ii < LFC_FBUFS; ii++)
  {
    buf = (struct v4l2_buffer){0};
    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory  = V4L2_MEMORY_MMAP;
    buf.index   = ii;
    if(-1 == lfc_ioctl(fd, VIDIOC_QUERYBUF, &buf))
      {errmsg = "Error: VIDIOC_QUERYBUF"; goto lfc_exit;}

    flen[ii] = buf.length;
    fbuf[ii] = mmap(NULL, buf.length,
      PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if(MAP_FAILED == fbuf[ii])
      {errmsg = "Error: Failed to map device frame buffers"; goto lfc_exit;}

    // Set up buffers
    buf = (struct v4l2_buffer){0};
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index  = ii;

    if (-1 == lfc_ioctl(fd, VIDIOC_QBUF, &buf))
      {errmsg = "Error: VIDIOC_QBUF"; goto lfc_exit;}
  }

  // Start capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (-1 == lfc_ioctl(fd, VIDIOC_STREAMON, &type))
    {errmsg = "Error: VIDIOC_STREAMON"; goto lfc_exit;}

  // Frame Capture Loop
  for (;;)
  {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeout.tv_sec  = 60;
    timeout.tv_usec = 0;

    r = select(fd + 1, &fds, NULL, NULL, &timeout);
    if (-1 == r)
    {
      if (EINTR == errno)
        continue;
      {errmsg = "Error: select returned error"; goto lfc_exit;}
    }
    if (0 == r && LFC_VERBOSE)
      fprintf(stderr, "Warning: Timeout (60s) waiting for frame\n");

    buf = (struct v4l2_buffer){0};
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == lfc_ioctl(fd, VIDIOC_DQBUF, &buf))
    {
      if(EAGAIN == errno)
        continue;
      {errmsg = "Error: VIDIOC_DQBUF"; goto lfc_exit;}
    }

    if(buf.index > LFC_FBUFS)
      {errmsg = "Error: driver buffer index out of bounds"; goto lfc_exit;}

    // user callback must return nonzero to break capture loop
    if(fh)
      if(fh(usr, fbuf[buf.index], buf.bytesused, w_pix, h_pix))
        break;

    // Set up for next frame
    if (-1 == lfc_ioctl(fd, VIDIOC_QBUF, &buf))
      {errmsg = "Error: VIDIOC_QBUF"; goto lfc_exit;}
  }

  errmsg = NULL;
  // falls through to lfc_exit
lfc_exit:
  if(errmsg && LFC_VERBOSE)
    fprintf(stderr, "%s\n", errmsg);

  // Stop capturing
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  lfc_ioctl(fd, VIDIOC_STREAMOFF, &type);

  // un-mmap() buffers
  for (ii = 0; ii < LFC_FBUFS; ii++)
    munmap(fbuf[ii], flen[ii]);

  // Close v4l2 device
  close(fd);

  if(errmsg)
    return -1;

  return 0;
}

#endif // LFC_GUARD
