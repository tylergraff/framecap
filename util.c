// ---------------------------------------------------------------------------
// util.c
// Author: Tyler Graff, 2017
// tagraff@gmail.com
//
// Some useful image format manipulation routines I wrote.
// ---------------------------------------------------------------------------
//
// MIT License
// Copyright (c) Tyler Graff 2017
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
// ----------------------------------------------------------------------------
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "util.h"

#define TJE_IMPLEMENTATION
#include "tiny_jpeg.h"

#define CR_SAT_U (0x80 + 8)
#define CR_SAT_L (0x80 - 8)
#define CB_SAT_U (0x80 + 8)
#define CB_SAT_L (0x80 - 8)

#define IMGBLK_SIDE (80) // 80 px on a side
#define IMGBLK_AREA (IMGBLK_SIDE * IMGBLK_SIDE)
uint8_t * yuyv2imgblk(const uint8_t *yuyv, uint32_t xres, uint32_t yres) {
  uint32_t  idx, bx, by, xx, yy, npix, len;
  uint8_t  *blk, *blk_y0, *blk_y1, *blk_cb, *blk_cr, y0, y1, cr, cb;

  npix   = xres*yres;
  len    = npix*2;

  blk    = malloc(len);
  blk_y0 = blk + 0;
  blk_y1 = blk + 1;
  blk_cb = blk + len/2;
  blk_cr = blk + len/2 + len/4;

  idx = 0;
  for (by = 0; idx < len/4; by += IMGBLK_SIDE*xres/2) {
    for (bx = 0; bx < xres/2; bx += IMGBLK_SIDE) {
      for (yy = 0; yy < IMGBLK_SIDE*xres/2; yy += xres/2) {
        for (xx = 0; xx < IMGBLK_SIDE; xx++) {

          y0 = yuyv[4*(by + bx + yy + xx)+0];
          cb = yuyv[4*(by + bx + yy + xx)+1];
          y1 = yuyv[4*(by + bx + yy + xx)+2];
          cr = yuyv[4*(by + bx + yy + xx)+3];

/*
          if (cr < 125 && cr >= 120) { cr = 122; }
          if (cr < 120 && cr >= 116) { cr = 118; }
          if (cr < 116 && cr >= 110) { cr = 113; }
          if (cr < 110 && cr >= 100) { cr = 105; }
          if (cr < 100) { cr = 95; }

          if (cr > 131 && cr <= 136) { cr = 134; }
          if (cr > 136 && cr <= 140) { cr = 138; }
          if (cr > 140 && cr <= 150) { cr = 145; }
          if (cr > 150 && cr <= 160) { cr = 155; }
          if (cr > 160) { cr = 165; }

          if (cb < 125 && cb >= 120) { cb = 122; }
          if (cb < 120 && cb >= 116) { cb = 118; }
          if (cb < 116 && cb >= 110) { cb = 113; }
          if (cb < 110 && cb >= 100) { cb = 105; }
          if (cb < 100) { cb = 95; }

          if (cb > 131 && cb <= 136) { cb = 134; }
          if (cb > 136 && cb <= 140) { cb = 138; }
          if (cb > 140 && cb <= 150) { cb = 145; }
          if (cb > 150 && cb <= 160) { cb = 155; }
          if (cb > 160) { cb = 165; }
          cb = cb > CR_SAT_L && cb < CR_SAT_U ? cb &= 0xFC : cb & 0xF0;
          cr = cr > CR_SAT_L && cr < CR_SAT_U ? cr &= 0xFC : cr & 0xF0;
*/

          if      (abs(0x80 - y0) > 0x20) { y0 = (y0 & 0xF8); }
          else if (abs(0x80 - y0) > 0x10) { y0 = (y0 & 0xFC); }

          if      (abs(0x80 - y1) > 0x20) { y1 = (y1 & 0xF8); }
          else if (abs(0x80 - y1) > 0x10) { y1 = (y1 & 0xFC); }

          if      (abs(0x80 - cr) > 0x10) { cr = (cr & 0xF8); }
          else if (abs(0x80 - cr) > 0x08) { cr = (cr & 0xFC); }

          if      (abs(0x80 - cb) > 0x10) { cb = (cb & 0xF8); }
          else if (abs(0x80 - cb) > 0x08) { cb = (cb & 0xFC); }


          blk_y0[2*idx] = y0;
          blk_cb[idx]   = cb;
          blk_y1[2*idx] = y1;
          blk_cr[idx]   = cr;
          idx++;
        }
      }
    }
  }
  return blk;
}

uint8_t * imgblk2yuyv(const uint8_t *blk, uint32_t xres, uint32_t yres) {
  uint32_t       idx, bx, by, xx, yy, npix, len;
  uint8_t       *yuyv;
  const uint8_t *blk_y0, *blk_y1, *blk_cb, *blk_cr;

  npix   = xres*yres;
  len    = npix*2;

  blk_y0 = blk + 0;
  blk_y1 = blk + 1;
  blk_cb = blk + len/2;
  blk_cr = blk + len/2 + len/4;
  yuyv   = malloc(len);

  idx = 0;
  for (by = 0; idx < len/4; by += IMGBLK_SIDE*xres/2) {
    for (bx = 0; bx < xres/2; bx += IMGBLK_SIDE) {
      for (yy = 0; yy < IMGBLK_SIDE*xres/2; yy += xres/2) {
        for (xx = 0; xx < IMGBLK_SIDE; xx++) {
          yuyv[4*(by + bx + yy + xx)+0] = blk_y0[2*idx];
          yuyv[4*(by + bx + yy + xx)+1] = blk_cb[idx];
          yuyv[4*(by + bx + yy + xx)+2] = blk_y1[2*idx];
          yuyv[4*(by + bx + yy + xx)+3] = blk_cr[idx];
          idx++;
        }
      }
    }
  }
  return yuyv;
}


uint8_t * file_read(const char *fname, size_t *fsize) {
  ssize_t  rc;
  size_t   blen, ofst;
  uint8_t *buf;
  int      fd, err = 0;

  fd = open(fname, O_RDONLY);
  if (0 > fd) { return NULL; }

  // read up to 1 page first
  blen = getpagesize();
  buf  = NULL;
  ofst = 0;
  rc   = 1;

  // read() returns bytes read, 0 on EOF, or -1 on error
  while (rc) {
    buf = realloc(buf, blen);
    rc  = read(fd, buf + ofst, blen - ofst);
    if (0 <= rc) {
      ofst += rc;
      // double the buffer if half or more full
      blen *= (blen/(1+ofst) <= 1) ? 2 : 1;
    }
    else if (EINTR != errno) { err = 1; break; }
  }

  close(fd);

  // realloc down to actual bytes read or 1 if no bytes read
  buf = realloc(buf, ofst > 0 ? ofst : 1);

  // An error occurred
  if (err) {
    free(buf);
    return NULL;
  }

  if (fsize) { *fsize = ofst; }

  return buf;
}


static uint8_t ycr_to_r(uint8_t y, uint8_t cr)
{
  int _y = y;
  int _cr = cr - 128;
  int r;

  // 1.403 ~= 45/32
  r = (_y*32) + 45*_cr;
  if (r <= 0)
    r = 0;
  else if (r >= 32*255)
    r = 255;
  else
    r = r / 32;

  return (uint8_t)r;
}

static uint8_t ycrcb_to_g(uint8_t y, uint8_t cr, uint8_t cb)
{
  int _y = y;
  int _cr = cr - 128;
  int _cb = cb - 128;
  int g;

  // 0.7169 ~= 23/32
  // 0.3455 ~= 11/32
  g = (_y*32) - 11*_cb - 23*_cr;
  if (g <= 0)
    g = 0;
  else if (g >= 32*255)
    g = 255;
  else
    g = g / 32;

  return (uint8_t)g;
}

static uint8_t ycb_to_b(uint8_t y, uint8_t cb)
{
  int _y = y;
  int _cb = cb - 128;
  int b;

  // 1.7790 ~= 57/32
  b = (_y*32) + 57*_cb;
  if (b <= 0)
    b = 0;
  else if (b >= 32*255)
    b = 255;
  else
    b = b / 32;

  return (uint8_t)b;
}

typedef struct {
  size_t   len;
  uint8_t *buf;
} JBuf;

// callback for tiny_jpeg that receives "chunks" of jpeg data and saves them
// Structure keeping track of all state for a child of this silo
static void chunk(void* ctx, void* data, int size)
{
  JBuf *jbuf = (JBuf*)ctx;

  // Allocate a new buffer to hold the next jpeg file chunk
  jbuf->buf = realloc(jbuf->buf, jbuf->len + size);
  if (jbuf) {
    memcpy(jbuf->buf + jbuf->len, data, size); // copy new chunk
    jbuf->len += size;
  }
}


// ----------------------------------------------------------------------------
// Public API
// ----------------------------------------------------------------------------

ssize_t file_write_atomic(char *fname, uint8_t *data, size_t len) {
  char tmp[256];
  int  fp;

  // Write data to temp file, then rename to output file
  snprintf(tmp, sizeof(tmp), "%s.XXXXXX", fname);
  mkstemp(tmp);

  if (strlen(tmp) == 0)
    return -1;

  fp = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fp < 0) {
    unlink(tmp);
    return -1;
  }

  if (len != (size_t)write(fp, data, len)) {
    close(fp);
    return -1;
  }
  close(fp);

  if (-1 == rename(tmp, fname))
    return -1;

  return len;
}

// Convert YUYV422 to RGB
void yuyv422_to_rgb24(uint8_t *rgb, uint8_t *yuyv, uint32_t npix) {
  uint32_t y, cr, cb, ii, jj;

  for (ii = 0, jj = 0; ii < 3*npix; ii+=6, jj+=4)
  {
    // first pixel uses 1st Y value
    y  = yuyv[jj+0];
    cb = yuyv[jj+1];
    cr = yuyv[jj+3];

    rgb[ii+0] = ycr_to_r(y, cr);
    rgb[ii+1] = ycrcb_to_g(y, cr, cb);
    rgb[ii+2] = ycb_to_b(y, cb);

    // second pixel uses 2nd Y value
    y  = yuyv[jj+2];

    rgb[ii+3] = ycr_to_r(y, cr);
    rgb[ii+4] = ycrcb_to_g(y, cr, cb);
    rgb[ii+5] = ycb_to_b(y, cb);
  }
}

// Convert YUYV422 to JPEG File-format
uint8_t * yuyv422_to_jpeg(uint8_t *yuyv, uint32_t w, uint32_t h,
                          uint8_t qual, size_t *len) {

  uint8_t *rgb;
  JBuf     ctx = {0};

  rgb =  malloc(3*w*h);
  if (!rgb)
    return 0;

  // Convert to RGB first
  yuyv422_to_rgb24(rgb, yuyv, w*h);

  // jpeg_handler() callback mallocs fc->frame and copies jpeg to it
  tje_encode_with_func(chunk, &ctx, qual, w, h, 3, rgb);

  free(rgb);

  if (len)
    *len = ctx.len;

  return ctx.buf;
}

// Convert RGB24 to JPEG File-format
uint8_t * rgb24_to_jpeg(uint8_t *rgb, uint32_t w, uint32_t h,
                        uint8_t qual, size_t *len) {

  JBuf     ctx = {0};

  if (!rgb)
    return 0;

  // jpeg_handler() callback mallocs fc->frame and copies jpeg to it
  tje_encode_with_func(chunk, &ctx, qual, w, h, 3, rgb);

  if (len)
    *len = ctx.len;

  return ctx.buf;
}


void yuyv_putstr(char *str, uint32_t x, uint32_t y,
                 uint8_t *yuyv, uint32_t w, uint32_t h) {
  uint32_t xx, yy, len;

  len = strlen(str);

  for(yy = 0; yy < CHR_W; yy++)
  {
    if (yy + y > h - 1)
      break;
    for(xx = 0; xx < len*CHR_W; xx++)
    {
      if (xx + x > w -1)
        break;
      if (font[(uint8_t)str[xx/CHR_W]][yy] & (1<<(xx%CHR_W)))
      {
        yuyv[2*(yy+y)*w + 2*(xx+x)]     = YUYV_TEXT_Y;
        yuyv[2*(yy+y)*w + 2*(xx+x) + 1] = YUYV_TEXT_CRCB;
      }
      else
      {
        yuyv[2*(yy+y)*w + 2*(xx+x)]     = YUYV_BACK_Y;
        yuyv[2*(yy+y)*w + 2*(xx+x) + 1] = YUYV_BACK_CRCB;
      }
    }
  }
}
