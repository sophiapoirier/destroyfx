// Playing around with programmatic background generation for skidder.

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <utility>
#include <optional>
#include <set>

#include "base/logging.h"
#include "base/stringprintf.h"

#include "image.h"
#include "util.h"
#include "arcfour.h"
#include "randutil.h"
#include "color-util.h"
#include "stb_truetype.h"

#include "makebg-util.h"

constexpr int WIDTH = 512;
constexpr int HEIGHT = 768;

struct TTFont {
  vector<uint8> ttf_bytes;
  stbtt_fontinfo font;
  
  TTFont(const string &filename) {
    ttf_bytes = Util::ReadFileBytes(filename);
    CHECK(!ttf_bytes.empty()) << filename;
  
    stbtt_InitFont(&font, ttf_bytes.data(), stbtt_GetFontOffsetForIndex(ttf_bytes.data(), 0));
  }

  // Not cached, so this does a lot more allocation than you probably want.
  ImageA GetChar(char c, int size) {
    int width, height;
    uint8 *bitmap = stbtt_GetCodepointBitmap(&font, 0, stbtt_ScaleForPixelHeight(&font, size),
					     c, &width, &height, 0, 0);
    CHECK(bitmap != nullptr) << "Character " << (char)c << " size " << size;

    const int bytes = width * height;
    vector<uint8> ret;
    ret.resize(bytes);
    memcpy(ret.data(), bitmap, bytes);
    stbtt_FreeBitmap(bitmap, nullptr);
    return ImageA(std::move(ret), width, height);
  }

};

#if 0
   int i,j,ascent,baseline,ch=0;
   float scale, xpos=2; // leave a little padding in case the character extends left

   scale = stbtt_ScaleForPixelHeight(&font, 15);
   stbtt_GetFontVMetrics(&font, &ascent,0,0);
   baseline = (int) (ascent*scale);

   while (text[ch]) {
      int advance,lsb,x0,y0,x1,y1;
      float x_shift = xpos - (float) floor(xpos);
      stbtt_GetCodepointHMetrics(&font, text[ch], &advance, &lsb);
      stbtt_GetCodepointBitmapBoxSubpixel(&font, text[ch], scale,scale,x_shift,0, &x0,&y0,&x1,&y1);
      stbtt_MakeCodepointBitmapSubpixel(&font, &screen[baseline + y0][(int) xpos + x0], x1-x0,y1-y0, 79, scale,scale,x_shift,0, text[ch]);
      // note that this stomps the old data, so where character boxes overlap (e.g. 'lj') it's wrong
      // because this API is really for baking character bitmaps into textures. if you want to render
      // a sequence of characters, you really need to render each bitmap to a temp buffer, then
      // "alpha blend" that into the working buffer
      xpos += (advance * scale);
      if (text[ch+1])
         xpos += scale*stbtt_GetCodepointKernAdvance(&font, text[ch],text[ch+1]);
      ++ch;
   }

   for (j=0; j < 20; ++j) {
      for (i=0; i < 78; ++i)
         putchar(" .:ioVM@"[screen[j][i]>>5]);
      putchar('\n');
   }
#endif




int main(int argc, char **argv) {
  Blue blue;

  ImageRGBA img(WIDTH, HEIGHT);
  img.Clear32(0x000000ff);

  uint32 color1 = 0x634021ff;
  uint32 color2 = 0x8d3d6cff;

  // uint32 color1 = 0xffff00ff;
  // uint32 color2 = 0x5959ffff;

  // Vertical stripes

  for (int x = 0; x < WIDTH; x++) {
    double f = x / (double)WIDTH;

    #if 0
    // smooth ramp...
    uint32 color = Mix32(color1, color2, f);
    for (int y = 0; y < HEIGHT; y++) {
      img.SetPixel32(x, y, color);
    }
    #endif

    uint32 color = 0xFF0000FF;
    int SPAN = 1 + blue.Get(x, 41);
    for (int y = 0; y < HEIGHT; y++) {
      if (y % SPAN == 0) {
	double noise = blue.Get(x, y) / 255.0;
	color = f < noise ? color1 : color2;
      }
      img.SetPixel32(x, y, color);
    }
  }


  // The slider control row.
  constexpr int CTRL_X = 27;
  constexpr int CTRL_Y = 78;

  constexpr int CTRL_W = 400;
  constexpr int CTRL_H = 64;

  constexpr int NUM_SLIDERS = 8;
  
  {
    // visualize the rows for "debugging"
    constexpr uint32 color1 = 0xFFFF000F;
    constexpr uint32 color2 = 0x0073FF0F;

    for (int i = 0; i < NUM_SLIDERS; i++) {
      img.BlendRect32(CTRL_X, CTRL_Y + i * CTRL_H,
		      CTRL_W, CTRL_H,
		      (i & 1) ? color1 : color2);
    }
  }

  {
    // big text labels for each row
    vector<string> labels = {
      "rate",
      "pulse width",
      "slope",
      "floor",
      "crossover frequency",
      "stereo spread",
      "rupture",
      "tempo",
    };
    CHECK(labels.size() >= NUM_SLIDERS);
    // within ctrl row
    constexpr int LABEL_X = 48;
    constexpr int LABEL_Y = 2;
    for (int i = 0; i < NUM_SLIDERS; i++) {
      img.BlendText2x32(CTRL_X + LABEL_X,
			i * CTRL_H + CTRL_Y + LABEL_Y,
			0xFFFFFFFF,
			labels[i]);
    }
  }
  
  
  img.Save("skidder.png");

  return 0;
}
