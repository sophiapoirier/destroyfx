
/* Compile this in all your projects, but it'll only
   do something if it's a VST project! */

#ifdef TARGET_API_VST

void drawbitmap(bitmap b, surface s, int x, int y) {

  directx_draw_bitmap(b, s, x, y);

}




#endif
