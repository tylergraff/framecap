#ifndef _V4L2CAP_H
#define _V4L2CAP_H


#include <stdint.h>


// Number of memory-mapped framebuffers to use. Minimum is 1. 2 or more allows
// driver to buffer frames while your handler is processing the current frame.
#define LFC_FBUFS (2)

// Control printing of LibFrameCap's error messages to stderr.
// 0: do not print any messages, 1: print all messages
#define LFC_VERBOSE (1)

typedef struct V4L2Cap V4L2Cap;

// Create a new context to capture frames from <fname>.
// Returns NULL on error.
V4L2Cap * v4l2cap_new(const char *device, uint32_t bufcnt);

// Stop capturing and free a context.
int v4l2cap_free(V4L2Cap *ctx);

// Returns the next captured frame and its meta-data.
uint8_t * v4l2cap_next(V4L2Cap *ctx,
                       uint32_t *l, uint32_t *w, uint32_t *h, uint32_t *ffmt);

// Tells the kernel it's OK to overwrite a frame captured by v4l2cap_next()
int v4l2cap_done(V4L2Cap *ctx, uint8_t *frame);

#endif
