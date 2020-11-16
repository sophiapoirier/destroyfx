// Generate backgrounds for plugins programmatically.

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

using namespace std;

using uint8 = uint8_t;
using uint32 = uint32_t;


constexpr int SQUARE = 8;

constexpr int WIDTH = 512 + SQUARE;
constexpr int HEIGHT = 512 + SQUARE * 23;

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

// Adapted from image.h; would make sense as a utility...
// assumes below alpha is FF, and outputs FF alpha.
static uint32 Blend32(uint32 below, uint32 atop) {
  uint32 r = (atop >> 24) & 0xFF;
  uint32 g = (atop >> 16) & 0xFF;
  uint32 b = (atop >>  8) & 0xFF;  
  uint32 a = (atop      ) & 0xFF;  

  uint32 old_r = (below >> 24) & 0xFF;
  uint32 old_g = (below >> 16) & 0xFF;
  uint32 old_b = (below >> 8) & 0xFF;

  // so a + oma = 255.
  uint32 oma = 0xFF - a;

  uint32 rr = (((uint32)r * (uint32)a) + (old_r * oma)) / 0xFF;
  if (rr > 0xFF) rr = 0xFF;

  uint32 gg = (((uint32)g * (uint32)a) + (old_g * oma)) / 0xFF;
  if (gg > 0xFF) gg = 0xFF;

  uint32 bb = (((uint32)b * (uint32)a) + (old_b * oma)) / 0xFF;
  if (bb > 0xFF) bb = 0xFF;

  return (rr << 24) | (gg << 16) | (b << 8) | 0xFF;
}

static uint32 Grey32(uint32 c) {
  float r = ((c >> 24) & 0xFF) / (float)0xFF;
  float g = ((c >> 16) & 0xFF) / (float)0xFF;
  float b = ((c >>  8) & 0xFF) / (float)0xFF;  

  float ll, aa, bb;
  ColorUtil::RGBToLAB(r, g, b, &ll, &aa, &bb);

  uint8 v = (ll / 100.0f) * 255.0f;
  return (v << 24) | (v << 16) | (v << 8) | 0xFF;
}

enum Dir { UP, DOWN, LEFT, RIGHT, };
static std::pair<int, int> Move(int x, int y, Dir d) {
  switch (d) {
  default:
  case UP: return {x, y - 1};
  case DOWN: return {x, y + 1};
  case LEFT: return {x - 1, y};
  case RIGHT: return {x + 1, y};
  }
}

struct ThreeColors {
  uint32 one = 0xFF0000FF;
  uint32 two = 0x00FF00FF;
  uint32 three = 0x0000FFFF;
  ThreeColors(uint32 one, uint32 two, uint32 three) :
    one(one), two(two), three(three) {}

  ThreeColors Mix(uint32 other) {
    return ThreeColors(Blend32(one, other),
		       Blend32(two, other),
		       Blend32(three, other));
  }
  ThreeColors Grey() {
    return ThreeColors(Grey32(one),
		       Grey32(two),
		       Grey32(three));
  }
  
};

// Colorboxes are rectangles (as x,y,w,h in squares space) with
// a color assignment for the three colors called ONE, TWO, THREE.
//
// A colorbox also results in a fence around the rectangle, where
// we don't allow any joins across that border.
struct ColorBox {
  int x = 0, y = 0, w = 0, h = 0;
  ThreeColors colors;
  ColorBox(int x, int y, int w, int h,
	   uint32 one, uint32 two, uint32 three) :
    x(x), y(y), w(w), h(h), colors(one, two, three) {}
  ColorBox(int x, int y, int w, int h,
	   ThreeColors colors) :
    x(x), y(y), w(w), h(h), colors(colors) {}
};


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
    
  // Pass corner_color = color for a crisp box, but setting
  // the corners to 50% alpha makes a nice roundrect effect.
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
  
  // Colors are assigned according to the first containing
  // rectangle.
  std::vector<ColorBox> color_boxes;

  ThreeColors standard{0x3f6593ff, 0x03621aff, 0x209289ff};

  // Horizontal sliders.
  constexpr int SLIDER_X = 3;
  // Position of the label.
  constexpr int SLIDER_Y = 11;
  constexpr int SLIDER_W = 32;
  constexpr int SLIDER_STRIDE = 6;

  // Yellow group.
  ThreeColors slider_yellow = standard.Mix(0x858a003f);

  // Each slider is actually two rectangles (of the same color).
  // The bottom one is basically occluded by the slider itself, but
  // we don't want joins going into it.
  for (int i = 0; i < 3; i++) {
    int yy = SLIDER_Y + (i * SLIDER_STRIDE);
    color_boxes.emplace_back(SLIDER_X, yy, SLIDER_W, 2,
			     slider_yellow);
    color_boxes.emplace_back(SLIDER_X, yy + 2, SLIDER_W, 2,
			     slider_yellow);
  }

  ThreeColors slider_red = standard.Mix(0x8a00003f);
  for (int i = 3; i < 7; i++) {
    int yy = SLIDER_Y + (i * SLIDER_STRIDE);
    color_boxes.emplace_back(SLIDER_X, yy, SLIDER_W, 2,
			     slider_red);
    color_boxes.emplace_back(SLIDER_X, yy + 2, SLIDER_W, 2,
			     slider_red);
  }

  ThreeColors slider_grey = standard.Grey();
  for (int i = 7; i < 9; i++) {
    int yy = SLIDER_Y + (i * SLIDER_STRIDE);
    color_boxes.emplace_back(SLIDER_X, yy, SLIDER_W, 2,
			     slider_grey);
    color_boxes.emplace_back(SLIDER_X, yy + 2, SLIDER_W, 2,
			     slider_grey);
  }

  ThreeColors slider_pink = standard.Mix(0x9b009e3f);
  for (int i = 9; i < 10; i++) {
    int yy = SLIDER_Y + (i * SLIDER_STRIDE);
    color_boxes.emplace_back(SLIDER_X, yy, SLIDER_W, 2,
			     slider_pink);
    color_boxes.emplace_back(SLIDER_X, yy + 2, SLIDER_W, 2,
			     slider_pink);
  }

  // Midi learn / reset.
  // Background color here is not important since they are opaque.
  constexpr int MIDIBUTTON_Y = SLIDER_Y + 10 * SLIDER_STRIDE;
  constexpr int MIDIBUTTON_WIDTH = 11;
  for (int i = 0; i < 2; i++) {
    int xx = 3 + ((MIDIBUTTON_WIDTH + 2) * i);
    color_boxes.emplace_back(xx, MIDIBUTTON_Y, MIDIBUTTON_WIDTH, 2,
			     slider_pink);
  }

  // Button backgrounds.
  static constexpr int BUTTON_WIDTH = 12;
  static constexpr int BUTTON_TOP_HEIGHT = 2;
  auto Button = [&color_boxes, &standard](int x, int y, int bh = 2) {
      // Two parts. These are just for fencing; we use standard
      // background color.
      color_boxes.emplace_back(x, y, BUTTON_WIDTH, BUTTON_TOP_HEIGHT,
			       standard);
      color_boxes.emplace_back(x, y + BUTTON_TOP_HEIGHT,
			       BUTTON_WIDTH, bh,
			       standard);
    };

  constexpr int BUTTON_X = 37;
  Button(BUTTON_X, SLIDER_Y);
  Button(BUTTON_X, SLIDER_Y + SLIDER_STRIDE * 1);
  Button(BUTTON_X, SLIDER_Y + SLIDER_STRIDE * 2);
  Button(BUTTON_X, SLIDER_Y + SLIDER_STRIDE * 3);  

  constexpr int BUTTON2_X = BUTTON_X + BUTTON_WIDTH + 1;
  // Algorithm slider is taller.
  Button(BUTTON2_X, SLIDER_Y, 5);
  Button(BUTTON2_X, SLIDER_Y + SLIDER_STRIDE * 2);
  Button(BUTTON2_X, SLIDER_Y + SLIDER_STRIDE * 3);

  // Mix buttons.
  constexpr int MIX_BUTTON_Y = 69;
  Button(BUTTON_X, MIX_BUTTON_Y);
  Button(BUTTON2_X, MIX_BUTTON_Y);  

  // Vertical sliders
  constexpr int VSLIDER_X = 39;
  constexpr int VSLIDER_Y = 35;
  constexpr int VSLIDER_H = 30;
  constexpr int VSLIDER_STRIDE = 8;
  for (int i = 0; i < 3; i++) {
    ThreeColors slider_green = standard.Mix(0x1e8a323f);

    int xx = VSLIDER_X + i * VSLIDER_STRIDE;
    color_boxes.emplace_back(xx, VSLIDER_Y, 2, VSLIDER_H,
			     slider_green);
    color_boxes.emplace_back(xx + 2, VSLIDER_Y, 2, VSLIDER_H,
			     slider_green);

    // And labels beneath.
    color_boxes.emplace_back(xx - 1, VSLIDER_Y + VSLIDER_H + 1,
			     6, 2,
			     slider_green);
  }
  
  // Help box.
  constexpr int HELP_HEIGHT = 9;
  color_boxes.emplace_back(3, SQUARESH - 3 - HELP_HEIGHT,
			   SQUARESW - 6, HELP_HEIGHT,
			   standard.Mix(0x0000007F));

  // most of the background.
  color_boxes.emplace_back(1, 1, SQUARESW - 2, SQUARESH - 2, standard);  
  // Border goes last. We don't want this as four rectangles (with the
  // same color) because we do want corners to be able to join in either
  // direction.
  color_boxes.emplace_back(0, 0, SQUARESW, SQUARESH,
			   0x7400d2ff, 0x004d65ff, 0x000eb1ff);

  // Gets the index of the first matching rectangle. Can use the
  // indices to compute the fences, or to index into color_boxes to
  // get the color. Returns -1 if not in any rectangle.
  auto GetRect = [&color_boxes](int x, int y) {
      for (int i = 0; i < (int)color_boxes.size(); i++) {
	const ColorBox &box = color_boxes[i];
	if (x >= box.x && y >= box.y &&
	    x < box.x + box.w && y < box.y + box.h)
	  return i;
      }
      return -1;
    };
  
  // First, draw background colors.

  auto ToColor =
    [&color_boxes, &GetRect](int x, int y, uint8 v) -> uint32 {
      const int r = GetRect(x, y);
      if (r == -1) return 0xFFFF00FF;
      switch (v) {
      case ONE: return color_boxes[r].colors.one;
      case TWO: return color_boxes[r].colors.two;
      case THREE: return color_boxes[r].colors.three;
      default: return 0xFF00FFFF;
      }
    };

  
  for (int row = 0; row < SQUARESH; row++) {
    int y = row * SQUARE;
    for (int col = 0; col < SQUARESW; col++) {
      int x = col * SQUARE;
      uint32 bg_color = ToColor(col, row, bg.GetPixel(col, row));
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
  

  // Note: Can't use unordered_set without custom hash :(  
  std::set<std::pair<int, int>> done;  
  for (const auto [x, y] : todo) {
    if (done.find({x, y}) == done.end()) {
      // In order to join across cells, the foreground and
      // fill style must match, as well as the background
      // if that option is enabled.

      // The cells must be in the same colorbox, as well.

      // We'll find a rectangle (1xN or Nx1) that includes
      // x,y but potentially extended to neighbors. Inclusive.
      // (Right now we only extend one cell, which might be
      // the best-looking choice anyway.)
      auto Extend = [&bg, &fg, &fill, &rc, &GetRect,
		     &Same, &done, x, y]() ->
	std::tuple<int, int, int, int> {
	  vector<Dir> dirs = {UP, DOWN, LEFT, RIGHT};
	  Shuffle(&rc, &dirs);

	  int r = GetRect(x, y);
	  CHECK(r != -1);
	  for (Dir dir : dirs) {
	    const auto [nx, ny] = Move(x, y, dir);
	    // Note: Out-of-bounds rectangles yield -1, so this
	    // also does the bounds check for us.
	    int nr = GetRect(nx, ny);
	    if (r == nr &&
		done.find({nx, ny}) == done.end() &&
		Same({x, y}, {nx, ny})) {
	      // XXX continue extending...?
	      // printf("Extended %d,%d->%d,%d\n", x0, y0, x1, y1);
	      return {std::min(x, nx), std::min(y, ny),
		      std::max(x, nx), std::max(y, ny)};
	    }
	  }
	  // Normal to find no extensions.
	  return {x, y, x, y};
	};

      
      auto [x0, y0, x1, y1] = Extend();

      // Same fill for entire box, so use x0, y0;
      const uint32 fg_color = ToColor(x0, y0, fg.GetPixel(x0, y0));
      const uint32 fg_lite = (fg_color & 0xFFFFFF00) | 0x7F;

      int xx = x0 * SQUARE;
      int yy = y0 * SQUARE;
      int w = x1 - x0 + 1;
      int h = y1 - y0 + 1;


      switch (fill.GetPixel(x0, y0)) {
      case DOT:
	FilledBox(xx + 1, yy + 1, (SQUARE * w) - 2, (SQUARE * h) - 2,
		  fg_color, fg_lite);
	break;
      default:
      case LOOP:
	Box(xx + 1, yy + 1, (SQUARE * w) - 2, (SQUARE * h) - 2,
	    fg_color, fg_lite);
	break;
      }

      // Mark as done so we don't encroach.
      for (int xx = x0; xx <= x1; xx++)
	for (int yy = y0; yy <= y1; yy++)
	  done.emplace(xx, yy);
      
    }
  }

  
  img.Save("makebg.png");
  
  return 0;
}
