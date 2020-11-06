// Generate backgrounds for plugins programmatically.

#include <string>
#include <vector>
#include <cstdint>

#include "base/logging.h"
#include "base/stringprintf.h"

#include "image.h"
#include "util.h"
#include "arcfour.h"

using namespace std;

using uint8 = uint8_t;
using uint32 = uint32_t;

constexpr int WIDTH = 512;
constexpr int HEIGHT = 512;

int main(int argc, char **argv) {
  ImageRGBA img(WIDTH, HEIGHT);

  img.Clear32(0x000000ff);

  /*
  uint32 red = 0x740000ff;
  uint32 yellow = 0xd1d34fff;
  uint32 orange = 0xc98320ff;
  */
  // Not actually red, yellow, orange!
  uint32 red = 0x007434ff;
  uint32 yellow = 0x4f95d3ff;
  uint32 orange = 0x20c2c9ff;

  constexpr int SQUARE = 8;
  static_assert (WIDTH % SQUARE == 0);
  static_assert (HEIGHT % SQUARE == 0);  

  constexpr int SQUARESW = WIDTH / SQUARE;
  constexpr int SQUARESH = HEIGHT / SQUARE;

  auto Box = [&img](int x, int y, int w, int h, uint32 color) {
      // Note this draws some corners multiple times!
      // (This could be in ImageRGBA, covering these subtleties)
      const int x1 = x + w - 1;
      const int y1 = y + h - 1;
      // Top
      img.BlendLine32(x, y, x1, y, color);
      // Left
      img.BlendLine32(x, y, x, y1, color);
      // Right
      img.BlendLine32(x1, y, x1, y1, color);
      // Bottom
      img.BlendLine32(x, y1, x1, y1, color);
    };

  // Maybe should make this blue-noisey; the clumps can be a little
  // visually distracting!
  ArcFour rc("makebg");
  for (int row = 0; row < SQUARESH; row++) {
    int y = row * SQUARE;
    for (int col = 0; col < SQUARESW; col++) {
      int x = col * SQUARE;
      const auto [fg, bg] = [&]() -> std::tuple<uint32, uint32> {
	  switch (rc.Byte() & 3) {
	  default: return {yellow, red};
	  case 0: return {orange, red};
	  case 1: return {yellow, orange};
	  case 2: return {red, orange};
	  }
	}();

      switch (rc.Byte() & 1) {
      case 0:
	img.BlendRect32(x, y, SQUARE, SQUARE, bg);
	Box(x, y, SQUARE, SQUARE, fg);
	Box(x + 1, y + 1, SQUARE - 2, SQUARE - 2, fg);
	break;
      case 1:
	img.BlendRect32(x, y, SQUARE, SQUARE, bg);
	Box(x, y, SQUARE, SQUARE, fg);
	img.BlendRect32(x + 2, y + 2, SQUARE - 4, SQUARE - 4, fg);
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
