/********************************************************************************************************************************
*
*  Project: 7 Band Spectrum Analyzer
*  Target Platform: Arduino NANO
*  
*  Version: 7.0
*  Spectrum analyses done with ONE analog chips MSGEQ7
*  
*  Dan Micu   
*  On SKYPE: dan.micu@live.com
*  On GoogleChat: samm928@gmail.com
*  Senior PCB Designer
*  youtube:   https://www.youtube.com/watch?v=TCKeMlC6nYg
*  github:    https://github.com/samm928
*  
/*******************************************************************************************************************************
*  Libaries and external files
********************************************************************************************************************************/
#include <FastLED_NeoMatrix.h>                     // Fastled Neomatrix driver.
#include <Wire.h>
#include <EasyButton.h>                            // Easybutton Library
#include "debug.h"                                 // External file with debug subroutine
#include "Settings.h"                              // External file with all changeable Settings

// Peak related stuff we need
int Peakdelay;                                     // Delay before peak falls down to stack. Overruled by PEAKDELAY Potmeter
int PeakTimer[COLUMNS];                            // counter how many loops to stay floating before falling to stack
char PeakFlag[COLUMNS];                            // the top peak delay needs to have a flag because it has different timing while floating compared to falling to the stack

// Led matrix Arrays do not change unless you have more then 16 bands
byte peak[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};          // The length of these arrays must be >= COLUMNS
int oldBarHeights[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // So if you have more then 16 bands, you must add zero's to these arrays
int bandValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Misc. Stuff
int colorIndex = 0;                                // Yep, we need this to keep track of colors
uint8_t colorTimer = 0;                            // Needed to change color periodically
long LastDoNothingTime = 0;                        // only needed for screensaver
int DemoModeMem = 0;                               // to remember what mode we are in when going to demo, in order to restore it after wake up
bool AutoModeMem = false;                          // same story
bool DemoFlag = false;                             // we need to know if demo mode was manually selected or auto engadged. 

// Button stuff
int buttonPushCounter = DefaultMode;               // Timer for Psuh Button
bool autoChangePatterns = false;                   // press the mode button 3 times within 2 seconds to auto change paterns.

// Defining some critical components
EasyButton modeBtn(Switch1);                       // The mode Button on A4

/*******************************************************************************************************************************
*  FastLED_NeoMaxtrix - see https://github.com/marcmerlin/FastLED_NeoMatrix for Tiled Matrixes, Zig-Zag and so forth
*******************************************************************************************************************************/
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, kMatrixWidth, kMatrixHeight,
  NEO_MATRIX_TOP           + NEO_MATRIX_LEFT +     // Use this to configure the array settings.
  NEO_MATRIX_COLUMNS       + NEO_MATRIX_ZIGZAG +   // Use Progressive if end-to end wiring was done to the array panel.
  NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS);   // I prefer lower frequencies to the left hand side.

/********************************************************************************************************************************
*  Setup routine
********************************************************************************************************************************/
void setup() {
  Serial.begin(57600);
//  FrequencyBoard.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);  CAN BE USED TO EXPAND TO 14-Channels or 21-Channels
//  FrequencyBoard.set_freq(10400000ULL, SI5351_CLK0);
//  FrequencyBoard.set_freq(14200000ULL, SI5351_CLK1);
//  FrequencyBoard.set_freq(18400000ULL, SI5351_CLK2);
//  dbgprint("Init of Freq board done.");
  
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);
  FastLED.setBrightness(BRIGHTNESSMARK);
  FastLED.clear();

  modeBtn.begin();
  modeBtn.onPressed(changeMode);                          // When mode key is pressed, call changeMode sub routine
  modeBtn.onSequence(3, 2000, startAutoMode);             // enable automode if pressed 3 times, within 2 seconds

//Now let's configure the datalines for the MSGEQ7's and prepare them for running mode
  pinMode      (STROBE_PIN,    OUTPUT);       // MSGEQ7 strobe pin configure as output pin
  pinMode      (RESET_PIN,     OUTPUT);       // MSGEQ7 reset pin configure as output pin
  pinMode      (LED_PIN,       OUTPUT);       // Connection to LEDSTRIP configure as output pin

//Initialize the Analyzer Ic's
  digitalWrite (RESET_PIN,  LOW);
  digitalWrite (STROBE_PIN, LOW);
  delay        (1);
  digitalWrite (RESET_PIN,  HIGH);
  delay        (1);
  digitalWrite (RESET_PIN,  LOW);
  digitalWrite (STROBE_PIN, HIGH);
  delay        (1);
}
/********************************************************************************************************************************
*  END OF setup routine
********************************************************************************************************************************/
void changeMode() {
  dbgprint("Button pressed");
  if (FastLED.getBrightness() == 0) FastLED.setBrightness(BRIGHTNESSMARK);  //Re-enable if lights are "off"
  autoChangePatterns = false;
  buttonPushCounter = (buttonPushCounter + 1) % NumberOfModes; //%6
  dbgprint("mode: %d\n", buttonPushCounter);
   if(DemoFlag==true) {                // in case we are in demo mode ... and manual go out of it.
    dbgprint("demo is true");
    buttonPushCounter=DemoModeMem;     // go back to mode we had before demo mode
    LastDoNothingTime = millis();      // reset that timer to prevent going back to demo mode the same instance
    DemoFlag=false;
 }
}

void startAutoMode() {
  autoChangePatterns = true;
  delay(2000);
  dbgprint(" Patterns will change after few seconds ");
  dbgprint(" You can reset by pressing the mode button again");
}

void brightnessOff() {
  FastLED.setBrightness(0);            // Lights out
}

/********************************************************************************************************************************
*  Main Loop
********************************************************************************************************************************/
void loop(){
   if (buttonPushCounter!=12)FastLED.clear();   // Not for demo mode

// GET OUR USER INPUTS   *****************************************************************************************************
   AMPLITUDE = map(analogRead(SENSITIVITYPOT), 0, 1023, 50, 1023);               // read sensitivity potmeter and update amplitude setting
   BRIGHTNESSMARK = map(analogRead(BRIGHTNESSPOT), 0, 1023, BRIGHTNESSMAX, 10);  // read brightness potmeter
   Peakdelay = map(analogRead(PEAKDELAYPOT), 0, 1023, 150, 1);                   // update the Peakdelay time with value from potmeter
   FastLED.setBrightness(BRIGHTNESSMARK);                                        // update the brightness
   modeBtn.read();                                                               // what the latest on our mode switch?
   
   for (int i = 0; i < COLUMNS; i++) bandValues[i] = 0;                          // Reset bandValues[ ]

// Now RESET the MSGEQ7's and use strobe to read out all current band values and store them in bandValues array **************
   digitalWrite(RESET_PIN, HIGH);
   delayMicroseconds(3000);
   digitalWrite(RESET_PIN, LOW);

// READ IN MGSEQ7 VALUES  ***********************************************************************************************
for (int i = 0; i < COLUMNS; i++) {
   digitalWrite(STROBE_PIN, LOW);
   delayMicroseconds(1000);
   bandValues[i] = analogRead(0) - NOISE;
     if (bandValues[i]<120)  bandValues[i]=0;
     bandValues[i] = constrain(bandValues[i], 0, AMPLITUDE);
     bandValues[i] = map(bandValues[i], 0, AMPLITUDE, 0, kMatrixHeight);        // add i++; if expanding to 14 or 21 channels and uncomment below
//   bandValues[i] = analogRead(1) - NOISE;
//     if (bandValues[i]<120) bandValues[i]=0;
//     bandValues[i] = constrain(bandValues[i], 0, AMPLITUDE);
//     bandValues[i] = map(bandValues[i], 0, AMPLITUDE, 0, kMatrixHeight); i++;
//   bandValues[i] = analogRead(2) - NOISE;
//     if (bandValues[i]<120) bandValues[i]=0;
//     bandValues[i] = constrain(bandValues[i], 0, AMPLITUDE);
//   bandValues[i] = map(bandValues[i], 0, AMPLITUDE, 0, kMatrixHeight);
//   if (bandValues[i] > DemoTreshold && i > 1) LastDoNothingTime = millis();   // if there is signal in any off the bands[>2] then no demo mode 
   digitalWrite(STROBE_PIN, HIGH);
}
   
// Collect and process data from BandValues and transform them into BAR HEIGHTS ***************************************
    for (byte band = 0; band < COLUMNS; band++) {                          // Scale the bars for the display
    int barHeight = bandValues[band];
    if (barHeight>TOP) barHeight = TOP;
    
    // Small amount of averaging between frames
       barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;            // Fast Filter, more rapid movement
    // barHeight = ((oldBarHeights[band] * 2) + barHeight) / 3;            // minimum filter makes changes more smooth

    // Move peak up
    if (barHeight > peak[band]) {
      peak[band] = min(TOP, barHeight);
      PeakFlag[band] = 1;
    }
    /*
       Mode 1: TriBar each Column is devided into 3 sections, Bottom,Middle and Top, Each section has different color
       Mode 2: Each Column different color, purple peaks
       Mode 3: Each Colomn has the same gradient from a color pattern, white peaks
       Mode 4: All is red color, blue peaks
       Mode 5: All is blue color, red peaks
       Mode 6: Center Bars following defined color pattern, Red White Yellow
       Mode 7: Center Bars following defined color pattern ---> White Red
       Mode 8: Center Bars following defined color pattern ---> Pink White Yellow
       Mode 9: Peaks only, color depends on level (height)
       Mode 10: Peaks only, blue color
       Mode 11: Peaks only, color depends on level(height), following the tribar pattern
       Mode 12: Fire, doesn't response to music
       Mode 0: Gradient mode, colomns all have the same gradient but gradient changes following a rainbow pattern
    */

// Now visualize those bar heights and add some MODES and PATERNS *******************************************************
    switch (buttonPushCounter) {
    case 0:
      ChangingBars(band, barHeight);    // Rainbow Dynamic mode from Bottom Up
      break;
    case 1:
      TriBar(band, barHeight);          // Default Mode Green Yellow-Orange
      TriPeak(band);                    // Green, Orange and Red Peaks
      break;
    case 2:
      RainbowBars(band, barHeight);     // Rainbow Static mode Horizontal
      NormalPeak(band, PeakColor1);     // Purple Peaks
      break;
    case 3:
      HolloweenBars(band, barHeight);   // Holloween Gradient mode
      NormalPeak(band, PeakColor2);     // Pink Peaks
      break;
    case 4:
      SameBar1(band, barHeight);        // Lava Gradient Gradient Heat Pallete
      NormalPeak(band, PeakColor3);     // Hot Pink peaks
      break;
    case 5:
      SameBar2(band, barHeight);        // Sunset Gradient
      NormalPeak(band, PeakColor4);     // White peaks
      break;
    case 6:
      SameBar3(band, barHeight);        // Fire
      NormalPeak(band, PeakColor1);     // Blue peak colors only   
      break;
    case 7:
      CenterBars1(band, barHeight);     // Center hot-up / Cold-down - Gradient 'Sunset_gp' 
      break;
    case 8:
      CenterBars2(band, barHeight);     // Center hot-up / Cold-down 
      break;
    case 9:
      CenterBars3(band, barHeight);     // Center hot-up / Cold-down
      break;
    case 10:
      NormalPeak(band, PeakColor5);     // Blue peak colors only
      // no bars
      break;
    case 11:
      TriBar2(band, barHeight);          // TriBar color Mode: Blue-Yellow-Orange
      TriPeak2(band);
      break;
    case 12:
      SameBar3(band, barHeight);        // Fire REMOVED to save memory
      break;
    } 
    oldBarHeights[band] = barHeight;    // Save oldBarHeights for averaging later
 }

  // Decay peak
  EVERY_N_MILLISECONDS(Fallingspeed) {
    for (byte band = 0; band < COLUMNS; band++) {
      if (PeakFlag[band] == 1) {
        PeakTimer[band]++;
        if (PeakTimer[band] > Peakdelay) {
          PeakTimer[band] = 0;
          PeakFlag[band] = 0;
        }
      } else if (peak[band] > 0) {
        peak[band] -= 1;
      }
    }
    colorTimer++;
  }
  EVERY_N_MILLISECONDS(10) colorTimer++;   // Used in some of the patterns
  EVERY_N_SECONDS(AutoChangetime) {
    if (autoChangePatterns) buttonPushCounter = (buttonPushCounter + 1) % (NumberOfModes - 1); //timer to autochange patterns when enabled but exclude demo mode
  }
  FastLED.show();
}
/********************************************************************************************************************************
*  END SUB  Main Loop
********************************************************************************************************************************/

/********************************************************************************************************************************
*  SUB-Rountines related to Paterns and Peaks
********************************************************************************************************************************/
void ChangingBars(int band, int barHeight) {                                 // Rainbow dynamic color Mode 0
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    matrix -> drawPixel(x, y, CHSV(ChangingBar_Color));
   }
  }
 }

void RainbowBars(int band, int barHeight) {                                   // Rainbow Static with Peaks color Mode 2
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    matrix -> drawPixel(x, y, CHSV(RainbowBar_Color));
   }
  }
 }

void SameBar1(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    matrix -> drawPixel(x, y, ColorFromPalette(heatPal, y * (255 / (barHeight + 1))));
   }
  }
 }
 
void SameBar2(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    matrix -> drawPixel(x, y, ColorFromPalette(sunset, y * (255 / (barHeight + 1))));
   }
  }
 }

void SameBar3(int band, int barHeight) {                                     // Rainbow Static with Peaks color Mode 2
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    matrix -> drawPixel(x, y, ColorFromPalette(lava, y * (255 / (barHeight + 1))));
   }
  }
 }  

void HolloweenBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    matrix -> drawPixel(x, y, ColorFromPalette(holloween, y * (255 / (barHeight + 1))));
   }
  }
 }

void CenterBars1(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    if (barHeight < 0) barHeight = 1;               // at least a line in the middle is what we want
    int yStart = ((kMatrixHeight - barHeight) / 2);
    for (int y = yStart; y <= (yStart + barHeight); y++) {
    int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
    matrix -> drawPixel(x, y, ColorFromPalette(heatPal, colorIndex));
   }
  }
 }

void CenterBars2(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    
    if (barHeight % 2 == 0) barHeight--;
    if (barHeight < 0) barHeight = 1;               // at least a line in the middle is what we want
    int yStart = ((kMatrixHeight - barHeight) / 2);
    for (int y = yStart; y <= (yStart + barHeight); y++) {
    int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
    matrix -> drawPixel(x, y, ColorFromPalette(sunset, colorIndex));
   }
  }
 }

void CenterBars3(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    if (barHeight < 0) barHeight = 1;               // at least a line in the middle is what we want
    int yStart = ((kMatrixHeight - barHeight) / 2);
    for (int y = yStart; y <= (yStart + barHeight); y++) {
    int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
    matrix -> drawPixel(x, y, ColorFromPalette(outrunPal, colorIndex));
   }
  }
 }

void NormalPeak(int band, int H, int S, int V) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
  matrix -> drawPixel(x, peakHeight, CHSV(H, S, V));
  }
 }

void TriBar(int band, int barHeight) {                                       // TRI-Bar color Green Yellow Red Mode 1
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    if (y < 5) matrix -> drawPixel(x, y, CHSV(TriBar_Color_Top));
    else if (y > 9) matrix -> drawPixel(x, y, CHSV(TriBar_Color_Bottom));
    else matrix -> drawPixel(x, y, CHSV(TriBar_Color_Middle));
   }
  }
 }

void TriBar2(int band, int barHeight) {                                       // TRI-Bar color Green Yellow Red Mode 1
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
   for (int y = TOP; y >= TOP - barHeight; y--) {
    if (y < 5) matrix -> drawPixel(x, y, CHSV(TriBar_Color_Top2));
    else if (y > 9) matrix -> drawPixel(x, y, CHSV(TriBar_Color_Bottom2));
    else matrix -> drawPixel(x, y, CHSV(TriBar_Color_Middle2));
   }
  }
 }

void TriPeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (peakHeight < 4) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Top_Peak));         
    else if (peakHeight > 8) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Bottom_Peak));
    else matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Middle_Peak));
  }
 }

void TriPeak2(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (peakHeight < 4) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Top_Peak2));
    else if (peakHeight > 8) matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Bottom_Peak2));
    else matrix -> drawPixel(x, peakHeight, CHSV(TriBar_Color_Middle_Peak2));
  }
 }
/********************************************************************************************************************************
*  END SUB Rountines related to Paterns and Peaks                                                                               *
********************************************************************************************************************************/
