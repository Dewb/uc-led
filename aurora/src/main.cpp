#include <FastLED.h>
#include <plasma.h>

#define BRIGHTNESS  255
#define FRAMES_PER_SECOND 200

// Tweak these values for sensor response
#define SENSOR_SCALING 0.6 
#define SENSOR_THRESHOLD 30
#define SENSOR_GROWTH_INTENSITY 0.125

// Global palette
CRGBPalette16 gPalette;
#define PALETTE_COLOR1 CRGB(50, 30, 0)
#define PALETTE_COLOR2 CRGB::Yellow
#define PALETTE_COLOR3 CRGB::Green
#define PALETTE_COLOR4 CRGB::SeaGreen

// The LED addresses closest to the six sensors
int sensorLocations[6] = {
    10, // verified
    43, // verified
    3,  // verified
    30, // verified
    17, // verified
    21  // verified
};

// LED strip pin and data format configuration
#define OUTER_RING_LED_PIN      10
#define OUTER_RING_COLOR_ORDER  BRG
#define OUTER_RING_CHIPSET      WS2811
#define OUTER_RING_NUM_LEDS     46
#define INNER_RING_LED_PIN      9
#define INNER_RING_COLOR_ORDER  GRB
#define INNER_RING_CHIPSET      WS2812
#define INNER_RING_NUM_LEDS     12

// Color arrays for the outer ring of 12V and the inner Neopixel ring
CRGB leds[OUTER_RING_NUM_LEDS];
CRGB innerLeds[INNER_RING_NUM_LEDS];

// Cellular automata array for the outer ring, plus previous frame for averaging
static int cells[OUTER_RING_NUM_LEDS];
static int lastFrameCells[OUTER_RING_NUM_LEDS];

// Frame counter for plasma animation
long frameCount = 0;

// Reduce noise from the IR sensors
static float integrateSensor(int index)
{
    static float lastValue[6];              
    static uint32_t lastTime[6];
    uint32_t now = micros();

    float value = analogRead(index) * SENSOR_SCALING;
    float dt = (now - lastTime[index]) * 1e-6; 
    lastValue[index] += (value - lastValue[index]) * dt;

    lastTime[index] = now;
    return lastValue[index];
}

void setup() {
  delay(500); // sanity delay
  FastLED.addLeds<OUTER_RING_CHIPSET, OUTER_RING_LED_PIN, OUTER_RING_COLOR_ORDER>(leds, OUTER_RING_NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.addLeds<INNER_RING_CHIPSET, INNER_RING_LED_PIN, INNER_RING_COLOR_ORDER>(innerLeds, INNER_RING_NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  gPalette = CRGBPalette16(PALETTE_COLOR1, PALETTE_COLOR2, PALETTE_COLOR3, PALETTE_COLOR4);
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
    for(int c = 0; c < OUTER_RING_NUM_LEDS; c++) {
        
        int neighbors = 0;
        
        if (c > 0 && cells[c-1] > threshhold) {
            neighbors++;
        }
        if (c < OUTER_RING_NUM_LEDS - 1 && cells[c+1] > threshhold) {
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

    // Use the sensor readings to increase neighboring cell scores
    for(int i = 0; i < 6; i++) {
        float sensor = integrateSensor(i);
        int origin = sensorLocations[i];
      
        if (sensor > SENSOR_THRESHOLD) {
          cells[origin] += (sensor - SENSOR_THRESHOLD) * SENSOR_GROWTH_INTENSITY;
        }
    }

// address debugging
#if 0
    for (int i = 0; i < OUTER_RING_NUM_LEDS; i++)
    {
        leds[i] = CRGB(i % 10 == 0 ? 255 : 0, 0, i % 2 ? 255 : 0);
    }
#endif

    // Convert cell scores to color
    for(int i = 0; i < OUTER_RING_NUM_LEDS; i++) {

        // Smooth over neighboring pixels
        int val = (cells[i] + cells[(i + 1) % OUTER_RING_NUM_LEDS] + cells[(i - 1) % OUTER_RING_NUM_LEDS]) / 3;

        // Clamp to 255
        if (val > 255) { val = 255; }

        // Average with last frame
        val = (lastFrameCells[i] + val) / 2;
        lastFrameCells[i] = val;

        // Convert value to RGB color from the palette
        CRGB color = ColorFromPalette(gPalette, val);

        // Dim pixel slightly with plasma animation to keep it interesting 
        byte p = plasma(frameCount, i, 1);
        color.r = scale8_video(color.r, dim8_video(p));
        color.g = scale8_video(color.g, dim8_video(p));
        color.b = scale8_video(color.b, dim8_video(p));

        // Stay out of the dimmest, noisy range on cheap-PWM LEDs
        if (color.r < 8)
            color.r = 8;
        if (color.g < 8)
            color.g = 8;

        // uncomment to display cell value directly for debugging
        //color = CRGB(val, 0, 0);

        leds[i] = color;
    }

    // Downsample the outer ring into the inner ring
    for (int i=0; i < INNER_RING_NUM_LEDS; i++) {
        innerLeds[i] = leds[int(i * OUTER_RING_NUM_LEDS / (INNER_RING_NUM_LEDS * 1.0))];
    }

    FastLED.show(); 
    FastLED.delay(1000 / FRAMES_PER_SECOND);
    frameCount++;
}
