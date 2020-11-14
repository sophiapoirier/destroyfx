// Generate backgrounds for plugins programmatically.

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "base/logging.h"
#include "base/stringprintf.h"

#include "image.h"
#include "util.h"
#include "arcfour.h"

using namespace std;

using uint8 = uint8_t;
using uint32 = uint32_t;

constexpr int WIDTH = 512;
constexpr int HEIGHT = 512 + 8 * 24;

struct Blue {
  static constexpr int SIZE = 470;
  Blue() : noise(SIZE, SIZE) {
    // The image is actually RGBA; convert to single-channel.
    // This texture comes from "Free blue noise textures":
    //   http://momentsingraphics.de/BlueNoise.html
    std::unique_ptr<ImageRGBA> rgba(
	ImageRGBA::Load("bluenoise470.png"));
    CHECK(rgba.get());
    CHECK(rgba->width == SIZE);
    CHECK(rgba->height == SIZE);
    for (int y = 0; y < SIZE; y++) {
      for (int x = 0; x < SIZE; x++) {
	uint32 p = rgba->GetPixel(x, y);
	// Assumes all the channels are the same.
	uint8 v = p >> 24;
	noise.SetPixel(x, y, v);
      }
    }	
  }

  uint8 Get(int x, int y) const {
    return noise.GetPixel(x, y);
  }
  
 private:
  ImageA noise;
};

int main(int argc, char **argv) {
  Blue blue;
  ImageRGBA img(WIDTH, HEIGHT);
  
  img.Clear32(0x000000ff);

  /*
  uint32 red = 0x740000ff;
  uint32 yellow = 0xd1d34fff;
  uint32 orange = 0xc98320ff;
  */
  // Not actually red, yellow, orange!

  // uint32 red = 0x005414ff;
  uint32 red = 0x03621aff;
  uint32 yellow = 0x3f6593ff;
  uint32 orange = 0x209289ff;

  constexpr int SQUARE = 8;
  static_assert (WIDTH % SQUARE == 0);
  static_assert (HEIGHT % SQUARE == 0);  

  constexpr int SQUARESW = WIDTH / SQUARE;
  constexpr int SQUARESH = HEIGHT / SQUARE;

  // Pass corner_color = color for a crisp box, but setting
  // the corners 
  auto Box = [&img](int x, int y, int w, int h,
		    uint32 color, uint32 corner_color) {
      // (This could be in ImageRGBA, covering these subtleties)
      const int x1 = x + w - 1;
      const int y1 = y + h - 1;

      // XXX this is probably wrong for 1x1 and 2x2 boxes.
      // Top
      img.BlendLine32(x + 1, y, x1 - 1, y, color);
      // Left
      img.BlendLine32(x, y + 1, x, y1 - 1, color);
      // Right
      img.BlendLine32(x1, y + 1, x1, y1 - 1, color);
      // Bottom
      img.BlendLine32(x + 1, y1, x1 - 1, y1, color);

      img.BlendPixel32(x, y, corner_color);
      img.BlendPixel32(x1, y, corner_color);
      img.BlendPixel32(x, y1, corner_color);
      img.BlendPixel32(x1, y1, corner_color);      
    };

  auto FilledBox = [&img, &Box](int x, int y, int w, int h,
				uint32 color, uint32 corner_color) {
      Box(x, y, w, h, color, corner_color);
      img.BlendRect32(x + 1, y + 1, w - 2, h - 2, color);
    };

  
  // Maybe should make this blue-noisey; the clumps can be a little
  // visually distracting!
  // ArcFour rc("makebg");
  for (int row = 0; row < SQUARESH; row++) {
    int y = row * SQUARE;
    for (int col = 0; col < SQUARESW; col++) {
      int x = col * SQUARE;

      uint8 sample = blue.Get(x, y);
	
      const auto [fg, bg] = [&]() -> std::tuple<uint32, uint32> {
	switch ((sample >> 5) & 3) {
	  default: return {yellow, red};
	  case 0: return {orange, red};
	  case 1: return {yellow, orange};
	  case 2: return {red, orange};
	  }
	}();

      uint32 fg_lite = (fg & 0xFFFFFF00) | 0x7F;
      
      switch ((sample >> 4) & 1) {
      case 0:
	img.BlendRect32(x, y, SQUARE, SQUARE, bg);
	// Box(x, y, SQUARE, SQUARE, fg, fg_lite);
	// Box(x + 1, y + 1, SQUARE - 2, SQUARE - 2, fg, fg_lite);
	FilledBox(x + 1, y + 1, SQUARE - 2, SQUARE - 2, fg, fg_lite);
	break;
      case 1:
	img.BlendRect32(x, y, SQUARE, SQUARE, bg);
	Box(x + 1, y + 1, SQUARE - 2, SQUARE - 2, fg, fg_lite);
	// img.BlendRect32(x + 2, y + 2, SQUARE - 4, SQUARE - 4, fg);
	break;
      default:
	// (impossible)
	img.BlendRect32(x, y, SQUARE, SQUARE, bg);
	break;

      }
    }
  }

  img.Save("makebg.png");
  
  return 0;
}
