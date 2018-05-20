// vcat
// Reads one or more frames from one or more v4l2 devices and writes them to
// stdout
//
// MIT License
// Copyright (c) Tyler Graff 2018
// tagraff@gmail.com
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
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "../common/v4l2cap.h"


static void usage(void) {
  fprintf(stderr,
"vcat: Read one or more frames from one or more v4l2 devices and write       \n"
"the raw frame data to stdout                                                \n"
"                                                                            \n"
"Usage:                                                                      \n"
" vcat [opts] <device1> [<device2> ...]                                      \n"
"  Capture one or more frames from v4l2 device(s) <device> and write the raw \n"
"  frame data to stdout. If more than 1 device is specified, frames are      \n"
"  read from each device sequentually.                                       \n"
"                                                                            \n"
"  Default options are: -t 0 -d 0 -e 1                                       \n"
"                                                                            \n"
"Option:          Description:                                               \n"
"                                                                            \n"
"  -t [int]       Output [t] Total frames and then exit.                     \n"
"                 0 outputs forever                                          \n"
"                                                                            \n"
"  -d [int]       After a frame is captured on a device, Discard the next [d]\n"
"                 frames from that device.                                   \n"
"                                                                            \n"
"  -e [int]       Capture [e] frames from Each device before moving to the   \n"
"                 next device.                                               \n"
"                                                                            \n");
}

static void bail(const char *msg) {
  fprintf(stderr, "\nERROR: %s\n\n", msg);
  usage();
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  V4L2Cap  **ctx = NULL;
  int       opt;
  uint8_t  *frame;
  uint32_t  len, devcnt;
  uint64_t  ii, total   = -1;
  uint64_t  jj, each    = 1;
  uint64_t  kk, discard = 0;

  opterr = 0;

  // Parse command-line options
  while((opt = getopt(argc, argv, "t:d:e:")) != -1)
  {
    switch (opt) {

    // Total number of frames to capture
    case 't':
      total = strtoul(optarg, NULL, 0);
      if (total < 1)
        total = -1;
      break;

    // Discard d frames after each captured frame
    case 'd':
      discard = strtoul(optarg, NULL, 0);
      break;

    // Capture e frames per device before moving to next device
    case 'e':
      each = strtoul(optarg, NULL, 0);
      if (each < 1)
        bail("-e must be greater than 0");
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

  // Set stdout pipe size
  fcntl(STDOUT_FILENO, F_SETPIPE_SZ, 4194304);

  // Capture <total> frames
  for (ii = 0; ii < total; ii++) {

    // Capture <each> frames on a device
    for (jj = 0; jj < each; jj++) {

      // throw away <discard> frames before capturing one
      for (kk = 0; kk < discard; kk++)
        v4l2cap_done(ctx[ii % devcnt],
                     v4l2cap_next(ctx[ii % devcnt], &len, NULL, NULL, NULL));

      frame = v4l2cap_next(ctx[ii % devcnt], &len, NULL, NULL, NULL);

      // Write it to STDOUT
      if (frame)
        write(STDOUT_FILENO, frame, len);

      v4l2cap_done(ctx[ii % devcnt], frame);
    }
  }

  // Close all devices
  for (ii = 0; ii < devcnt; ii++)
    v4l2cap_free(ctx[ii]);

  free(ctx);
  return 0;
}
