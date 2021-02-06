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
constexpr int BORDER = 6;

constexpr bool MOCKUP = false;

struct TTFont {

  TTFont(const string &filename) {
    ttf_bytes = Util::ReadFileBytes(filename);
    CHECK(!ttf_bytes.empty()) << filename;

    stbtt_InitFont(&font, ttf_bytes.data(),
		   stbtt_GetFontOffsetForIndex(ttf_bytes.data(), 0));
  }

  // Not cached, so this does a lot more allocation than you probably want.
  ImageA GetChar(char c, int size) {
    int width, height;
    uint8 *bitmap = stbtt_GetCodepointBitmap(
	&font, 0, stbtt_ScaleForPixelHeight(&font, size),
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
  // XXX y position is the baseline, I think, but I have not really tested this.
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
      if (bitmap != nullptr) {
	for (int yy = 0; yy < bitmap_h; yy++) {
	  for (int xx = 0; xx < bitmap_w; xx++) {
	    DrawPixel(xpos + xx + xoff, ypos + yy + yoff,
		      bitmap[yy * bitmap_w + xx]);
	  }
	}
	stbtt_FreeBitmap(bitmap, nullptr);
      }

      xpos += advance * scale;
      if (text[idx + 1] != '\0') {
	xpos += scale *
	  stbtt_GetCodepointKernAdvance(&font, text[idx], text[idx + 1]);
      }

      if (!subpixel) {
	// Or floor?
	xpos = roundf(xpos);
      }
    }
  }

  // Measure the nominal width and height of the string using the same
  // method as above. (This does not mean that all pixels lie within
  // the rectangle.)
  std::pair<int, int> MeasureString(const string &text,
				    int size_px, bool subpixel = true) {
    const float scale = stbtt_ScaleForPixelHeight(&font, size_px);

    int ascent = 0, descent = 0, line_gap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);

    float xpos = 0.0f;
    for (int idx = 0; idx < (int)text.size(); idx++) {

      int advance = 0, left_side_bearing = 0;
      stbtt_GetCodepointHMetrics(&font, text[idx],
				 &advance, &left_side_bearing);

      xpos += advance * scale;
      if (text[idx + 1] != '\0') {
	xpos += scale *
	  stbtt_GetCodepointKernAdvance(&font, text[idx], text[idx + 1]);
      }

      if (!subpixel) {
	// Or floor?
	xpos = roundf(xpos);
      }
    }
    return {xpos, ascent - descent + line_gap};
  }

private:
  vector<uint8> ttf_bytes;
  stbtt_fontinfo font;

};


int main(int argc, char **argv) {
  Blue blue;
  // Looks good with size=14, subpixel false.
  // Drawing (x,y) at (x,y) and (x+1,y) also looks
  // good for "faux bold."
  TTFont snoot("../fonts/px10.ttf");

  // Color schemes.

  // Native yellow.
  const std::unordered_map<uint32, uint32> SCHEME_NATIVE = {};

  // Intense!
  const std::unordered_map<uint32, uint32> SCHEME_RED = {
    // bright yellow
    {0xFFFC00FF, 0xFF0000FF},
    // yellow-brown corners
    {0x978d08FF, 0xba0b05FF},
    // lighter brown lines
    {0x64560cFF, 0x7e140aFF},
    // very dark brown outlines
    {0x312010FF, 0x311010FF},
  };

  // Pretty good contrast but not annoying.
  const std::unordered_map<uint32, uint32> SCHEME_ORANGEY = {
    // bright yellow
    {0xFFFC00FF, 0xFF8B00FF},
    // yellow-brown corners
    {0x978d08FF, 0xBF6e00FF},
    // lighter brown lines
    {0x64560cFF, 0x885600FF},
    // very dark brown outlines
    {0x312010FF, 0x312410FF},
  };


  const auto SCHEME = SCHEME_ORANGEY;
  const auto LEARN_SCHEME = SCHEME_NATIVE;

  ImageRGBA img(WIDTH, HEIGHT);
  img.Clear32(0x000000ff);

  // TODO: 2x version?
  auto DrawText = [&snoot, &img](int px, int py, uint32 color,
				 const string &text) {
      snoot.BlitString(px, py, 14, text,
		       [&](int x, int y, uint8 v) {
			 if (v > 128) {
			   img.BlendPixel32(x, y, color);
			 }
		       },
		       false);
    };

  auto TextWidth = [&snoot](const string &text) {
      const auto [w, h] = snoot.MeasureString(text, 14, false);
      return w;
    };

  // Note that 'bold' versions over-draw, so alpha should be 0xFF
  // unless you like a weird effect.
  auto DrawBoldText = [&snoot](ImageRGBA *img,
			       int px, int py, uint32 color,
			       const string &text) {
      snoot.BlitString(px, py, 14, text,
		       [&](int x, int y, uint8 v) {
			 if (v > 128) {
			   img->BlendPixel32(x, y, color);
			   img->BlendPixel32(x + 1, y, color);
			 }
		       },
		       false);
    };

  auto DrawBoldText2x = [&snoot, &img](int px, int py, uint32 color,
				       const string &text) {
      snoot.BlitString(0, 0, 14, text,
		       [&](int x, int y, uint8 v) {
			 if (v > 128) {
			   int xx = px + (x * 2);
			   int yy = py + (y * 2);
			   auto Px2x = [&](int x, int y) {
			       img.BlendPixel32(x, y, color);
			       img.BlendPixel32(x + 1, y, color);
			       img.BlendPixel32(x, y + 1, color);
			       img.BlendPixel32(x + 1, y + 1, color);
			     };

			   Px2x(xx, yy);
			   Px2x(xx + 2, yy);
			 }
		       },
		       false);
    };

  constexpr bool SLIDER_DISABLED[] = {
    false, // "rate",
    false, // "pulse width",
    false, // "slope",
    false, // "floor",
    true,  // "crossover frequency",
    false, // "stereo spread",
    false, // "rupture",
    false, // "tempo",
  };

  constexpr bool SLIDER_LEARN[] = {
    false, // "rate",
    false, // "pulse width",
    false, // "slope",
    false, // "floor",
    false, // "crossover frequency",
    false, // "stereo spread",
    true, // "rupture",
    false, // "tempo",
  };
  
  constexpr uint32 color1 = 0x634021ff;
  constexpr uint32 color2 = 0x8d3d6cff;
  // constexpr uint32 color1 = 0xffff00ff;
  // constexpr uint32 color2 = 0x5959ffff;

  // Location/size of the control rows. Lots of stuff is keyed off this.
  constexpr int CTRL_X = 12;
  constexpr int CTRL_Y = 78;

  constexpr int CTRL_W = 400;
  constexpr int CTRL_H = 64;

  constexpr int NUM_SLIDERS = 8;

  // Position of the slider backgrounds within the control rows,
  // which provide another key line.
  constexpr int SLIDER_X = 48;
  constexpr int SLIDER_Y = 22;
  constexpr int SLIDER_H = 28;
  constexpr int SLIDER_W = 270;

  #define FULLWIDTH_HELP false

#if FULLWIDTH_HELP
  constexpr int HELP_W = WIDTH - (BORDER * 2) - 16;
  constexpr int HELP_H = 64;
  constexpr int HELP_X = (WIDTH - HELP_W) / 2;
  constexpr int HELP_Y = HEIGHT - HELP_H - BORDER - 22;
#else
  constexpr int HELP_LEFT_PADDING = CTRL_X + SLIDER_X;
  constexpr int HELP_W = WIDTH - HELP_LEFT_PADDING - BORDER - 7;
  constexpr int HELP_H = 64;
  constexpr int HELP_X = HELP_LEFT_PADDING;
  constexpr int HELP_Y = HEIGHT - HELP_H - BORDER - 22;
#endif


  // Vertical "skid" stripes
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

      uint32 c = color;

      // Don't allow stripes over the left and right
      // border pinstripes; they just create distracting
      // artifacts.
      // (Maybe it would have been simpler to just draw color1
      // and color2 boxes over the pinstripes after drawing bg..)
      if (x < BORDER) c = color1;
      if (x >= WIDTH - BORDER) c = color2;

      // So too in help.
      if (y > HELP_Y - BORDER && y < HELP_Y + HELP_H + BORDER - 2) {
	// c = 0xFF0000FF;

	if (x > HELP_X - BORDER && x <= HELP_X)
	  c = color1;

	// This is just right aligned.
	if (x >= HELP_X + HELP_W) c = color2;
      }

      img.SetPixel32(x, y, c);
    }
  }

  if (false) {
    // visualize the rows for "debugging"
    constexpr uint32 color1 = 0xFFFF000F;
    constexpr uint32 color2 = 0x0073FF0F;

    for (int i = 0; i < NUM_SLIDERS; i++) {
      img.BlendRect32(CTRL_X, CTRL_Y + i * CTRL_H,
		      CTRL_W, CTRL_H,
		      (i & 1) ? color1 : color2);
    }
  }


  // Pass corner_color = color for a crisp box, but setting
  // the corners to 50% alpha makes a nice roundrect effect.
  auto Box = [&img](int x, int y, int w, int h,
		    uint32 color, uint32 corner_color) {
      img.BlendBox32(x, y, w, h, color, corner_color);
    };

  // Standardized on 1/3 alpha
  auto Disable = [](const ImageRGBA &src) {
      ImageRGBA disabled(src.Width(), src.Height());
      for (int y = 0; y < src.Height(); y++) {
	for (int x = 0; x < src.Width(); x++) {
	  auto [r, g, b, a] = src.GetPixel(x, y);
	  disabled.SetPixel(x, y, r, g, b, a * 0.3333f);
	}
      }
      return disabled;
    };

  ImageRGBA slider(SLIDER_W, SLIDER_H);

  {
    auto DarkNoiseRect = [&blue](ImageRGBA *img, int sx, int sy, int w, int h) {
	for (int y = 0; y < h; y++) {
	  for (int x = 0; x < w; x++) {
	    uint8 v = blue.Get(sx + x, sy + (y / 4));
	    uint32 color = 0x00000000 | (128 + (v >> 1));
	    img->BlendPixel32(sx + x, sy + y, color);
	  }
	}
      };

    slider.Clear32(color2);

    slider.BlendBox32(0, 0, SLIDER_W, SLIDER_H, 0x000000FF, 0x0000001F);
    slider.BlendBox32(1, 1, SLIDER_W - 2, SLIDER_H - 2, 0x000000FF, 0x000000FF);

    DarkNoiseRect(&slider, 2, 2, SLIDER_W - 4, SLIDER_H - 4);

    // just for the inner corners
    slider.BlendBox32(2, 2, SLIDER_W - 4, SLIDER_H - 4, 0x00000000, 0x0000003F);
  };
  ImageRGBA disabled_slider = Disable(slider);

  auto DrawSlider = [&img, &slider, &disabled_slider](
      int sx, int sy, bool disabled = false) {
      img.BlendImage(sx, sy, disabled ? disabled_slider : slider);
    };

  // These are not baked into the background because we want to be able to
  // draw them transparent for the disabled state.
  if (MOCKUP) {
    for (int i = 0; i < NUM_SLIDERS; i++) {
      DrawSlider(CTRL_X + SLIDER_X,
		 CTRL_Y + (i * CTRL_H) + SLIDER_Y,
		 SLIDER_DISABLED[i]);
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
    constexpr int LABEL_Y = -8;
    for (int i = 0; i < NUM_SLIDERS; i++) {
      int lx = CTRL_X + LABEL_X;
      int ly = i * CTRL_H + CTRL_Y + LABEL_Y;

      // Using builtin bit7.
      // img.BlendText2x32(lx, ly, 0xFFFFFFFF, labels[i]);
      // Shadow?
      DrawBoldText2x(lx + 1, ly + 1, 0x000000FF, labels[i]);
      // Actual text
      DrawBoldText2x(lx, ly, 0xFFFFFFFF, labels[i]);
    }
  }

  // Parameter values.
  if (MOCKUP) {
    vector<pair<string, string>> values = {
      {"3.000", "4.476 Hz"}, // rate divisor
      {"39.7%", "100.0%"}, // pulse width
      {"", "16.384 ms"}, // slope
      {"-59.2", "-25.8 dB"},  // floor
      {"", "99.99 kHZ"}, // crossover
      {"", "50.7%"}, // stereo spread
      {"", "-10.7 dB"}, // rupture
      {"", "214.04 bpm"}, // tempo
    };
    CHECK(values.size() >= NUM_SLIDERS);

    constexpr int LVALUE_X = 8;
    constexpr int RVALUE_X = SLIDER_X + SLIDER_W + 4;
    constexpr int VALUE_Y = 28;
    for (int i = 0; i < NUM_SLIDERS; i++) {
      const uint32 color = SLIDER_DISABLED[i] ? 0xFFFFFF55 : 0xFFFFFFFF;

      int lx = CTRL_X + LVALUE_X;
      int ly = i * CTRL_H + CTRL_Y + VALUE_Y;
      DrawText(lx, ly, color, values[i].first);

      int rx = CTRL_X + RVALUE_X;
      int ry = i * CTRL_H + CTRL_Y + VALUE_Y;
      DrawText(rx, ry, color, values[i].second);
    }
  }

  // Slider mode buttons.
  if (MOCKUP) {
    ImageRGBA beat_sync_button =
	Recolor(SCHEME, CroppedButton("skidder-beat-sync-button.png", 2, 0));
    // Demo 'all' state and disable crossover slider
    ImageRGBA crossover_mode_button =
	Recolor(SCHEME, CroppedButton("skidder-crossover-mode-button.png", 3, 0));
    // or this one...
    ImageRGBA tempo_sync_button =
	Recolor(SCHEME, CroppedButton("skidder-tempo-sync-button.png", 2, 0));
    std::vector<std::optional<ImageRGBA>> buttons = {
      {beat_sync_button}, // rate
      nullopt, // pulsewidth
      nullopt, // slope
      nullopt, // floor
      {crossover_mode_button}, // crossover
      nullopt, // stereo
      nullopt, // rupture
      {tempo_sync_button}, // TODO tempo
    };

    constexpr int BUTTON_Y = 24;
    for (int i = 0; i < (int)buttons.size(); i++) {
      if (buttons[i].has_value()) {
	const ImageRGBA &button = buttons[i].value();
	const int bx = WIDTH - BORDER - 3 - button.Width();
	const int by = i * CTRL_H + CTRL_Y + BUTTON_Y;
	img.BlendImage(bx, by, button);
      }
    }
  }


  // help zone
  {
    img.BlendRect32(HELP_X, HELP_Y, HELP_W, HELP_H,
		    0x0000005F);

    // help pinstripes
    for (int b = 0; b < 5; b++) {
      if (!(b & 1)) {
	Box(HELP_X - b, HELP_Y - b,
	    HELP_W + (b * 2), HELP_H + (b * 2),
	    0x0000007F, 0x0000004F);
      }
    }

    // border pinstripes
    for (int b = 0; b < BORDER; b++) {
      if (b & 1) {
	Box(b, b, WIDTH - (b * 2), HEIGHT - (b * 2),
	    0x000000FF, 0x000000AF);
      }
    }

    if (MOCKUP) {
      DrawText(HELP_X + 4, HELP_Y + 4,
	       0xFFFFFFFF,
	       "You can get some help in the help box!");
    }
  }



  // Title
  {
    ImageRGBA title = LoadImage("skidder-title.png");
    // const int TITLE_X = WIDTH - title.Width() - BORDER + 2;
    // hang 'd' slightly over the sliders ("optical alignment")
    const int TITLE_X = CTRL_X + SLIDER_X - 8;
    const int TITLE_Y = BORDER + 5;
    img.BlendImage(TITLE_X, TITLE_Y, title);
  }

  // MIDI controls at the bottom
  {
    const int MIDI_LABEL_X = CTRL_X + 48;
    const int MIDI_LABEL_Y = CTRL_Y + CTRL_H * NUM_SLIDERS;

    DrawBoldText2x(MIDI_LABEL_X + 1, MIDI_LABEL_Y + 1, 0x000000FF, "MIDI mode");
    DrawBoldText2x(MIDI_LABEL_X, MIDI_LABEL_Y, 0xFFFFFFFF, "MIDI mode");

    constexpr int MIDI_BUTTON_X = MIDI_LABEL_X;
    constexpr int MIDI_BUTTON_Y = MIDI_LABEL_Y + 18 + 12;

    if (MOCKUP) {
      // constexpr int BUTTON_HEIGHT = 22;
      ImageRGBA mode_button =
	Recolor(SCHEME, CroppedButton("skidder-midi-mode-button.png", 3, 0));
      img.BlendImage(MIDI_BUTTON_X, MIDI_BUTTON_Y, mode_button);

      constexpr int VELOCITY_X = 41;
      constexpr int VELOCITY_Y_MARGIN = 4;

      ImageRGBA vel_button =
	Recolor(SCHEME, CroppedButton("skidder-use-velocity-button.png", 2, 0));
      img.BlendImage(MIDI_BUTTON_X + VELOCITY_X,
		     MIDI_BUTTON_Y + mode_button.Height() + VELOCITY_Y_MARGIN,
		     vel_button);

      constexpr int LEARN_X = 221;
      constexpr int LEARN_Y = MIDI_BUTTON_Y;

      constexpr int RESET_X = 295;
      constexpr int RESET_Y = MIDI_BUTTON_Y;

      ImageRGBA learn_button =
	Recolor(SCHEME, CroppedButton("skidder-midi-learn-button.png", 2, 0));
      ImageRGBA reset_button =
	Recolor(SCHEME, CroppedButton("skidder-midi-reset-button.png", 2, 0));

      img.BlendImage(LEARN_X, LEARN_Y, learn_button);
      img.BlendImage(RESET_X, RESET_Y, reset_button);
    }
  }

  // Slider handles
  if (MOCKUP) {
    ImageRGBA splittable_handle =
      Recolor(SCHEME, LoadImage("skidder-splittable-handle.png"));
    CHECK(splittable_handle.Width() % 2 == 0);
    const int SPLIT_HANDLE_WIDTH = splittable_handle.Width() / 2;
    ImageRGBA handle_left =
      splittable_handle.Crop32(0, 0, SPLIT_HANDLE_WIDTH,
			       splittable_handle.Height());
    ImageRGBA handle_right =
      splittable_handle.Crop32(SPLIT_HANDLE_WIDTH, 0, SPLIT_HANDLE_WIDTH,
			       splittable_handle.Height());

    ImageRGBA handle = Recolor(SCHEME, LoadImage("skidder-handle.png"));
    ImageRGBA handle_learn = Recolor(LEARN_SCHEME, LoadImage("skidder-handle-learn.png"));
    const int HANDLE_WIDTH = handle.Width();
    ImageRGBA handle_disabled = Disable(handle);

    // If second is 0, handle is not split.

    constexpr int STARTX = CTRL_X + SLIDER_X;
    vector<pair<int, int>> handles = {
      // rate - splittable
      {STARTX + 40, STARTX + 40 + SPLIT_HANDLE_WIDTH},
      // pulse width - splittable
      {STARTX + 80, STARTX + 120 + SPLIT_HANDLE_WIDTH},
      // slope
      {STARTX + 20, 0},
      // floor - splittable
      {STARTX, STARTX + SPLIT_HANDLE_WIDTH},
      // crossover
      {STARTX + SLIDER_W - HANDLE_WIDTH * 3, 0},
      // stereo
      {STARTX, 0},
      // rupture
      {STARTX + SLIDER_W - HANDLE_WIDTH, 0},
      // tempo
      {STARTX + 75, 0},
    };

    {
      constexpr int HANDLE_YMARGIN = -3;
      int ypos = CTRL_Y + SLIDER_Y;
      for (int i = 0; i < (int)handles.size(); i++) {
	const auto [x1, x2] = handles[i];
	if (x2 == 0) {
	  // special states only mocked for regular handle
	  if (SLIDER_DISABLED[i]) {
	    img.BlendImage(x1, ypos + HANDLE_YMARGIN,
			   handle_disabled);
	  } else if (SLIDER_LEARN[i]) {
	    img.BlendImage(x1, ypos + HANDLE_YMARGIN,
			   handle_learn);
	  } else {
	    img.BlendImage(x1, ypos + HANDLE_YMARGIN,
			   handle);
	  }

	} else {
	  img.BlendImage(x1, ypos + HANDLE_YMARGIN, handle_left);
	  img.BlendImage(x2, ypos + HANDLE_YMARGIN, handle_right);
	}
	ypos += CTRL_H;
      }
    }
  }

  // destroyfx link
  const string dfx_text = "destroyfx.org";
  const int DFX_WIDTH = TextWidth(dfx_text);
  const int DFX_HEIGHT = 16;
  ImageRGBA dfx_button(DFX_WIDTH, DFX_HEIGHT * 2);
  // normal state
  DrawBoldText(&dfx_button, 1, 1, 0x000000FF, dfx_text);
  DrawBoldText(&dfx_button, 0, 0, 0xFFFFFFFF, dfx_text);
  // pressed state
  DrawBoldText(&dfx_button, 1, DFX_HEIGHT + 1, 0xFFFF00FF, dfx_text);

  if (MOCKUP) {
    ImageRGBA button_normal = dfx_button.Crop32(0, 0, DFX_WIDTH, DFX_HEIGHT);
    int lx = WIDTH - DFX_WIDTH - 4 - BORDER + 1;
    int ly = HEIGHT - 16 - BORDER - 1;
    img.BlendImage(lx, ly, button_normal);
  }

  img.Save("skidder.png");

  if (!MOCKUP) {
    string asset_dir = "skidder-assets/";
    img.Save(asset_dir + "background.png");
    slider.Save(asset_dir + "slider-background.png");

    dfx_button.Save(asset_dir + "destroy-fx-link.png");
    
    ImageRGBA handle = Recolor(SCHEME, LoadImage("skidder-handle.png"));
    ImageRGBA handle_learn = Recolor(LEARN_SCHEME, LoadImage("skidder-handle-learn.png"));
    handle.Save(asset_dir + "slider-handle.png");
    handle_learn.Save(asset_dir + "slider-handle-learn.png");

    const ImageRGBA splittable_handle =
      Recolor(SCHEME, LoadImage("skidder-splittable-handle.png"));
    const int split_w = splittable_handle.Width() >> 1;
    const int split_h = splittable_handle.Height();
    splittable_handle.Crop32(0, 0, split_w, split_h).Save(
	asset_dir + "range-slider-handle-left.png");
    splittable_handle.Crop32(split_w, 0, split_w, split_h).Save(
	asset_dir + "range-slider-handle-right.png");

    const ImageRGBA splittable_handle_learn =
      Recolor(LEARN_SCHEME, LoadImage("skidder-splittable-handle-learn.png"));
    CHECK(splittable_handle_learn.Width() == splittable_handle.Width());
    splittable_handle_learn.Crop32(0, 0, split_w, split_h).Save(
	asset_dir + "range-slider-handle-left-learn.png");
    splittable_handle_learn.Crop32(split_w, 0, split_w, split_h).Save(
	asset_dir + "range-slider-handle-right-learn.png");

    
    Recolor(SCHEME, LoadImage("skidder-beat-sync-button.png")).Save(
	asset_dir + "beat-sync-button.png");
    Recolor(SCHEME, LoadImage("skidder-crossover-mode-button.png")).Save(
	asset_dir + "crossover-mode-button.png");
    Recolor(SCHEME, LoadImage("skidder-tempo-sync-button.png")).Save(
	asset_dir + "tempo-sync-button.png");


    Recolor(SCHEME, LoadImage("skidder-midi-learn-button.png")).Save(
	asset_dir + "midi-learn-button.png");
    Recolor(SCHEME, LoadImage("skidder-midi-mode-button.png")).Save(
	asset_dir + "midi-mode-button.png");
    Recolor(SCHEME, LoadImage("skidder-midi-reset-button.png")).Save(
	asset_dir + "midi-reset-button.png");
    Recolor(SCHEME, LoadImage("skidder-use-velocity-button.png")).Save(
	asset_dir + "use-velocity-button.png");
  }
    
  return 0;
}
