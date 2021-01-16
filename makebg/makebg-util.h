
#ifndef _DESTROYFX_MAKEBG_MAKEBG_UTIL_H
#define _DESTROYFX_MAKEBG_MAKEBG_UTIL_H


#include <cstdint>
#include <utility>
#include <memory>

#include "base/logging.h"
#include "color-util.h"
#include "image.h"

using namespace std;

using uint8 = uint8_t;
using uint32 = uint32_t;

inline ImageRGBA LoadImage(const std::string &filename) {
  // TODO: image.h should use value semantics, maybe
  // Load returns optional<>
  std::unique_ptr<ImageRGBA> img(ImageRGBA::Load(filename));
  CHECK(img.get() != nullptr) << "Couldn't load " << filename;
  ImageRGBA ret = *img;
  return ret;
}

// Mapping should specify alpha as FF always.
inline ImageRGBA Recolor(const std::unordered_map<uint32, uint32> &mapping,
			 const ImageRGBA &img) {
  ImageRGBA ret(img.Width(), img.Height());

  for (int y = 0; y < img.Height(); y++) {
    for (int x = 0; x < img.Width(); x++) {
      uint32 color = img.GetPixel32(x, y);
      uint32 rgb = color | 0xFF;
      uint32 alpha = color & 0xFF;
      auto it = mapping.find(rgb);
      if (it != mapping.end()) {
	// Keep original alpha.
	color = (it->second & ~0xFF) | alpha;
      }
      ret.SetPixel32(x, y, color);
    }
  }
  return ret;
}

struct Blue {
  static constexpr int SIZE = 470;
  Blue() : noise(SIZE, SIZE) {
    // The image is actually RGBA; convert to single-channel.
    // This texture comes from "Free blue noise textures":
    //   http://momentsingraphics.de/BlueNoise.html
    std::unique_ptr<ImageRGBA> rgba(
	ImageRGBA::Load("bluenoise470.png"));
    CHECK(rgba.get());
    CHECK(rgba->Width() == SIZE);
    CHECK(rgba->Height() == SIZE);
    for (int y = 0; y < SIZE; y++) {
      for (int x = 0; x < SIZE; x++) {
	uint32 p = rgba->GetPixel32(x, y);
	// Assumes all the channels are the same.
	uint8 v = p >> 24;
	noise.SetPixel(x, y, v);
      }
    }
  }

  uint8 Get(int x, int y) const {
    // XXX should support negative values as well
    return noise.GetPixel(x % SIZE, y % SIZE);
  }

 private:
  ImageA noise;
};

inline uint32 Mix32(uint32 c1, uint32 c2, float f) {
  uint32 r1 = (c1 >> 24) & 0xFF;
  uint32 g1 = (c1 >> 16) & 0xFF;
  uint32 b1 = (c1 >>  8) & 0xFF;
  uint32 a1 = (c1      ) & 0xFF;

  uint32 r2 = (c2 >> 24) & 0xFF;
  uint32 g2 = (c2 >> 16) & 0xFF;
  uint32 b2 = (c2 >>  8) & 0xFF;
  uint32 a2 = (c2      ) & 0xFF;

  // Use integer multiplication so that we can shift to
  // divide.
  uint32 f32 = f * 65536.0;
  uint32 omf32 = 65536 - f32;
  auto Mix = [f32, omf32](uint32 ch1, uint32 ch2) -> uint8 {
      uint32 v1 = ch1 * f32;
      uint32 v2 = ch2 * omf32;

      uint32 r = v1 + v2;
      return (r >> 16) & 0xFF;
    };

  uint8 r = Mix(r1, r2);
  uint8 g = Mix(g1, g2);
  uint8 b = Mix(b1, b2);
  uint8 a = Mix(a1, a2);
  return (r << 24) | (g << 16) | (b << 8) | a;
}

// Adapted from image.h; would make sense as a utility...
// assumes below alpha is FF, and outputs FF alpha.
inline uint32 Blend32(uint32 below, uint32 atop) {
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

inline uint32 Grey32(uint32 c) {
  float r = ((c >> 24) & 0xFF) / (float)0xFF;
  float g = ((c >> 16) & 0xFF) / (float)0xFF;
  float b = ((c >>  8) & 0xFF) / (float)0xFF;

  float ll, aa, bb;
  ColorUtil::RGBToLAB(r, g, b, &ll, &aa, &bb);

  uint8 v = (ll / 100.0f) * 255.0f;
  return (v << 24) | (v << 16) | (v << 8) | 0xFF;
}

enum Dir { UP, DOWN, LEFT, RIGHT, };
inline std::pair<int, int> Move(int x, int y, Dir d) {
  switch (d) {
  default:
  case UP: return {x, y - 1};
  case DOWN: return {x, y + 1};
  case LEFT: return {x - 1, y};
  case RIGHT: return {x + 1, y};
  }
}

inline ImageRGBA CroppedButton(const std::string &filename,
			       int num_buttons,
			       int button_idx) {
  std::unique_ptr<ImageRGBA> full(ImageRGBA::Load(filename));
  CHECK(full.get() != nullptr) << filename;

  CHECK(full->Height() % num_buttons == 0) << "Wrong number of "
    "buttons in " << filename << "? Got height: " << full->Height();
  int bh = full->Height() / num_buttons;
  return full->Crop32(0, bh * button_idx, full->Width(), bh);
}
			       
#endif
