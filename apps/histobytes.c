#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "../common/util.h"

int main(int argc, char **argv)
{
  uint64_t  ii, len, max = 0, histo[256] = {0};
  uint8_t  *buf;

  // slurp the file
  buf = file_read("/dev/stdin", &len);
  if (!len)
    exit(1);

  for (ii = 0; ii < len; ii++) {
    histo[buf[ii]]++;
    if (max < histo[buf[ii]]) {
      max = histo[buf[ii]];
    }
  }

  max = max < 76 ? 76 : max;

  for (ii = 0; ii < 256; ii++) {
    fprintf(stdout, "%2x|", ii);
    if (0 != histo[ii])
      fprintf(stdout, "#");
    for (len = 0; len < histo[ii]/(max/76); len++) {
      fprintf(stdout, "#");
    }
    fprintf(stdout, "\n");
  }
  return 0;
}
