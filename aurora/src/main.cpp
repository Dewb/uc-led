#include <FastLED.h>
#include <plasma.h>

#define LED_PIN     10
#define CLOCK_PIN   9
#define COLOR_ORDER BGR
#define CHIPSET     WS2801
#define NUM_LEDS    32

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 120

bool gReverseDirection = false;

CRGB leds[NUM_LEDS];
CRGBPalette16 gPalette;
static int cells[NUM_LEDS];
long frameCount = 100;

void setup() {
  delay(500); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );

  gPalette = CRGBPalette16(CRGB(20, 10, 0), CRGB::Yellow, CRGB::Green, CRGB::SeaGreen);

  // initialize with some activity
  cells[0] = 180;
  cells[1] = 190;
  cells[2] = 200;
  cells[10] = 190;
  cells[11] = 180;
  cells[12] = 190;
  cells[22] = 180;
  cells[20] = 200;
  cells[21] = 180;

}

void loop()
{
    random16_add_entropy(random(561));
    int threshhold = 170;
    int boost_low = 80;
    int boost_high = 290;
    int reduce_low = 80;
    int reduce_high = 300;

    // simple continuous CA based loosely on Rule 30
    for(int c = 0; c < NUM_LEDS; c++) {
        
        int neighbors = 0;
        
        if (c > 0 && cells[c-1] > threshhold) {
            neighbors++;
        }
        if (c < NUM_LEDS - 1 && cells[c+1] > threshhold) {
            neighbors++;
        }
        if (cells[c] > threshhold && random(0, 10) == 1) { 
            neighbors++;
        }

        if (neighbors == 1) {
            cells[c] += random(boost_low, boost_high)/100;
        } else {
            cells[c] -= random(reduce_low, reduce_high)/100;
        }

        if (cells[c] < 1) {
          cells[c] = 1;
        } else if (cells[c] > 255) {
          cells[c] = 255;
        }
    }

    for(int i = 0; i < 6; i++) {
      byte sensor = analogRead(i);
      int origin = NUM_LEDS/6*i;
      cells[origin] += sensor / 120;
    }

    for(int i = 0; i < NUM_LEDS; i++) {
      byte colorindex = scale8(cells[i], 255);
      CRGB color = ColorFromPalette(gPalette, colorindex);
      byte p = plasma(frameCount, i, 1);
      color.r = scale8_video(color.r, dim8_video(p));
      color.g = scale8_video(color.g, dim8_video(p));
      color.b = scale8_video(color.b, dim8_video(p));

      if (color.r < 3) color.r = 3;
      if (color.g < 3) color.g = 3;
      
      leds[i] = color;
    }

    // Keep first two LEDs on pigtail off
    leds[0] = CRGB::Black;
    leds[1] = CRGB::Black;
  
    FastLED.show(); 
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    frameCount++;
}
