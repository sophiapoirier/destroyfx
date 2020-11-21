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

#include "makebg-util.h"

constexpr int WIDTH = 512;
constexpr int HEIGHT = 768;

int main(int argc, char **argv) {
  Blue blue;
  
  ImageRGBA img(WIDTH, HEIGHT);
  img.Clear32(0x000000ff);
    
  
  img.Save("skidder.png");
  
  return 0;
}
