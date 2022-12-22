// Test out 1D dithering

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

#include "makebg-util.h"

constexpr int WIDTH = 1024;
constexpr int HEIGHT = 32;

int main(int argc, char **argv) {

  ImageRGBA img(WIDTH, HEIGHT);
  img.Clear32(0x000000ff);

  float err = 0.0;
  for (int x = 0; x < WIDTH; x++) {
    // In [0,1]
    // Target value (i.e. input image, a gradient)
    float f = x / (float)(WIDTH - 1);

    {
      // For comparison, 8-bit value
      uint8 f8 = roundf(f * 255.0f);
      for (int y = HEIGHT / 2; y < HEIGHT; y++)
        img.SetPixel(x, y, f8, f8, f8, 0xFF);
    }

    // Accumulate error
    float ef = f + err;
    // Quantized value
    float qv = ef > 0.5f ? 1.0f : 0.0f;
    // Carry forward error. 
    err = ef - qv;

    {
      uint8 ef8 = roundf(qv * 255.0f);
      for (int y = 0; y < HEIGHT / 2; y++)
        img.SetPixel(x, y, ef8, ef8, ef8, 0xFF);
    }
  }
  
  img.Save("dither.png");
    
  return 0;
}
