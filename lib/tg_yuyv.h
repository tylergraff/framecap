// ---------------------------------------------------------------------------
// tg_yuyv.h
// Author: Tyler Graff, 2017
// tyler@graff.com
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
// Public API:

// Converts YUYV422-format image <yuyv> of total element count (W x H) <npix>
// into RGB24 image at user-allocated space in <rgb>.
// Note that you must allocate 3*npix bytes at <rgb> before calling.
static void yuyv_to_rgb(unsigned char* rgb,
                        unsigned char* yuyv,
                        int            npix);

// Prints string <str> at location <str_x>,<str_y> on image <yuyv>
// having resolution <yuyv_w> x <yuyv_h> pixels
static void tg_yuyv_putstr(unsigned char* yuyv,
                           unsigned int   yuyv_w,
                           unsigned int   yuyv_h,
                           char*          str,
                           unsigned int   str_x,
                           unsigned int   str_y);

// Foreground and background Y/CrCb values
#define TG_YUYV_TEXT_Y    (0xFF)
#define TG_YUYV_TEXT_CRCB (0x7F)
#define TG_YUYV_BACK_Y    (0x00)
#define TG_YUYV_BACK_CRCB (0x7F)

// ----------------------------------------------------------------------------
static unsigned char int_ycr_to_r(unsigned char y, unsigned char cr)
{
  int _y = y;
  int _cr = cr - 128;
  int r;

  // 1.403 ~= 45/32
  r = (_y<<5) + 45*_cr;
  if(r <= 0)
    r = 0;
  else if(r >= 32*255)
    r = 255;
  else
    r = r >> 5; // divide-by-32

  return (unsigned char)r;
}

static unsigned char int_ycrcb_to_g(unsigned char y, unsigned char cr, unsigned char cb)
{
  int _y = y;
  int _cr = cr - 128;
  int _cb = cb - 128;
  int g;

  // 0.7169 ~= 23/32
  // 0.3455 ~= 11/32
  g = (_y<<5) - 11*_cb - 23*_cr;
  if(g <= 0)
    g = 0;
  else if(g >= 32*255)
    g = 255;
  else
    g = g >> 5;

  return (unsigned char)g;
}

static unsigned char int_ycb_to_b(unsigned char y, unsigned char cb)
{
  int _y = y;
  int _cb = cb - 128;
  int b;

  // 1.7790 ~= 57/32
  b = (_y<<5) + 57*_cb;
  if(b <= 0)
    b = 0;
  else if(b >= 32*255)
    b = 255;
  else
    b = b >> 5; // divide-by-32

  return (unsigned char)b;
}

// Convert YUYV422 to RGB:
static void yuyv_to_rgb(unsigned char* rgb, unsigned char* yuyv, int npix)
{
  int y, cr, cb, ii, jj;

  for (ii = 0, jj = 0; ii < npix*3; ii+=6, jj+=4)
  {
    // first pixel uses 1st Y value
    y  = yuyv[jj+0];
    cb = yuyv[jj+1];
    cr = yuyv[jj+3];

    rgb[ii+0] = int_ycr_to_r(y, cr);
    rgb[ii+1] = int_ycrcb_to_g(y, cr, cb);
    rgb[ii+2] = int_ycb_to_b(y, cb);

    // second pixel uses 2nd Y value
    y  = yuyv[jj+2];

    rgb[ii+3] = int_ycr_to_r(y, cr);
    rgb[ii+4] = int_ycrcb_to_g(y, cr, cb);
    rgb[ii+5] = int_ycb_to_b(y, cb);
  }
}

#define TG_CHR_W (8) // character pixel width
#define TG_CHR_H (8) // character pixel height
const char tg_font[128][TG_CHR_H] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x00
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x01
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x02
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x03
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x04
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x05
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x06
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x07
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x08
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x09
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x0A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x0B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x0C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x0D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x0E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x0F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x10
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x11
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x12
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x13
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x14
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x15
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x16
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x17
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x18
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x19
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x1A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x1B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x1C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x1D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x1E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x1F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x20 ( )
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},  // 0x21 (!)
    { 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x22 (")
    { 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00},  // 0x23 (#)
    { 0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00},  // 0x24 ($)
    { 0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},  // 0x25 (%)
    { 0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00},  // 0x26 (&)
    { 0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x27 (')
    { 0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00},  // 0x28 (()
    { 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00},  // 0x29 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},  // 0x2A (*)
    { 0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00},  // 0x2B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06},  // 0x2C (,)
    { 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00},  // 0x2D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},  // 0x2E (.)
    { 0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},  // 0x2F (/)
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},  // 0x30 (0)
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},  // 0x31 (1)
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},  // 0x32 (2)
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},  // 0x33 (3)
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},  // 0x34 (4)
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},  // 0x35 (5)
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},  // 0x36 (6)
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},  // 0x37 (7)
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},  // 0x38 (8)
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},  // 0x39 (9)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},  // 0x3A (:)
    { 0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06},  // 0x3B (/)
    { 0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00},  // 0x3C (<)
    { 0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00},  // 0x3D (=)
    { 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00},  // 0x3E (>)
    { 0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},  // 0x3F (?)
    { 0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00},  // 0x40 (@)
    { 0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00},  // 0x41 (A)
    { 0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00},  // 0x42 (B)
    { 0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00},  // 0x43 (C)
    { 0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00},  // 0x44 (D)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00},  // 0x45 (E)
    { 0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00},  // 0x46 (F)
    { 0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00},  // 0x47 (G)
    { 0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00},  // 0x48 (H)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},  // 0x49 (I)
    { 0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00},  // 0x4A (J)
    { 0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00},  // 0x4B (K)
    { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00},  // 0x4C (L)
    { 0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00},  // 0x4D (M)
    { 0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00},  // 0x4E (N)
    { 0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},  // 0x4F (O)
    { 0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},  // 0x50 (P)
    { 0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00},  // 0x51 (Q)
    { 0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00},  // 0x52 (R)
    { 0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00},  // 0x53 (S)
    { 0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},  // 0x54 (T)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00},  // 0x55 (U)
    { 0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},  // 0x56 (V)
    { 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00},  // 0x57 (W)
    { 0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00},  // 0x58 (X)
    { 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00},  // 0x59 (Y)
    { 0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00},  // 0x5A (Z)
    { 0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00},  // 0x5B ([)
    { 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},  // 0x5C (\)
    { 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00},  // 0x5D (])
    { 0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00},  // 0x5E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},  // 0x5F (_)
    { 0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x60 (`)
    { 0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00},  // 0x61 (a)
    { 0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00},  // 0x62 (b)
    { 0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},  // 0x63 (c)
    { 0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00},  // 0x64 (d)
    { 0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00},  // 0x65 (e)
    { 0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00},  // 0x66 (f)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F},  // 0x67 (g)
    { 0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00},  // 0x68 (h)
    { 0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},  // 0x69 (i)
    { 0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E},  // 0x6A (j)
    { 0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00},  // 0x6B (k)
    { 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00},  // 0x6C (l)
    { 0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00},  // 0x6D (m)
    { 0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00},  // 0x6E (n)
    { 0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},  // 0x6F (o)
    { 0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F},  // 0x70 (p)
    { 0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78},  // 0x71 (q)
    { 0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00},  // 0x72 (r)
    { 0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00},  // 0x73 (s)
    { 0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00},  // 0x74 (t)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00},  // 0x75 (u)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00},  // 0x76 (v)
    { 0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00},  // 0x77 (w)
    { 0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00},  // 0x78 (x)
    { 0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F},  // 0x79 (y)
    { 0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00},  // 0x7A (z)
    { 0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00},  // 0x7B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},  // 0x7C (|)
    { 0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00},  // 0x7D (})
    { 0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // 0x7E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}   // 0x7F
};

static void tg_yuyv_putstr(unsigned char* yuyv,
                           unsigned int   yuyv_w,
                           unsigned int   yuyv_h,
                           char*          str,
                           unsigned int   str_x,
                           unsigned int   str_y)
{
  int xx, yy, len;

  len = strlen(str);

  for(yy = 0; yy < TG_CHR_W; yy++)
  {
    if(yy + str_y > yuyv_h - 1)
      break;
    for(xx = 0; xx < len*TG_CHR_W; xx++)
    {
      if(xx + str_x > yuyv_w -1)
        break;
      if(tg_font[(unsigned char)str[xx/TG_CHR_W]][yy] & (1<<(xx%TG_CHR_W)))
      {
        yuyv[2*(yy+str_y)*yuyv_w + 2*(xx+str_x)]     = TG_YUYV_TEXT_Y;
        yuyv[2*(yy+str_y)*yuyv_w + 2*(xx+str_x) + 1] = TG_YUYV_TEXT_CRCB;
      }
      else
      {
        yuyv[2*(yy+str_y)*yuyv_w + 2*(xx+str_x)]     = TG_YUYV_BACK_Y;
        yuyv[2*(yy+str_y)*yuyv_w + 2*(xx+str_x) + 1] = TG_YUYV_BACK_CRCB;
      }
    }
  }
}