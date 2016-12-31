
// ---------------------------------------------------------------------------
// LibFrameCap
// Wraps v4l2 and provides callback on each frame

typedef int (*OnFrame)(void*,char*,int);

typedef struct
{
  char*   fname;
  int     is_v4l2;
  int     fp;
  char*   frame;
  int     frame_size;
  void*   user_ptr;
  OnFrame on_frame;
} LibFrameCap;


// Initialize libframecap with a user-supplied function pointer to call on
// every frame and a user context pointer to pass in to that function
int
lfc_open(LibFrameCap* lfc, char* fname, void* user, OnFrame cb)
{
  int fp;

  lfc->user_ptr = user;
  lfc->on_frame = cb;

  // TODO: try to open as v4l2 device, on failure, open as static image


  // Support static image from file
  fp = open(fname, O_RDONLY, S_IREAD);

  if(fp < 0)
    return -1;

  // Determine file size
  lfc->frame_size = lseek(fp, 0L, SEEK_END);
  lseek(fp, 0L, SEEK_SET);

  // File must be between 1 and MAXFRAME bytes
  if(lfc->frame_size < 1 || lfc->frame_size > MAXFRAME)
  {
    close(fp);
    return -1;
  }

  lfc->is_v4l2 = 0;

  lfc->frame = (char*) malloc(lfc->frame_size);

  if(!lfc->frame)
  {
    close(fp);
    return -1;
  }

  // Clear frame, read entire file into frame
  memset(lfc->frame, 0, lfc->frame_size);
  read(fp, lfc->frame, lfc->frame_size);

  // File has been read into frame buffer so we don't need it anymore
  close(fp);

  return 0;
}

// Start capture loop. Returns when <n> frames have been captured, error occurs,
// or user-callback returns < 0
// n=0 means capture forever
int lfc_capture(LibFrameCap* lfc, int frames)
{
//  int

  return 0;
}

// Clean up
void lfc_close(LibFrameCap* lfc)
{
    free(lfc->frame);
}

