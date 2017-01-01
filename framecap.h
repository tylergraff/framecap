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




