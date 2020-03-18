#ifndef _FRAMECAP_H
#define _FRAMECAP_H


#include <stdint.h>


// Number of memory-mapped framebuffers to use. Minimum is 1. 2 or more allows
// driver to buffer frames while your handler is processing the current frame.
#define LFC_FBUFS (2)

// Control printing of LibFrameCap's error messages to stderr.
// 0: do not print any messages, 1: print all messages
#define LFC_VERBOSE (1)

typedef struct Framecap Framecap;

// Create a new context to capture frames from <fname>.
// Returns NULL on error.
Framecap * framecap_new(const char *device, uint32_t bufcnt);

// Stop capturing and free a context.
int framecap_free(Framecap *ctx);

// Returns the next captured frame and its meta-data.
uint8_t * framecap_next(Framecap *ctx, uint32_t *l, uint32_t *w, uint32_t *h, uint32_t *ffmt);

// Tells the kernel it's OK to overwrite a frame captured by framecap_next()
int framecap_done(Framecap *ctx, uint8_t *frame);

#endif
