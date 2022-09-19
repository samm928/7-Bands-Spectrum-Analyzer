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
********************************************************************************************************************************/

#pragma once
char version[]="4.0";                               // Define version number for reference only

// Debugging
#define DEBUG_BUFFER_SIZE 100                       // Debug buffer size
int  DEBUG = 1;                                     // When debug=1, extra information is printed to serial port. Turn of if not needed--> DEBUG=0

// Ledstrips/ matrix main display  **************************************************************************************
#define LED_PIN           9                         // This is the data pin of your led matrix, or ledstrips.
#define COLUMNS           7                         // Number of bands on display, this is not the same as display width...because display can be 28 ( double pixels per bar)

//const uint8_t                                     // if you have more then 16 bands, you will need to change the Led Matrix Arrays in the main file.
#define kMatrixWidth      7                         // Matrix width --> number of columns in your led matrix
#define kMatrixHeight    16                         // Matrix height --> number of leds per column   

// Some definitions for setting up the matrix  **************************************************************************
#define BAR_WIDTH         1
//#define BAR_WIDTH  (kMatrixWidth / (COLUMNS -1))    // If width >= 8 light 1 LED width per bar, >= 16 light 2 LEDs width bar etc
#define TOP        (kMatrixHeight - 0)              // Don't allow the bars to go offscreen
#define NUM_LEDS   (kMatrixWidth * kMatrixHeight)   // Total number of LEDs in the display array

// Ledstrips or pixelmatrix  +*******************************************************************************************
#define CHIPSET      WS2812B                        // LED strip type -> Same for both ledstrip outputs( Matrix and logo)
#define BRIGHTNESSMAX    100                        // Max brightness of the leds...carefull...to bright might draw to much amps!
#define COLOR_ORDER      GRB                        // If colours look wrong, play with this
#define LED_VOLTS          5                        // Usually 5 or 12
#define MAX_MILLIAMPS   2000                        // Careful with the amount of power here if running off USB port, This will effect your brightnessmax. Currentlimit overrules it.
                                                    // If your power supply or usb can not handle the set current, arduino will freeze due to power drops.
// ADC Filter  **********************************************************************************************************
#define NOISE             20                        // Used as a crude noise filter on the adc input, values below this are ignored

//Controls  *************************************************************************************************************
#define PEAKDELAYPOT      A1                        // Potmeter for sensitivity input 0...5V (0-3.3V on ESP32)
#define SENSITIVITYPOT    A2                        // Potmeter for Brightness input 0...5V (0-3.3V on ESP32)  
#define BRIGHTNESSPOT     A3                        // Potmeter for Peak Delay Time input 0...5V (0-3.3V on ESP32)
#define Switch1           A4                        // Connect a push button to this pin to change patterns PIN64
#define LONG_PRESS_MS   3000                        // Number of ms to count as a long press on the switch

// MSGEQ7 Pinout Connections  *******************************************************************************************
#define STROBE_PIN        8                         //MSGEQ7 strobe pin
#define RESET_PIN         7                         //MSGEQ7 reset pin

int BRIGHTNESSMARK =    100;                        // Default brightnetss, however, overruled by the Brightness potmeter
int AMPLITUDE      =   2000;                        // Depending on your audio source level, you may need to alter this value. it's controlled by the Sensitivity Potmeter

// Peak related stuff  **************************************************************************************************
#define Fallingspeed     30                         // This is the time it takes for peak tiels to fall to stack, this is not the extra time that you can add by using the potmeter
                                                    // for peakdelay. Because that is the extra time it levitates before falling to the stack    
#define AutoChangetime   10                         // If the time  in seconds between modes, when the patterns change automatically, if to fast, you can increase this number            
#define NumberOfModes    13                         // The number of modes, remember it starts counting at 0,so if your last mode is 11 then the total number of modes is 12
#define DefaultMode       1                         // This is the mode it will start with after a reset or boot

CRGB leds[NUM_LEDS];                                // Leds on the Ledstrips/Matrix of the actual Spectrum analyzer lights.

/****************************************************************************
 * Colors of bars and peaks in different modes, changeable to your likings  *
 ****************************************************************************/

// Colors mode 0 *************************************************************
#define ChangingBar_Color   y * (255 / kMatrixHeight) + colorTimer, 255, 255

// Colors MODE 1 These are the colors from the TriBar **************************
#define TriBar_Color_Top              3, 255, 255            // Orange-Red
#define TriBar_Color_Middle          25, 255, 255            // Yellow-Orange
#define TriBar_Color_Bottom          75, 255, 255            // Green- Yellow

// Colors MODE 11 These are the colors from the TriBar2 ************************
#define TriBar_Color_Top2             6, 255, 225            // Red-Orange
#define TriBar_Color_Middle2         30, 255, 225            // Orange-Yellow
#define TriBar_Color_Bottom2        180, 255, 255            // Purple-Blue

// Original Version of Tri-Bar Color Peaks                   // TRI-BAR Color Peaks: Green - Orange - Red
#define TriBar_Color_Top_Peak         0, 255, 255            // Red
#define TriBar_Color_Middle_Peak     10, 255, 255            // Orange
#define TriBar_Color_Bottom_Peak     95, 255, 255            // Green

// Second Version of Tri-Bar Color Peaks                     // TRI-BAR Color Peaks: Blue - Purple - Pink
#define TriBar_Color_Top_Peak2        0, 255, 255            // Red
#define TriBar_Color_Middle_Peak2    45, 255, 255            // Orange
#define TriBar_Color_Bottom_Peak2   160, 255, 255            // Blue

// Colors MODE 2 Static horizontal Rainbow ***********************************
#define RainbowBar_Color  (x / BAR_WIDTH) * (255 / COLUMNS), 255, 255

// Colors MODE 3 - Holloween *************************************************
#define PeakColor2  130,  30, 255              // White peaks
DEFINE_GRADIENT_PALETTE( purple_gp ) {
    0,   2,  1,  1,
   53,  18,  1,  0,
  104,  69, 29,  1,
  153, 167,135, 10,
  255,  46, 56,  4};
CRGBPalette16 holloween = purple_gp;

// Colors mode Gradient Lava **************************************************
#define SameBar_Color1    185, 235, 235

// Colors mode Gradient Outrun **************************************************
DEFINE_GRADIENT_PALETTE( Outrun_gp ) {
    0,  47, 30,  2,
   42, 213,147, 24,
   84, 103,219, 52,
  127,   3,219,207,
  170,   1, 48,214,
  212,   1,  1,111,
  255,   1,  7, 33};
CRGBPalette16 outrunPal = Outrun_gp;

// Colors mode Gradient Heat Pallete *******************************************
DEFINE_GRADIENT_PALETTE( redyellow_gp ) {  
  0,   0,   0, 255,
 64, 255, 218,   0,
128, 231,   0,   0,
192, 255, 218,   0,
255,   0,   0,   0 };
CRGBPalette16 heatPal = redyellow_gp;

// Colors mode Gradient ********************************************************
DEFINE_GRADIENT_PALETTE( Sunset_Real_gp ) {
    0, 120,  0,   0,
   22, 179, 22,   0,
   51, 255,104,   0,
   85, 167, 22,  18,
  135, 100,  0, 103,
  198,  16,  0, 130,
  255,   0,  0, 160};
CRGBPalette16 sunset = Sunset_Real_gp;

// Colors mode Gradient ********************************************************
DEFINE_GRADIENT_PALETTE( fire_gp ) {
    0,   1,  1,  0,
   76,  32,  5,  0,
  146, 192, 24,  0,
  197, 220,105,  5,
  240, 252,255, 31,
  250, 252,255,111,
  255, 255,255,255};
CRGBPalette16 fire = fire_gp;

// Colors mode Gradient ********************************************************
DEFINE_GRADIENT_PALETTE( lava_gp ) {
    0,   0,  0,  0,
   46,  18,  0,  0,
   96, 113,  0,  0,
  108, 142,  3,  1,
  119, 175, 17,  1,
  146, 213, 44,  2,
  174, 255, 82,  4,
  188, 255,115,  4,
  202, 255,156,  4,
  218, 255,203,  4,
  234, 255,255,  4,
  244, 255,255, 71,
  255, 255,255,255};
CRGBPalette16 lava = lava_gp;

// ADDITIONAL PEAK COLOR MODES  ********************************************************
#define PeakColor1    235, 245, 255                         // Purple peaks
#define PeakColor2    225, 235, 255                         // Orange peaks
#define PeakColor3    225, 220, 255                         // Hot Pink peaks
#define PeakColor4    250, 245, 255                         // Red peaks
#define PeakColor5    160, 255, 255                         // Blue Peaks
