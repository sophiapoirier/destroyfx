
/*
*/


/* Define the abstract types for the drawing library. 
   Don't look at this, look at the DfxDraw class below! */

#if defined(TARGET_API_VST)

typedef directx_bitmap bitmap;
typedef directx_surface surface;

#elif defined(TARGET_API_AUDIOUNIT)

typedef ...

#endif

/* Here is where to look! */

struct DfxDraw {

  static void drawbitmap (bitmap b, surface s, int x, int y); 

  /* ... */

};
