// Copyright Tyler Graff 2016
// tagraff@gmail.com
//

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

// YUYV422 interleaved pixel format
typedef struct
{
  unsigned char y0;
  unsigned char cb;
  unsigned char y1;
  unsigned char cr;
} YuYv;

