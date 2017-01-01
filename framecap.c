// framecap: Capture and save v4l2 camera frames to multiple locations
// Copyright Tyler Graff 2016
// tyler@prolaag.com
//

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


#include "framecap.h"
#include "libframecap.h"

typedef struct
{
  int banner;
  int count;
  int jpeg;
  int motion;
  int subsamp;
  int stdoutp;
  int stdoutp_raw;
  char* outfile;
  char* seqfile;
  char* fname;

  // Internal state
  int current_frame;
} FrameCap;


// Frame callback
int on_frame(void* ctx, char* frame, int len)
{
  FrameCap*  fc = (FrameCap*) ctx;
  static int count = 10;
  char       fname[255];
  int        fp;

  sprintf(fname, "%s-%d.raw", fc->outfile, count);
  count--;

  printf("%s\n", fname);

  fp = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
  write(fp, frame, len);
  fsync(fp);
  close(fp);

  if(count)
    return 0;

  return -1;
}

void usage()
{
  fprintf(stderr, "Usage: framecap [opts] <device> <fname>\n");
  fprintf(stdout, "\n");
}

int main(int argc, char **argv)
{
  FrameCap    framecap;
  LibFrameCap lfc;
  int         opt;

  opterr = 0;

  memset(&framecap, 0, sizeof(framecap));
  memset(&lfc, 0, sizeof(lfc));

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

  framecap.outfile = argv[argc - 1];
  framecap.fname    = argv[argc - 2];
  fprintf(stdout, "v4l2 device:   %s\n", framecap.fname);
  fprintf(stdout, "output file:   %s\n", framecap.outfile);

  if(235 < strlen(framecap.outfile))
  {
    fprintf(stderr, "Error: output filename too long\n");
    return -1;
  }

  if(framecap.seqfile)
  {    fprintf(stdout, "seq outfile:   %s\n", framecap.seqfile);
    if(235 < strlen(framecap.seqfile))
    {
      fprintf(stderr, "Error: output sequence filename too long\n");
      return -1;
    }
  }

  if(0 > lfc_open(&lfc, framecap.fname))
    return -1;

  lfc_capture(&lfc, &framecap, on_frame);
  lfc_close(&lfc);

  fprintf(stdout, "\n");
  return 0;
}
