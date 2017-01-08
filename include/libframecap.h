// ---------------------------------------------------------------------------
// libframecap.h
// Author: Tyler Graff, 2017
// tyler@graff.com
//
// LibFrameCap: A header-only, no-dynamic-allocation library that provides a
// simple API to capture frames from a v4l2 device. LibFrameCap does NOT provide
// any method to configure device parameters such as framerate or resolution.
// The device must be configured using a separate utility (like v4l2-util)
// before LibFrameCap's capture loop is invoked.
//
// ---------------------------------------------------------------------------
//
// MIT License
// Copyright (c) Tyler Graff 2017
// tyler@graff.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

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

// ---------------------------------------------------------------------------
// To use LibFrameCap:
// 1.) #include this file ("libframecap.h")
//
// 2.) Define a function to handle frames as they are captured. Refer to
//     "Frame Handler Function Prototype" below.
//
// 3.) Call lfc_capture() to invoke your handler on every frame captured by the
//     capture device. Refer to "Capture Loop" below.
//
// LibFrameCap Public API:
//
// Number of memory-mapped framebuffers to use. Minimum is 1. 2 or more allows
// driver to buffer frames while your handler is processing the current frame.
#define LFC_FBUFS (2)

// Control printing of LibFrameCap's error messages to stderr.
// 0: do not print any messages, 1: print all messages
#define LFC_VERBOSE (1)

// Frame Handler Function Prototype:
//      <usr>     pointer to a struct that holds your state
//      <frame>   frame data as returned by the camera
//      <len>     length of the frame in bytes
//      <w_pix>   frame width in pixels
//      <h_pix>   frame height in pixels
//      <img_fmt> frame format, refer to V4L2_PIX_FMT_* in linux/videodev2.h
//      return value: <0>       capture loop continues
//                    <nonzero> capture loop stops and returns this value
typedef int (*LFC_FrameHandler) (void* usr,
                                 unsigned char* frame,
                                 int len,
                                 int w_pix,
                                 int h_pix,
                                 int img_fmt);
// Capture Loop:
//      <fname> v4l2 device filepath to open (eg, "/dev/video0")
//      <usr>   pointer to your user-allocated state
//      <fh>    pointer to your frame-handler function
//      return value: <-1> an internal error has occurred
//                    otherwise, returns any nonzero frame-handler return value
static int lfc_capture(char* fname,
                       void* usr,
                       LFC_FrameHandler fh);
// ---------------------------------------------------------------------------



// ioctl wrapper (internal use only)
static int lfc_ioctl(int, int, void*);

// Capture loop. Returns -1 if error occurs, or user-callback retval if != 0.
static int lfc_capture(char* fname, void* usr, LFC_FrameHandler fh)
{
  struct v4l2_capability     cap;
  struct v4l2_cropcap        cropcap;
  struct v4l2_crop           crop;
  struct v4l2_format         fmt;
  struct v4l2_requestbuffers req;
  struct v4l2_buffer         buf;
  enum   v4l2_buf_type       type;
  struct timeval             timeout;
  void*  fbuf[LFC_FBUFS];  // frame buffers
  int    flen[LFC_FBUFS];  // buffer lengths
  char*  errmsg;
  int    fd, ii, min, r, w_pix, h_pix, img_fmt;
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

  // Store off image parameters to pass to callback
  w_pix  = fmt.fmt.pix.width;
  h_pix  = fmt.fmt.pix.height;
  img_fmt = fmt.fmt.pix.pixelformat; // YUYV422, MJPEG, etc

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
      if(fh(usr, fbuf[buf.index], buf.bytesused, w_pix, h_pix, img_fmt))
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

// Wrap ioctl() to spin on EINTR
static int lfc_ioctl(int fd, int req, void* arg)
{
  int r;
  while(r = ioctl(fd, req, arg), r == -1 && EINTR == errno);
  return r;
}

#endif // LFC_GUARD
