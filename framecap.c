// framecap.c
// Capture and save v4l2 camera frames to multiple locations
// Author: Tyler Graff, 2017
// tyler@graff.com
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
#define _POSIX_C_SOURCE  199309L // needed for C99

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "framecap.h"
#include "lib/libframecap.h"
#include "lib/tg_yuyv.h"

#include <time.h>

#define TJE_IMPLEMENTATION
#include "lib/tiny_jpeg.h"


void usage()
{
  fprintf(stderr, "framecap: Capture v4l2 device frames                      \n"
"                                                                            \n"
"Usage:                                                                      \n"
" framecap [opts] <device> [file]                                            \n"
"  Capture one or more frames from v4l2 device <device>,                     \n"
"  write them to [file] (if specified), using options specified in [opts]    \n"
"                                                                            \n"
"  Default options are: -c 0 -n 1                                            \n"
"                                                                            \n"
"Option:          Description:                                               \n"
"  -b <str>       Print banner text <str> to top-left of frame (YUYV only)   \n"
"                                                                            \n"
"  -c [n]         Output [n] frames and then exit. Use -c 0 to output forever\n"
"                                                                            \n"
"  -f <file>      _Atomically_ write frames to <file> by first writing them  \n"
"                 to <file>.tmp, and the renaming that to <file>. This allows\n"
"                 other programs to concurrently read <file> safely          \n"
"                                                                            \n"
"  -j [1-3]       Compress frames into JPEG format at quality 1 (lowest) to  \n"
"                 3 (highest/largest file). (YUYV only)                      \n"
"                                                                            \n"
"  -n [n]         Sub-sample by capturing only 1 of every [n] frames provided\n"
"                 by the v4l2 device. n=1 outputs every frame                \n"
"                                                                            \n"
"  -o             Also write each output frame to STDOUT                     \n"
"                                                                            \n"
"  -r <n>         Capture (at most) 1 frame every <n> milliseconds. This also\n"
"                 prevents the device from writing to its framebuffer during \n"
"                 the delay period, which may help decrease bus bandwidth.   \n"
"                                                                            \n"
"  -s <file>      Also write each output frame to <f>-<d> where <d> is a     \n"
"                 sequential decimal integer incremented each frame          \n"
"                                                                            \n"
"  -t             Print date/time to top-left of frame (YUYV only)           \n"
"                                                                            \n"
"Copyright 2017 Tyler Graff                                                  \n"
"tyler@graff.com                                                           \n");
}

// callback for tiny_jpeg that receives "chunks" of jpeg data and copies them
// to a contiguous buffer
static void
jpeg_handler(void* ctx, void* data, int size)
{
  ImgBuf*        jb = (ImgBuf*) ctx;
  unsigned char* tmp;

    // Allocate a new buffer to hold the next jpeg file chunk
    tmp = (unsigned char*) malloc(jb->len + size);
    if(!tmp)
      return;

    memcpy(tmp, jb->buf, jb->len); // copy previous buffer
    memcpy(&(tmp[jb->len]), data, size); // copy new chunk
    if(jb->buf)
      free(jb->buf);
    jb->buf = tmp;
    jb->len = jb->len + size;
    jb->fre = 1; // flag for freeing after use
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


int main(int argc, char **argv)
{
  FrameCap  framecap;
  int       opt, r;

  opterr = 0;
  memset(&framecap, 0, sizeof(framecap));

  // Command-line defaults
  framecap.banner  = NULL;
  framecap.seqfile = NULL;
  framecap.outfile = NULL;

  framecap.subsamp = 1; // sub-sampling modulus
  framecap.count   = 0; // number of frames to capture, 0 for forever
  framecap.jpeg    = 0; // default to RAW format
  framecap.tstamp  = 0;
  framecap.stdoutp = 0;
  framecap.rate_ms = 0;

  // Internal state
  framecap.framecount = 0;

  // Parse command-line options
  while((opt = getopt(argc, argv, "b:c:f:j:m:n:or:s:t")) != -1)
  {
    switch (opt) {

    // Print a custom banner at top-left of frame
    case 'b':
      framecap.banner = optarg;
      break;

    // Number of frames to capture
    case 'c':
      framecap.count = atoi(optarg);
      if(framecap.count < 0)
        framecap.count = 0;
      break;

    // Write frames to output file
    case 'f':
      framecap.outfile = optarg;
      break;

    // Compress YUYV frame to jpeg
    case 'j':
      framecap.jpeg = atoi(optarg);
      if(framecap.jpeg < 0)
        framecap.jpeg = 1;
      if(framecap.jpeg > 3)
        framecap.jpeg = 3;
      break;

    // Capture only one every <n> frames
    case 'n':
      framecap.subsamp = atoi(optarg);
      if(framecap.subsamp < 1)
        framecap.subsamp = 1;
      break;

    // Output raw frame (plus optional banner and timestamp) to STDOUT
    case 'o':
      framecap.stdoutp = 1;
      break;

    // Write frames to a sequence of files like: %zu-<name>
    case 's':
      framecap.seqfile = optarg;
      break;

    // Frame period in milliseconds
    case 'r':
      framecap.rate_ms = atoi(optarg);
      if(framecap.rate_ms < 0)
        framecap.rate_ms = 0;

    // Print timestamp at top-left of frame
    case 't':
      framecap.tstamp = 1;
      break;

    default:
      usage();
      abort();
      exit(-1);
    }
  }

  if(argc - optind == 1)
    framecap.v4l2    = argv[argc - 1];
  else
  {
    usage();
    exit(-1);
  }

  if(framecap.outfile && 235 < strlen(framecap.outfile))
  {
    fprintf(stderr, "Error: filename too long: %s\n", framecap.outfile);
    return -1;
  }

  if(framecap.seqfile && 235 < strlen(framecap.seqfile))
  {
    fprintf(stderr, "Error: filename too long: %s\n", framecap.seqfile);
    return -1;
  }

  // set initial timestamp before capture starts
  clock_gettime(CLOCK_MONOTONIC_RAW, &framecap.start_time);

  // Enter frame capture loop here
  r = lfc_capture(framecap.v4l2, &framecap, on_frame);
  if(r < 0)
    fprintf(stderr, "Errors occurred during capture loop!\n");

  fprintf(stderr, "\n");
  return r;
}
