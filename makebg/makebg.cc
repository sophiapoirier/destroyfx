// Generate backgrounds for plugins programmatically.

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <utility>
#include <optional>
// #include <unordered_set>
#include <set>

#include "base/logging.h"
#include "base/stringprintf.h"

#include "image.h"
#include "util.h"
#include "arcfour.h"
#include "randutil.h"

using namespace std;

using uint8 = uint8_t;
using uint32 = uint32_t;


constexpr int SQUARE = 8;

constexpr int WIDTH = 512;
constexpr int HEIGHT = 512 + SQUARE * 24;

static_assert (WIDTH % SQUARE == 0);
static_assert (HEIGHT % SQUARE == 0);  

constexpr int SQUARESW = WIDTH / SQUARE;
constexpr int SQUARESH = HEIGHT / SQUARE;

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


enum Dir { UP, DOWN, LEFT, RIGHT, };
static std::optional<std::pair<int, int>> Move(int x, int y, Dir d) {
  switch (d) {
  case UP:
    if (y <= 0) return nullopt;
    return {{x, y - 1}};
  case DOWN:
    if (y >= SQUARESH - 1) return nullopt;
    return {{x, y + 1}};
  case LEFT:
    if (x <= 0) return nullopt;
    return {{x - 1, y}};
  case RIGHT:
    if (x >= SQUARESW - 1) return nullopt;
    return {{x + 1, y}};
  default: return nullopt;
  }
}

int main(int argc, char **argv) {
  Blue blue;
  
  // First create the image abstractly.
  // Here we're just using ImageA to store enum values.
  enum color : uint8 {
    ONE = 1,
    TWO = 2,
    THREE = 3,
  };
  ImageA fg(SQUARESW, SQUARESH);
  ImageA bg(SQUARESW, SQUARESH);
  fg.Clear(0);
  bg.Clear(0);  
  
  enum fill : uint8 {
    DOT = 1,
    LOOP = 2,
  };
  ImageA fill(SQUARESW, SQUARESH);
  fill.Clear(0);

  for (int y = 0; y < SQUARESH; y++) {
    for (int x = 0; x < SQUARESW; x++) {

      uint8 sample = blue.Get(x, y);

      // one = "yellow"
      // two = "red"
      // three = "orange"
      
      switch ((sample >> 5 & 3)) {
      case 0:
	fg.SetPixel(x, y, THREE);
	bg.SetPixel(x, y, TWO);
	break;
      case 1:
	fg.SetPixel(x, y, ONE);
	bg.SetPixel(x, y, THREE);
	break;
      case 2:
	fg.SetPixel(x, y, TWO);
	bg.SetPixel(x, y, THREE);
	break;
      default:
      case 3:
	fg.SetPixel(x, y, ONE);
	bg.SetPixel(x, y, TWO);
	break;
      }

      bool f = !!((sample >> 4) & 1);
      fill.SetPixel(x, y, f ? DOT : LOOP);
    }
  }

  
  // Now actually render it as colored pixels.
  ImageRGBA img(WIDTH, HEIGHT);  
  img.Clear32(0x000000ff);
  
  constexpr uint32 red = 0x03621aff;
  constexpr uint32 yellow = 0x3f6593ff;
  constexpr uint32 orange = 0x209289ff;

  auto ToColor = [](uint8 v) {
      switch (v) {
      case ONE: return yellow;
      case TWO: return red;
      case THREE: return orange;
      default: return 0xFF0000FF;
      }
    };
  
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

  // First, draw background colors.
  
  for (int row = 0; row < SQUARESH; row++) {
    int y = row * SQUARE;
    for (int col = 0; col < SQUARESW; col++) {
      int x = col * SQUARE;
      uint32 bg_color = ToColor(bg.GetPixel(col, row));      
      img.BlendRect32(x, y, SQUARE, SQUARE, bg_color);
    }
  }

  // Now, fill every cell. But allow joining a box/loop across cells
  // that match. Do this in a random order.
  ArcFour rc("makebg");
  
  vector<std::pair<int, int>> todo;
  {
    for (int y = 0; y < SQUARESH; y++)
      for (int x = 0; x < SQUARESW; x++)
	todo.emplace_back(x, y);

    Shuffle(&rc, &todo);
  }

  constexpr bool REQUIRE_BG_MATCH = true;  
  auto Same = [&bg, &fg, &fill](std::pair<int, int> p0,
				std::pair<int, int> p1) {
      auto [x0, y0] = p0;
      auto [x1, y1] = p1;
      return (!REQUIRE_BG_MATCH ||
	      bg.GetPixel(x0, y0) == bg.GetPixel(x1, y1)) &&
	fg.GetPixel(x0, y0) == fg.GetPixel(x1, y1) &&
	fill.GetPixel(x0, y0) == fill.GetPixel(x1, y1);
  };

  // Can't use unordered_set without custom hash...
  std::set<std::pair<int, int>> done;
  for (const auto [x, y] : todo) {
    if (done.find({x, y}) == done.end()) {
      // In order to join across cells, the foreground and
      // fill style must match, as well as the background
      // if that option is enabled.

      // We'll find a rectangle (1xN or Nx1) that includes
      // x,y but potentially extended to neighbors. Inclusive.

      auto Extend = [&bg, &fg, &fill, &rc, &Same, &done, x, y]() ->
	std::tuple<int, int, int, int> {
	  vector<Dir> dirs = {UP, DOWN, LEFT, RIGHT};
	  Shuffle(&rc, &dirs);

	  for (Dir dir : dirs) {
	    auto neighbor = Move(x, y, dir);
	    if (neighbor.has_value() &&
		done.find(neighbor.value()) == done.end() &&
		Same({x, y}, neighbor.value())) {
	      // XXX continue extending...
	      int x0 = x, y0 = y;
	      auto [x1, y1] = neighbor.value();
	      // printf("Extended %d,%d->%d,%d\n", x0, y0, x1, y1);
	      return {std::min(x0, x1), std::min(y0, y1),
		      std::max(x0, x1), std::max(y0, y1)};
	    }
	  }
	  // Normal to find no extensions.
	  return {x, y, x, y};
	};

      
      auto [x0, y0, x1, y1] = Extend();

      // Same fill for entire box, so use x0, y0;
      const uint32 fg_color = ToColor(fg.GetPixel(x0, y0));
      const uint32 fg_lite = (fg_color & 0xFFFFFF00) | 0x7F;

      int xx = x0 * SQUARE;
      int yy = y0 * SQUARE;
      int w = x1 - x0 + 1;
      int h = y1 - y0 + 1;


      switch (fill.GetPixel(x0, y0)) {
      case DOT:
	FilledBox(xx + 1, yy + 1, (SQUARE * w) - 2, (SQUARE * h) - 2, fg_color, fg_lite);
	break;
      default:
      case LOOP:
	Box(xx + 1, yy + 1, (SQUARE * w) - 2, (SQUARE * h) - 2, fg_color, fg_lite);
	break;
      }

      // Mark as done so we don't encroach.
      for (int xx = x0; xx <= x1; xx++)
	for (int yy = y0; yy <= y1; yy++)
	  done.emplace(xx, yy);
      
    }
  }

  
#if 0
  for (int row = 0; row < SQUARESH; row++) {
    int y = row * SQUARE;
    for (int col = 0; col < SQUARESW; col++) {
      int x = col * SQUARE;

      uint32 fg_color = ToColor(fg.GetPixel(col, row));
      uint32 bg_color = ToColor(bg.GetPixel(col, row));      
      
      uint32 fg_lite = (fg_color & 0xFFFFFF00) | 0x7F;
      
      switch (fill.GetPixel(col, row)) {
      case DOT:
	img.BlendRect32(x, y, SQUARE, SQUARE, bg_color);
	FilledBox(x + 1, y + 1, SQUARE - 2, SQUARE - 2, fg_color, fg_lite);
	break;
      default:
      case LOOP:
	img.BlendRect32(x, y, SQUARE, SQUARE, bg_color);
	Box(x + 1, y + 1, SQUARE - 2, SQUARE - 2, fg_color, fg_lite);
	break;
      }
    }
  }
#endif

  img.Save("makebg.png");
  
  return 0;
}
