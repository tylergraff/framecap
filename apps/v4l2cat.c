// v4l2cat
// Reads one or more frames from one or more v4l2 devices and writes them to
// stdout
// Author: Tyler Graff, 2018
// tagraff@gmail.com
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "../common/v4l2cap.h"


void usage()
{
  fprintf(stderr, ""
"v4l2cat: Read one or more frames from one or more v4l2 devices and write    \n"
"the raw frame data to stdout                                                \n"
"                                                                            \n"
"Usage:                                                                      \n"
" v4l2cat [opts] <device1> [<device2> ...]                                   \n"
"  Capture one or more frames from v4l2 device(s) <device> and write the raw \n"
"  frame data to stdout. If more than 1 device is specified, frames are      \n"
"  read from each device sequentually.                                       \n"
"                                                                            \n"
"  Default options are: -c -1 -n 0                                           \n"
"                                                                            \n"
"Option:          Description:                                               \n"
"                                                                            \n"
"  -t [int]       Output [t] Total frames and then exit.                     \n"
"                 -1 outputs forever                                         \n"
"                                                                            \n"
"  -d [int]       After a frame is captured on a device, Discard the next [d]\n"
"                 frames from that device.                                   \n"
"                                                                            \n"
"  -e [int]       Capture [e] frames from Each device before moving to the   \n"
"                 next device.                                               \n"
"                                                                            \n"
}









// Callback for LibFrameCap, processes frames & saves them according to
// command line options
static int
on_frame(void* ctx, unsigned char* frame, int len, int w, int h, int fmt)
{
  struct timespec current_time, poll_time;
  FrameCap*       fc;
  ImgBuf          img_buf;
  time_t          ltime;
  unsigned char*  rgb;
  size_t          elapsed_nsec;
  char            tmpname[255];
  int             fp, npix, banner_y;

  // Save off references for homogenous use later
  fc = (FrameCap*) ctx;
  img_buf.buf = frame;
  img_buf.len = len;
  img_buf.fre = 0;


  // Check & enforce rate limit (-r option)
  for(;;)
  {
    clock_gettime(CLOCK_MONOTONIC_RAW, &current_time);

    elapsed_nsec = 1000000000*(current_time.tv_sec - fc->start_time.tv_sec) +
                    (current_time.tv_nsec - fc->start_time.tv_nsec);

    // Enforce rate limit
    if((elapsed_nsec / 1000000) > fc->rate_ms)
     break;

    // 10 milliseconds
    poll_time.tv_sec = 0;
    poll_time.tv_nsec = 10000000;
    nanosleep(&poll_time, NULL);
  }

  clock_gettime(CLOCK_MONOTONIC_RAW, &(fc->start_time));

  // keep track of frames and subsample if selected
  if(0 != fc->framecount % fc->subsamp)
  {
    fc->framecount++;
    return 0;
  }

  // if frame is in YUYV format, add banner if selected
  banner_y = 0;
  if(fc->banner && fmt == V4L2_PIX_FMT_YUYV)
  {
    tg_yuyv_putstr(img_buf.buf, w, h, fc->banner, 0, banner_y);
    banner_y += 8;
  }

  // if frame is in YUYV format, add timestamp if selected
  if(fc->tstamp && fmt == V4L2_PIX_FMT_YUYV)
  {
    ltime  = time(NULL);
    tg_yuyv_putstr(img_buf.buf,w,h,asctime(localtime(&ltime)), 0, banner_y);
  }

  // Write RAW frame to stdout if selected
  if(fc->stdoutp)
    if(write(STDOUT_FILENO, img_buf.buf, img_buf.len))
      fsync(STDOUT_FILENO);

  // If frame is in YUYV format, convert to JPEG if selected
  if(fc->jpeg)
  {
    if(fmt == V4L2_PIX_FMT_YUYV)
    {
      npix = w * h;
      rgb = (unsigned char*) malloc(npix * 3);
      yuyv_to_rgb(rgb, img_buf.buf, npix);

      // This referece will be now used to hold resulting jpeg image
      img_buf.buf = NULL;
      img_buf.len = 0;
      img_buf.fre = 0;

      // jpeg_handler() callback mallocs fc->frame and copies jpeg to it
      tje_encode_with_func(jpeg_handler, &img_buf, fc->jpeg, w, h, 3, rgb);
      free(rgb);
    }
    else
    { fprintf(stderr, "Cannot convert frame format to JPEG\n"); return -1; }
  }

  // Write frame to temp file, then atomically rename to output file
  // if selected
  if(fc->outfile)
  {
    sprintf(tmpname, "%s.tmp", fc->outfile);
    fp = open(tmpname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fp < 0)
    {
      fprintf(stderr, "Error opening: %s.tmp\n", fc->outfile);
      return -1;
    }
    if(write(fp, img_buf.buf, img_buf.len))
      fsync(fp);
    close(fp);
    if(-1 == rename(tmpname, fc->outfile))
    {
      fprintf(stderr, "Error renaming: %s.tmp to %s\n",fc->outfile,fc->outfile);
      return -1;
    }
  }

  // Write frame to sequence file if selected
  if(fc->seqfile)
  {
    // fc->framecount counts *before* optional subsample
    sprintf(tmpname, "%s-%06zu", fc->seqfile, fc->framecount/fc->subsamp);
    fp = open(tmpname, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fp < 0)
    {
      fprintf(stderr, "Error opening: %s\n", tmpname);
      return -1;
    }
    if(write(fp, img_buf.buf, img_buf.len))
      fsync(fp);
    close(fp);
  }

  // free image buffer if it was allocated by jpeg compressor
  if(img_buf.fre)
    free(img_buf.buf);

  // Only capture up to framecap->count frames
  // unless framecap->count is 0, in which case capture forever
  // fc->framecount counts *before* optional subsample
  fc->framecount++;
  if(fc->count > 0 && fc->count == fc->framecount/fc->subsamp)
    return -1;

  return 0;
}



static void bail(const char *msg) {
  fprintf(stderr, "%s\n\n", msg);
  usage();
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  V4L2Cap  *ctx = NULL;
  uint8_t  *frame;
  int       len;

  int       devcnt, opt, ii, r;
  int       total   = -1;
  int       each    = 1;
  int       discard = 0;

  opterr = 0;

  // Parse command-line options
  while((opt = getopt(argc, argv, "t:d:e:")) != -1)
  {
    switch (opt) {

    // Total number of frames to capture
    case 't':
      total = atoi(optarg);
      if(total < -2)
        bail("t < -2");
      break;

    // Discard n frames after each captured frame
    case 'd':
      discard = atoi(optarg);
      if(discard < 0)
        bail("d < 0");
      break;

    // Capture n frames per device before moving to next device
    case 'e':
      each = atoi(optarg);
      if(each < 0)
        bail("e < 0");
      break;

    default:
      bail("");
    }
  }

  devcnt = argc - optind;
  if(devcnt < 1)
    bail("No Devices Specified");

  // Open all devices
  ctx = malloc(devcnt * sizeof(ctx));
  for (ii = 0; ii < devcnt; ii++) {
    ctx[ii] = v4l2cap_new(argv[optind + ii], 2);
    if (!ctx[ii]) {
      fprintf(stderr, "Error opening: %s\n", argv[optind + ii]);
      free(ctx);
      exit(EXIT_FAILURE);
    }
  }

  // Get the next captured frame and its meta-data.
  frame = v4l2cap_next(ctx[0], &len, NULL, NULL, NULL);

  // Write it to STDOUT
  write(stdout, frame, len);

  // Tell the kernel we're done with this frame
  v4l2cap_done(ctx[0], frame);


  // Close all devices
  for (ii = 0; ii < devcnt; ii++)
    v4l2cap_free(ctx[ii]);

  free(ctx);
  return 0;
}
