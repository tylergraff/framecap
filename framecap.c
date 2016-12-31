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

// Frame callback
int on_frame(void* ctx, char* frame, int len);

typedef struct
{
  // From cmd-line params
  int banner;
  int count;
  int jpeg;
  int motion;
  int subsamp;
  int stdoutp;
  int stdoutp_raw;
  char* outfile;
  char* seqfile;
  char* vdev;

  // Internal state
  int current_frame;

} FrameCap;


int on_frame(void* ctx, char* frame, int len)
{
  return 0;
}


void usage()
{
  fprintf(stderr, "Usage: framecap [opts] <device> <fname>\n");
  fprintf(stdout, "\n");
}


int main(int argc, char **argv)
{
  FrameCap framecap;
  // Options, flags, parameters
//  int bb=0, cc=0, jj=95, mm=5, nn=1, oo=0, oo_raw=0;
//  char* outfile     = NULL;
//  char* seq_outfile = NULL;
//  char* vdev        = NULL;    // v4l2 device name

  int   opt;

  opterr = 0;

  memset(&framecap, 0, sizeof(FrameCap));


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

  framecap.outfile = argv[argc - 2];
  framecap.vdev    = argv[argc - 1];
  fprintf(stdout, "v4l2 device:   %s\n", framecap.vdev);
  fprintf(stdout, "output file:   %s\n", framecap.outfile);

  if(framecap.seqfile)
    fprintf(stdout, "seq outfile:   %s\n", framecap.seqfile);




  // libframecap_init();
  // libframecap_open();
  // libframecap_capture();
  // libframecap_close();

  fprintf(stdout, "\n");
  return 0;
}
