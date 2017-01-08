// MIT License
// Copyright (c) Tyler Graff 2017
// tyler@graff.com
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
//

typedef struct
{
  char*  banner;
  double delay;
  int    count;
  int    jpeg;
  int    motion;
  int    subsamp;
  int    stdoutp;
  int    tstamp;
  char*  outfile;
  char*  seqfile;
  char*  v4l2;

  // Internal state
  size_t         framecount;
  unsigned char* jpg_buf;
  size_t         jpg_len;
} FrameCap;

// For collecting file before writing
typedef struct
{
  unsigned char* buf;
  size_t         len;
  char           fre; // set if buffer needs to be free'd
} ImgBuf;


