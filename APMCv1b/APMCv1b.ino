/*
The Arduino Pulsed Motor Controller v1.0b by Jean-Louis Naudin
-------      April 2013 - http://www.jlnlab.com    ----------

This firmware is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either version 2.1 of the License, or (at your option) any later version.
     
Hall effect Probe : Siemens TLE 4935L
http://www.produktinfo.conrad.com/datenblaetter/150000-174999/153775-da-01-en-HALL_SENSOR_TLE_4935L.pdf
    * 1   GND
    * 2   HALL probe (TLE4935L) output signal connected to PIN 21 (use the interrupt 2 for the Arduino Mega 2560)
    * Vdd +5V
Motor Coil :
    * 52 : COIL connected to the Arduino Mega 2560 output DIGITAL PIN 52
    * GND

Arduino MEGA 2560 :
http://arduino.cc/en/Main/arduinoBoardMega2560
    
Arduino Shield LCD Keypad used :
http://www.hobbyking.com/hobbyking/store/__25087__Arduino_LCD_Keypad_Shield.html

*/
// Author : Jean-Louis Naudin (JLN) 
// updates: last update 2013-04-28
//
// 2013-04-08: Start of the project
// 2013-04-09: Hardware build
// 2013-04-10: RPM measurement and autostart feature at low RPM
// 2013-04-11: First release
// 2013-04-12: RPM smoothing improvement (avg on 10 samples)
// 2013-04-28: Update for the Bedini's type unipolar motor
//   

/*--------------------------------------------------------------------------------------
  Includes
--------------------------------------------------------------------------------------*/
#include <avr/io.h>
#include <avr/eeprom.h>
#include <math.h>
#include <LiquidCrystal.h>   // include LCD library
#include "LCDtools.h"

/*--------------------------------------------------------------------------------------
  Init the LCD library with the LCD pins to be used
--------------------------------------------------------------------------------------*/
LiquidCrystal lcd( 8, 9, 4, 5, 6, 7 );   //Pins for the freetronics 16x2 LCD shield. LCD: ( RS, E, LCD-D4, LCD-D5, LCD-D6, LCD-D7 )

/*--------------------------------------------------------------------------------------
  Variables
--------------------------------------------------------------------------------------*/

byte buttonJustPressed  = false;         //this will be true after a ReadButtons() call if triggered
byte buttonJustReleased = false;         //this will be true after a ReadButtons() call if triggered
byte buttonWas          = BUTTON_NONE;   //used by ReadButtons() for detection of button events
byte button;

/* -----------------DLE gen ------------------*/

static int hall              = false;
static boolean data_display  = true;
static boolean running       = false;

static int rpm     = 0;
volatile float time = 0;
volatile float time_last = 0;
volatile int rpm_array[10] = {0,0,0,0,0,0,0,0,0,0};

static unsigned long turn    = 0;
static int ptime             = 0;
static int count             = 0;

static int pulse_on          = 250;  // Pulse lagging after the TDC in µS
static int pulse_dur         = 2500; // Pulse duration in µS
static int pulse_long_step   = 250;  // increment step in µS for the tuning
static int lograte           = 300;

#define COIL_MAIN            52  // MAIN COIL  : Digital OUTPUT 52
#define LED_PULSE            53  // LED_FIRING : Digital OUTPUT 53
#define ENABLED              1
#define DISABLED             0

#define TRACE               ENABLED  // Output some datas on serial port

#define AUTODURATION        ENABLED  // set for auto timing adjustement Vs the RPM
#define AUTOSTART           DISABLED  // Autostart mode to help the starting

#define NUMB_POLES           4  // number of magnetic poles

/*--------------------------------------------------------------------------------------
Initialisation of parameters
--------------------------------------------------------------------------------------*/
void setup()
{ 
#if TRACE == ENABLED    
   Serial.begin (115200);
   delay(2000);
   Serial.println("\n\nThe Arduino Pulsed Motor Controller APMC v1.0b by Jean-Louis Naudin\n\n");
#endif

/* -----------------APMC Init OUTPUTS -----------------*/

  pinMode(COIL_MAIN, OUTPUT);
  pinMode(LED_PULSE, OUTPUT);
  
  //Digital Pin 21 of ArduMega 2560 is set as the Interrupt 2
  attachInterrupt(2, hall_interrupt, FALLING);
    
/* -----------------------------------*/
   //button adc input
   pinMode( BUTTON_ADC_PIN, INPUT );         //ensure A0 is an input
   digitalWrite( BUTTON_ADC_PIN, LOW );      //ensure pullup is off on A0
   //lcd backlight control
   digitalWrite( LCD_BACKLIGHT_PIN, HIGH );  //backlight control pin D3 is high (on)
   pinMode( LCD_BACKLIGHT_PIN, OUTPUT );     //D3 is an output
   //set up the LCD number of columns and rows: 
   lcd.begin( 16, 2 );
   //Print some initial text to the LCD.
   lcd.setCursor( 0, 0 );   //top left
   lcd.print( "   APMC v1.0B   " ); 
   lcd.setCursor( 0, 1 );   //top left
   lcd.print( " www.jlnlab.com" ); 
   delay(3000);
   lcd.clear();
   lcd.setCursor( 0, 0 );   //top left
   lcd.print( "APMC1B" );
   hall = false;
}

/* --------------- APMC Tools --------------------*/

// Send the pulse to the coil
void SendPulse()
{
    if(hall) {  // TDC DETECTED (SOUTH POLE)    
      data_display = false;
      hall = false;
      if(rpm < 400 && AUTOSTART == ENABLED)      // At low RPM (<300) set to autostart mode
      { delayMicroseconds(7000);
      } else {
      delayMicroseconds(pulse_on); // Send the pulse after pulse_on µS from the TDC
      }
      if(running) 
      {   
          digitalWrite(COIL_MAIN, LOW);  // High Power OUTPUT ON
      }
      digitalWrite(LED_PULSE, HIGH);  // set the LED on
      lcd.setCursor( 15, 0 );   //bottom left
      lcd.print( "T" );
      
#if AUTODURATION == ENABLED                  
      if(rpm < 400 && AUTOSTART == ENABLED)     // At low RPM (<300) set to autostart mode
      { ptime = 8000;
      } else {
      ptime = 1000*(float)pulse_dur/rpm;  // auto pulse duration Vs RPM
      if(ptime >10000) ptime=10000;
      else if(ptime<250) ptime=250;     
      }
#else
      ptime = pulse_dur;
#endif

      delayMicroseconds(ptime);   // after ptime ON, the pulse is set to OFF
      
      digitalWrite(COIL_MAIN, HIGH);  // High Power OUTPUT OFF
      digitalWrite(LED_PULSE, LOW);   // set the LED oFF
    } 
    else { // OFF TDC
      lcd.setCursor( 15, 0 );   //bottom left
      lcd.print( " " );
      data_display = true;
    }     
}

// Display the pulse setting and the RPM on the LCD display
void display_pulse()
{
   count++; 
      if (data_display && count > lograte)
    {    count = 0;
         lcd.setCursor( 0, 1 );
         lcd.print( pulse_on ); lcd.print( "  " );
         lcd.setCursor( 5, 1 );
         lcd.print( pulse_dur ); lcd.print( " " );
         lcd.setCursor( 15, 1 ); 
         if(running)
         { lcd.print( "O" );
           lcd.setCursor( 10, 1 );   
           lcd.print( ptime );  lcd.print( " " );
         } else {
           lcd.print( "F" );
           lcd.setCursor( 10, 1 );   
           lcd.print( pulse_dur );  lcd.print( " " );         
         } 
         lcd.setCursor( 7, 0 );   //bottom left
         lcd.print( rpm );      
         lcd.print( "RPM " );   
#if TRACE == ENABLED      
         Serial.print(micros());  Serial.print(","); Serial.print(rpm);  
         Serial.print(","); Serial.print(time); 
         Serial.print(","); Serial.print(pulse_on); 
         Serial.print(","); Serial.print(pulse_dur); 
         Serial.print(","); Serial.println(ptime);
#endif   
    }  
}

// RPM computations
void calc_rpm()
{  
  if(time > 0)
  {  // 10 Samples for smoothing the datas
      rpm_array[0] = rpm_array[1];
      rpm_array[1] = rpm_array[2];
      rpm_array[2] = rpm_array[3];
      rpm_array[3] = rpm_array[4];
      rpm_array[4] = rpm_array[5];
      rpm_array[5] = rpm_array[6];
      rpm_array[6] = rpm_array[7];
      rpm_array[7] = rpm_array[8];
      rpm_array[8] = rpm_array[9];     
      rpm_array[9] = 60*(1000000/(time*NUMB_POLES));    
    // compute the avg rpm
      rpm = (rpm_array[0] + rpm_array[1] + rpm_array[2] + rpm_array[3] + rpm_array[4] 
      + rpm_array[5] + rpm_array[6] + rpm_array[7] + rpm_array[8] + rpm_array[9]) / 10;
  }
}

// Keyboard control for the pulse setting and control
void check_buttons()
{
     //show text label for the button pressed
   switch( button )
   {
      case BUTTON_NONE:
      {
         lcd.print( "      " );
         break;
      }
      case BUTTON_RIGHT:    // Increase the pulse lag
      {
         pulse_on = pulse_on + pulse_long_step;
         break;
      }
      case BUTTON_UP:       // Increase the pulse duration
      {
         pulse_dur = pulse_dur + pulse_long_step;
         break;
      }
      case BUTTON_DOWN:     // Increase the pulse duration
      {
         pulse_dur = pulse_dur - pulse_long_step;
         break;
      }
      case BUTTON_LEFT:    // Decrease the pulse lagging
      {
         pulse_on = pulse_on - pulse_long_step;
        break;
     }
     case BUTTON_SELECT:   // ON/OFF the Pulse Motor
     {        
        running = !running;       
        break;
      }
      default:
     {
        break;
     }
   }
}

// handle buttons
void control_buttons()
{
   //get the latest button pressed, also the buttonJustPressed, buttonJustReleased flags
   button = ReadButtons();
   if( buttonJustPressed || buttonJustReleased )
   { check_buttons();
   }

   //clear the buttonJustPressed or buttonJustReleased flags, they've already done their job now.
   if( buttonJustPressed )
      buttonJustPressed = false;
   if( buttonJustReleased )
      buttonJustReleased = false; 
}

// Hall sensor function set on interrupt IRQ 2 on PIN 21 for the ArduMega 2560
void hall_interrupt()
{
   time = (micros() - time_last); 
   time_last = micros();
   hall = true;
}

/*--------------------------------------------------------------------------------------
  Arduino main loop
--------------------------------------------------------------------------------------*/
void loop()
{  
   calc_rpm();         // compute the true RPM
   
   SendPulse();        // energize the coil with the computed pulse duration
     
   display_pulse();    // display data on the LCD and send the data to the serial port
   
   control_buttons();  // check the buttons states
}



    
