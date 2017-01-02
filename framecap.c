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
  int ii, jj, y1, y2, u, v;

  // every 8 bytes of YUYV422 represents 4 pixels
  // every 3 bytes of RGB represents 1 pixel
  // traverse YUYV422 array 4 bytes at a time and RGB array 6 bytes at a time,
  // and convert 2 pixels per iteration
  for (ii = 0, jj = 0; ii < npix*3; ii += 6, jj += 4)
  {
    y1 = ((unsigned char)yuyv[jj]) - 16;
    u  = ((unsigned char)yuyv[jj+1]) - 128;
    y2 = ((unsigned char)yuyv[jj+2]) - 16;
    v  = ((unsigned char)yuyv[jj+3]) - 128;
    rgb[ii]   = fmax(0, fmin(255, 1.164*y1 + 1.596*u));
    rgb[ii+1] = fmax(0, fmin(255, 1.164*y1 - 0.813*u - 0.391*v));
    rgb[ii+2] = fmax(0, fmin(255, 1.164*y1 + 2.018*u));
    rgb[ii+3] = fmax(0, fmin(255, 1.164*y2 + 1.596*u));
    rgb[ii+4] = fmax(0, fmin(255, 1.164*y2 - 0.813*u - 0.391*v));
    rgb[ii+5] = fmax(0, fmin(255, 1.164*y2 + 2.018*u));
  }
}


// Frame handler
int on_frame(void* ctx, char* frame, int len, int w_pix, int h_pix)
{
  FrameCap*      fc = (FrameCap*) ctx;
  static size_t  framecount = 0, capcount = 0;
  unsigned char* rgb;
  char           tmpname[255];
  int            fp, npix;

  // subsample frames
  framecount++;
  if(0 != framecount % fc->subsamp)
    return 0;

  sprintf(tmpname, "%s.tmp", fc->outfile);

  // Write image to temp file. Determine if format should be RAW or jpeg.
  // If fewer than 1 byte per pixel, assume that the frame is already in
  // jpeg format and write that directly to file
  npix = w_pix * h_pix;
  if(!fc->jpeg || npix > len)
  {
    fp = open(tmpname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    write(fp, frame, len);
    fsync(fp);
    close(fp);
  }
  else // Need to compress it ourselves
  {
    // Convert raw RGB image to jpeg & write to file
    // YUYV422 stores 4px in 8 bytes, rgb stores 1px in 3 bytes
    // Crude way to determine if YUYV422 format is used
    rgb = (unsigned char*) malloc(npix * 3);
    tje_encode_to_file_at_quality(tmpname, fc->jpeg, w_pix, h_pix, 3, rgb);
    free(rgb);
  }

  // Atomically move temp file to output file
  rename(tmpname, fc->outfile);

  // Write RAW frame to stdout if instructed
  if(fc->stdoutp)
    write(STDOUT_FILENO, frame, len);

  // Write sequence files if instructed

  // Only capture up to framecap->count frames
  // unless framecap->count is 0, in which case capture forever
  capcount++;
  if(capcount == fc->count && fc->count > 0)
    return -1;

  return 0;
}

void usage()
{
  fprintf(stderr, "Usage: framecap [opts] <device> <fname>\n");
  fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
  FrameCap  framecap;
  int       opt;

  opterr = 0;
  memset(&framecap, 0, sizeof(framecap));

  // Set defaults
  framecap.subsamp = 1; // sub-sampling modulus
  framecap.count   = 0; // number of frames to capture, 0 for forever
  framecap.jpeg    = 0; // default to RAW format

  // Parse command-line options
  while((opt = getopt(argc, argv, "bc:j:m:n:os:")) != -1)
  {
    switch (opt) {
//    case 'b':
//      framecap.banner = 1;
//      fprintf(stderr, "banner\n");
//      break;

    case 'c':
      framecap.count = atoi(optarg);
      if(framecap.count < 0)
        framecap.count = 0;
      fprintf(stderr, "count: %d\n", framecap.count);
      break;

    case 'j':
      framecap.jpeg = atoi(optarg);
      if(framecap.jpeg < 0)
        framecap.jpeg = 1;
      if(framecap.jpeg > 3)
        framecap.jpeg = 3;
      fprintf(stderr, "jpeg quality: %d\n", framecap.jpeg);
      break;

//    case 'm':
//      framecap.motion = atoi(optarg);
//      printf("motion percent: %d\n", framecap.motion);
//      break;

    case 'n':
      framecap.subsamp = atoi(optarg);
      if(framecap.subsamp < 1)
        framecap.subsamp = 1;
      fprintf(stderr, "sub-sample every: %d frames\n", framecap.subsamp);
      break;

    case 'o':
      fprintf(stderr, "output raw frame to STDOUT\n");
      framecap.stdoutp = 1;
      break;

    case 's':
      framecap.seqfile = optarg;
      fprintf(stderr, "sequential output to: %s\n", framecap.seqfile);
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
  fprintf(stderr, "v4l2 device:   %s\n", framecap.fname);
  fprintf(stderr, "output file:   %s\n", framecap.outfile);

  if(235 < strlen(framecap.outfile))
  {
    fprintf(stderr, "Error: output filename too long\n");
    return -1;
  }

  if(framecap.seqfile)
  {
    fprintf(stderr, "seq outfile:   %s\n", framecap.seqfile);
    if(235 < strlen(framecap.seqfile))
    {
      fprintf(stderr, "Error: output sequence filename too long\n");
      return -1;
    }
  }

  if(0 > lfc_capture(framecap.fname, &framecap, on_frame))
    fprintf(stderr, "Errors occurred!\n");

  fprintf(stderr, "\n");
  return 0;
}
