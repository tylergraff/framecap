// yuyv2imgblk
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "../common/util.h"

static void usage(void) {
  fprintf(stderr,
"Usage:                                                                      \n"
" yuyv2imgblk -h <px_height> -w <px_width> <imgblk_file>                     \n"
"                                                                            \n"
"Option:          Description:                                               \n"
"  -h [int]       Input image height in pixels                               \n"
"  -w [int]       Input image width in pixels                                \n"
"                                                                            \n");
}

static void bail(const char *msg) {
  fprintf(stderr, "\nERROR: %s\n\n", msg);
  usage();
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
  int       opt;
  uint8_t  *yuyv, *imgblk;
  uint32_t  npix, h = 720, w = 1280;
  size_t    len;

  // Parse command-line options
  opterr = 0;
  while((opt = getopt(argc, argv, "h:w:")) != -1) {
    switch (opt) {

    case 'h':
      h = strtoul(optarg, NULL, 0);
      if (h < 1)
        bail("-h must be greater than 0");
      break;

    case 'w':
      w = strtoul(optarg, NULL, 0);
      if (w < 1)
        bail("-w must be greater than 0");
      break;

    default:
      bail("Unknown argument");
    }
  }

  if ((argc - optind) != 1)
    bail("Must specify exactly one output file");

  npix = h*w;

  // read an entire yuyv frame
  yuyv = file_read("/dev/stdin", &len);
  if (len != (2*npix))
      bail("Incorrect input length");

  // convert to ImgBlk
  imgblk = yuyv2imgblk(yuyv, w, h);
  free(yuyv);

  file_write_atomic(argv[argc-1], imgblk, len);
  free(imgblk);
  return 0;
}
