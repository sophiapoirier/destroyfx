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

  // Pass DrawPixel(int x, int y, uint8 v) which should do the pixel blending.
  template<class DP>
  void BlitString(int x, int y, int size_px,
		  const string &text, const DP &DrawPixel,
		  bool subpixel = true) {
    const float scale = stbtt_ScaleForPixelHeight(&font, size_px);

    const int baseline = [&]() {
	int ascent = 0;
	stbtt_GetFontVMetrics(&font, &ascent, 0, 0);
	return (int) (ascent * scale);
      }();

    const int ypos = y + baseline;
    // Should stay integral if subpixel is false.
    float xpos = x;
    for (int idx = 0; idx < (int)text.size(); idx++) {

      int advance = 0, left_side_bearing = 0;
      stbtt_GetCodepointHMetrics(&font, text[idx], &advance, &left_side_bearing);

      int bitmap_w = 0, bitmap_h = 0;
      int xoff = 0, yoff = 0;
      uint8 *bitmap = nullptr;      
      if (subpixel) {
	const float x_shift = xpos - (float) floor(xpos);
	constexpr float y_shift = 0.0f;
	bitmap = stbtt_GetCodepointBitmapSubpixel(&font, scale, scale,
						  x_shift, y_shift,
						  text[idx],
						  &bitmap_w, &bitmap_h,
						  &xoff, &yoff);
      } else {
	bitmap = stbtt_GetCodepointBitmap(&font, scale, scale,
					  text[idx],
					  &bitmap_w, &bitmap_h,
					  &xoff, &yoff);
      }
      if (bitmap == nullptr) continue;
      
      for (int yy = 0; yy < bitmap_h; yy++) {
	for (int xx = 0; xx < bitmap_w; xx++) {
	  DrawPixel(xpos + xx + xoff, ypos + yy + yoff, bitmap[yy * bitmap_w + xx]);
	}
      }

      stbtt_FreeBitmap(bitmap, nullptr);

      xpos += advance * scale;
      if (text[idx + 1] != '\0') {
	xpos += scale * stbtt_GetCodepointKernAdvance(&font, text[idx], text[idx + 1]);
      }
      
      if (!subpixel) {
	// Or floor?
	xpos = roundf(xpos);
      }
    }
  }

private:
  vector<uint8> ttf_bytes;
  stbtt_fontinfo font;

};


int main(int argc, char **argv) {
  Blue blue;
  // Looks good with size=14, subpixel false.
  TTFont snoot("../fonts/px10.ttf");
  
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
      int lx = CTRL_X + LABEL_X;
      int ly = i * CTRL_H + CTRL_Y + LABEL_Y;

      // Using builtin bit7.
      // img.BlendText2x32(lx, ly, 0xFFFFFFFF, labels[i]);

      snoot.BlitString(lx, ly, 14, labels[i],
		       [&](int x, int y, uint8 v) {
			 if (v > 128) {
			   // uint32 color = 0xFFFFFF00 | v;
			   img.BlendPixel32(x, y, 0xFFFFFFFF);
			 }
		       },
		       false);
    }
  }
  
  
  img.Save("skidder.png");

  return 0;
}
