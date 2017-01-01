// framecap: Capture and save v4l2 camera frames to multiple locations
// Copyright Tyler Graff 2016
// tagraff@gmail.com
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include "framecap.h"
#include "libframecap.h"

// For JPEG encoding
#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"


// (Crudely) convert YUYV422 to RGB:
// npix is number of pixels in the image
void yuyv422_to_rgb(unsigned char* rgb, unsigned char* yuyv, int npix)
{
  int ii, jj, r, g, b, y1, y2, u, v;

  // every 8 bytes of YUYV422 represents 4 pixels
  // every 3 bytes of RGB represents 1 pixel
  // traverse YUYV422 array 4 bytes at a time and RGB array 6 bytes at a time,
  // and convert 2 pixels per iteration
  for (ii = 0, jj = 0; ii < npix*3; ii += 6, jj += 4)
  {
    y1 = (unsigned char)yuyv[jj];
    u  = (unsigned char)yuyv[jj+1];
    y2 = (unsigned char)yuyv[jj+2];
    v  = (unsigned char)yuyv[jj+3];

    r = fmax(0, fmin(255, 1.164*(y1 - 16) + 1.596*(u - 128)));
    g = fmax(0, fmin(255, 1.164*(y1 - 16) - 0.813*(u - 128) - 0.391*(v - 128)));
    b = fmax(0, fmin(255, 1.164*(y1 - 16) + 2.018*(u - 128)));

    rgb[ii]   = r;
    rgb[ii+1] = g;
    rgb[ii+2] = b;

    r = fmax(0, fmin(255, 1.164*(y2 - 16) + 1.596*(u - 128)));
    g = fmax(0, fmin(255, 1.164*(y2 - 16) - 0.813*(u - 128) - 0.391*(v - 128)));
    b = fmax(0, fmin(255, 1.164*(y2 - 16) + 2.018*(u - 128)));

    rgb[ii+3] = r;
    rgb[ii+4] = g;
    rgb[ii+5] = b;
  }
}


// Frame callback
int on_frame(void* ctx, char* frame, int len)
{
  FrameCap*      fc = (FrameCap*) ctx;
  static int     count = 0;
  unsigned char* rgb;
  char           fname[255];
  int            fp, npix;

  // Write out raw image to file
  sprintf(fname, "%s-%06d.raw", fc->outfile, count);
  printf("%s\n", fname);
  fp = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  write(fp, frame, len);
  fsync(fp);
  close(fp);


  // YUYV422 stores 4px in 8 bytes, rgb stores 1px in 3 bytes
  npix = len / 2;
  rgb = (unsigned char*) malloc(npix * 3);
  if(!rgb)
    return -1;

  yuyv422_to_rgb(rgb, (unsigned char*)frame, npix);

  // Convert raw RGB image to jpg & write to file
  sprintf(fname, "%s-%06d.jpg", fc->outfile, count);
  tje_encode_to_file_at_quality(fname, 3, 1280, 720, 3, rgb);

  count++;

  if(count == 1)
    return -1;

  return 0;
}

void usage()
{
  fprintf(stderr, "Usage: framecap [opts] <device> <fname>\n");
  fprintf(stdout, "\n");
}

int main(int argc, char **argv)
{
  FrameCap  framecap;
  int       opt;

  opterr = 0;
  memset(&framecap, 0, sizeof(framecap));

  // Parse command-line options
  while((opt = getopt(argc, argv, "bc:j:m:n:oOs:")) != -1)
  {
    switch (opt) {
    case 'b':
      framecap.banner = 1;
      printf("banner\n");
      break;

    case 'c':
      framecap.count = atoi(optarg);
      printf("count: %d\n", framecap.count);
      break;

    case 'j':
      framecap.jpeg = atoi(optarg);
      printf("jpeg quality: %d\n", framecap.jpeg);
      break;

    case 'm':
      framecap.motion = atoi(optarg);
      printf("motion percent: %d\n", framecap.motion);
      break;

    case 'n':
      framecap.subsamp = atoi(optarg);
      printf("sub-sample every: %d\n", framecap.subsamp);
      break;

    case 'o':
      printf("output to STDOUT\n");
      framecap.stdoutp = 1;
      break;

    case 'O':
      printf("output to STDOUT in RAW format\n");
      framecap.stdoutp_raw = 1;
      break;

    case 's':
      framecap.seqfile = optarg;
      printf("sequential output to: %s\n", framecap.seqfile);
      break;

    default:
      usage();
      abort();
      exit(-1);
    }
  }

  if(argc - optind != 2) {
    usage();
    exit(-1);
  }

  framecap.outfile  = argv[argc - 1];
  framecap.fname    = argv[argc - 2];
  fprintf(stdout, "v4l2 device:   %s\n", framecap.fname);
  fprintf(stdout, "output file:   %s\n", framecap.outfile);

  if(235 < strlen(framecap.outfile))
  {
    fprintf(stderr, "Error: output filename too long\n");
    return -1;
  }

  if(framecap.seqfile)
  {
    fprintf(stdout, "seq outfile:   %s\n", framecap.seqfile);
    if(235 < strlen(framecap.seqfile))
    {
      fprintf(stderr, "Error: output sequence filename too long\n");
      return -1;
    }
  }

  if(0 > lfc_capture(framecap.fname, &framecap, on_frame))
    fprintf(stderr, "Errors occurred!\n");

  fprintf(stdout, "\n");
  return 0;
}
