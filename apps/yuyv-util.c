// yuyv2jpeg
// Reads one or more yuyv422-formatted frames from stdin and writes them to one
// or more JPEG files
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
//#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

#include "util.h"

static void usage(void) {
  fprintf(stderr,
"yuyv2jpeg: Read one or more YUYV422-formatted (2 bytes/pixel) frames from   \n"
"stdin and write them atomically to the specified JPEG file.                 \n"
"                                                                            \n"
"Usage:                                                                      \n"
" yuyv2jpeg -h <px_height> -w <px_width> <jpeg_file>                         \n"
"                                                                            \n"
"Option:          Description:                                               \n"
"                                                                            \n"
"  -h [int]       Input image height in pixels                               \n"
"                                                                            \n"
"  -w [int]       Input image width in pixels                                \n"
"                                                                            \n"
"  -q [1,2,3]     JPEG Filesize (1-smallest, 3-largest)                      \n"
"                                                                            \n"
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
  uint8_t  *yuyv, *rgb, *imgblk, *jpeg;
  uint32_t  npix, h = 720, w = 1280, q = 2;
  size_t    len;

  // Set stdin pipe size
  fcntl(STDIN_FILENO, F_SETPIPE_SZ, 4194304);

  // Parse command-line options
  opterr = 0;
  while((opt = getopt(argc, argv, "h:w:q:")) != -1) {
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

    case 'q':
      q = strtoul(optarg, NULL, 0);
      if (q < 1 || q > 3)
        bail("-q must be 1, 2, or 3");
      break;

    default:
      bail("Unknown argument");
    }
  }

  if ((argc - optind) < 1)
    bail("Must specify output file");

  npix = h*w;

  // RGB uses 3 bytes per pixel
  rgb = malloc(3*npix);
  if (!rgb)
    bail("Could not allocate memory!");

  // Convert forever
  for (;;) {

    // read an entire yuyv frame
    yuyv = file_read("/dev/stdin", &len);
    if (len != (2*npix))
      break;

    // convert to ImgBlk
    imgblk = yuyv2imgblk(yuyv, w, h);
    free(yuyv);

    file_write_atomic(argv[argc-1], imgblk, len);

    // convert back to YUYV
    yuyv = imgblk2yuyv(imgblk, w, h);
    free(imgblk);

    // Convert to RGB, then to JPEG
    yuyv422_to_rgb24(rgb, yuyv, npix);
    jpeg = rgb24_to_jpeg(rgb, w, h, q, &len);

    // Write to disk
    if (0 > file_write_atomic(argv[argc-2], jpeg, len))
      fprintf(stderr, "Error writing to file: %s\n", argv[argc-1]);

    free(jpeg);
    free(yuyv);
  }

  free(yuyv);
  free(rgb);
  return 0;
}
