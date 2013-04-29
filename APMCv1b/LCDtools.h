/* LCD Tools
  Pins used by LCD & Keypad Shield:
  
    A0: Buttons, analog input from voltage ladder
    D4: LCD bit 4
    D5: LCD bit 5
    D6: LCD bit 6
    D7: LCD bit 7
    D8: LCD RS
    D9: LCD E
    D3: LCD Backlight (high = on, also has pullup high so default is on)
  
  ADC voltages for the 5 buttons on analog input pin A0:
  
    RIGHT:  0.00V :   0 @ 8bit ;   0 @ 10 bit
    UP:     0.71V :  36 @ 8bit ; 145 @ 10 bit
    DOWN:   1.61V :  82 @ 8bit ; 329 @ 10 bit
    LEFT:   2.47V : 126 @ 8bit ; 505 @ 10 bit
    SELECT: 3.62V : 185 @ 8bit ; 741 @ 10 bit
    
*/   
/*--------------------------------------------------------------------------------------
  Defines
--------------------------------------------------------------------------------------*/
// Pins in use
#define BUTTON_ADC_PIN           A0   // A0 is the button ADC input
#define BATTERY_ADC_PIN          30  // A30 is the Battery measurement ADC input
#define INPUT_ADC_PIN            31  // A31 is the VOLTAGE INPUT measurement ADC input

// Original code removed as pin 10 controls this sheild
// #define LCD_BACKLIGHT_PIN         3  // D3 controls LCD backlight
#define LCD_BACKLIGHT_PIN         10  // D10 controls LCD backlight
// Original values for buttons
//#define RIGHT_10BIT_ADC           0  // right
//#define UP_10BIT_ADC            145  // up
//#define DOWN_10BIT_ADC          329  // down
//#define LEFT_10BIT_ADC          505  // left
//#define SELECT_10BIT_ADC        741  // right
//#define BUTTONHYSTERESIS         10  // hysteresis for valid button sensing window

// Added values by Maxx Eastick
// ADC readings expected for the 5 buttons on the ADC input
#define RIGHT_10BIT_ADC           0  // right
#define UP_10BIT_ADC            120  // up
#define DOWN_10BIT_ADC          280  // down
#define LEFT_10BIT_ADC          480  // left
#define SELECT_10BIT_ADC        720  // right
#define BUTTONHYSTERESIS         30  // hysteresis for valid button sensing window
//return values for ReadButtons()
#define BUTTON_NONE               0  // 
#define BUTTON_RIGHT              1  // 
#define BUTTON_UP                 2  // 
#define BUTTON_DOWN               3  // 
#define BUTTON_LEFT               4  // 
#define BUTTON_SELECT             5  // 
//some example macros with friendly labels for LCD backlight/pin control, tested and can be swapped into the example code as you like
#define LCD_BACKLIGHT_OFF()     digitalWrite( LCD_BACKLIGHT_PIN, LOW )
#define LCD_BACKLIGHT_ON()      digitalWrite( LCD_BACKLIGHT_PIN, HIGH )
#define LCD_BACKLIGHT(state)    { if( state ){digitalWrite( LCD_BACKLIGHT_PIN, HIGH );}else{digitalWrite( LCD_BACKLIGHT_PIN, LOW );} }



