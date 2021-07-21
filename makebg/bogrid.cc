// Generates the rezsynth (2020) background programmatically.

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <utility>
#include <optional>
#include <set>

#include "base/logging.h"
#include "base/stringprintf.h"

#include "lines.h"
#include "image.h"
#include "util.h"
#include "arcfour.h"
#include "randutil.h"
#include "color-util.h"

#include "makebg-util.h"

constexpr int WIDTH = 354;
constexpr int HEIGHT = 249;

constexpr uint32 LITE = 0xce6df1ff;
constexpr uint32 MED = 0x752194ff;
// constexpr uint32 DARK = 0x36034bff;
constexpr uint32 DARK = 0x5828b8ff;

// TODO: Could use alpha from color; move to image.h?
static void StippleLine32(ImageRGBA *img,
                          int x1, int y1, int x2, int y2,
                          // 'alpha' in [0,1].
                          float f,
                          uint32 color) {
  float err = 0.0;
  for (const auto [x, y] : Line<int>{x1, y1, x2, y2}) {
    float ef = f + err;
    bool q = ef > 0.5f;
    err = ef - (q ? 1.0f : 0.0f);

    if (q) {
      img->BlendPixel32(x, y, color);
    }
  }
}


int main(int argc, char **argv) {

  ImageRGBA img(WIDTH, HEIGHT);
  img.Clear32(MED);

  img.BlendBox32(0, 0, WIDTH, HEIGHT, DARK, MED);

#if 0  
  // Recursively subdivide
  std::function<void(int, float, float, float, float)> Rec =
    [&](int depth, float x, float y, float w, float h) {
      if (depth == 0) return;
      
      // img.BlendBox32(x, y, w, h, DARK & 0xFFFFFF33, DARK & 0xFFFFFF33);
      // draw cross in center of rectangle
      int cx = round(x + w * 0.5f);
      int cy = round(y + h * 0.5f);

      int lx = ceil(x + 0.5f);
      int ly = ceil(y + 0.5f);

      int rx = floor(x + w - 0.5f);
      int ry = floor(y + h - 0.5f);      

      int iw = rx - lx;
      int ih = ry - ly;      

      img.BlendLine32(cx, ly, cx, ry, DARK);
      img.BlendLine32(lx, cy, rx, cy, DARK);      

      float ww = w * 0.5f;
      float hh = h * 0.5f;
      // four quadrants
      Rec(depth - 1, x, y, ww, hh);
      Rec(depth - 1, x + ww, y, ww, hh);
      Rec(depth - 1, x, y + hh, ww, hh);
      Rec(depth - 1, x + ww, y + hh, ww, hh);

      img.BlendBox32(lx, ly, iw, ih, MED, MED);
    };

  Rec(4, 0.0f, 0.0f, (float)WIDTH, (float)HEIGHT);
#endif

#if 0
  // Recursively subdivide, integral
  std::function<void(int, int, int, int, int)> Rec =
    [&](int depth, int x, int y, int w, int h) {
      if (depth == 0) return;

      int BORD = 2;
      x += BORD;
      y += BORD;
      w -= 2 * BORD;
      h -= 2 * BORD;
      
      int ww = w >> 1;
      int hh = h >> 1;

      int cx = x + ww;
      int cy = y + hh;

      img.BlendLine32(cx, y, cx, y + h, DARK);
      img.BlendLine32(x, cy, x + w, cy, DARK);      

      // four quadrants
      Rec(depth - 1, x, y, ww, hh);
      Rec(depth - 1, x + ww, y, ww, hh);
      Rec(depth - 1, x, y + hh, ww, hh);
      Rec(depth - 1, x + ww, y + hh, ww, hh);

      // img.BlendBox32(lx, ly, iw, ih, MED, MED);
    };

  Rec(4, 0, 0, WIDTH, HEIGHT);
#endif

#if 0
  constexpr float FADE = 0.75f;
  // Recursively subdivide, integral, stipple
  std::function<void(int, int, int, int, int, float)> Rec =
    [&](int depth, int x, int y, int w, int h, float f) {
      if (depth == 0) return;

      int BORD = 2;
      x += BORD;
      y += BORD;
      w -= 2 * BORD;
      h -= 2 * BORD;
      
      int ww = w >> 1;
      int hh = h >> 1;

      int cx = x + ww;
      int cy = y + hh;

      // probably should draw four segments, from the center;
      // it looks bad if the x/y stippling don't line up
      StippleLine32(&img, cx, y, cx, y + h, f, DARK);
      StippleLine32(&img, x, cy, x + w, cy, f, DARK);

      float ff = f * FADE;
      // four quadrants
      Rec(depth - 1, x, y, ww, hh, ff);
      Rec(depth - 1, x + ww, y, ww, hh, ff);
      Rec(depth - 1, x, y + hh, ww, hh, ff);
      Rec(depth - 1, x + ww, y + hh, ww, hh, ff);

      // img.BlendBox32(lx, ly, iw, ih, MED, MED);
    };

  Rec(4, 0, 0, WIDTH, HEIGHT, 1.0f);
#endif
  
  constexpr float FADE = 0.85f;
  // Recursively subdivide, integral, stipple
  std::function<void(int, int, int, int, int, float)> Rec =
    [&](int depth, int x, int y, int w, int h, float f) {
      if (depth == 0) return;

      int BORD = 2;
      x += BORD;
      y += BORD;
      w -= 2 * BORD;
      h -= 2 * BORD;
      
      int ww = w >> 1;
      int hh = h >> 1;

      int cx = x + ww;
      int cy = y + hh;

      // probably should draw four segments, from the center;
      // it looks bad if the x/y stippling don't line up

      // up, down
      StippleLine32(&img, cx, cy, cx, cy - hh, f, DARK);
      StippleLine32(&img, cx, cy, cx, cy + hh, f, DARK);      
      // left, right
      StippleLine32(&img, cx, cy, cx - ww, cy, f, DARK);
      StippleLine32(&img, cx, cy, cx + ww, cy, f, DARK);      
      
      float ff = f * FADE;
      // four quadrants
      Rec(depth - 1, x, y, ww, hh, ff);
      Rec(depth - 1, x + ww, y, ww, hh, ff);
      Rec(depth - 1, x, y + hh, ww, hh, ff);
      Rec(depth - 1, x + ww, y + hh, ww, hh, ff);

      // img.BlendBox32(lx, ly, iw, ih, MED, MED);
    };

  Rec(4, 0, 0, WIDTH, HEIGHT, 1.0f);

  
  
  img.Save("bogrid.png");
  
  return 0;
}
