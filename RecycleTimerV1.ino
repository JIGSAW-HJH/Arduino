#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

//NOKIA DISPLAY DEFINES:
#define XPOS 0
#define YPOS 1
#define EEPROM_ADDRESS 0x50 // 0x50 0x51 0x52 0x53 0x54 0x55 0x56 0x57
#define RTC_ADDRESS 0x68 


enum myStates{
  S_initsetup0,//setup settings -> check if 1st time boot
  S_initsetup1,//setup settings -> set the date and time for the RTC
  S_mainmenu,//show battery voltage level and current active state
  S_LCDbacklightSwitch,//switch the LCD backlight on/off
  S_CheckVoltage,//Read the voltage
  S_CheckTimers,//Check the delay timers
  S_switchContactor,//trigger the contactor ON/OFF
  S_DoFactoryReset,//Reset ALL settings on device -> After reset must do init setup again
  S_CheckKeypress,//Read any keypresses
  S_idlescreen,//switch off LCD screen and backlight
  S_qwerty
};

myStates STATES;
// Init the DS1307
RTC_DS1307 rtc;

//Init the NOKIA LCD SCREEN PINS:
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

unsigned long currentMillis = millis();

//FLAGS:
bool F_KeyPressed = false;// the key is not pressed
bool F_BackLight = false;// check if backlight is on or not
bool F_SwitchContactorOFF = false;//check when to switch contactor
//flag to chek when to start running the timer for the 
//backlight of the lcd
bool F_StartBacklightTimer = false;//timer flag for LCD screen
bool F_StartContactorTimer = false;//timer flag for contactor
bool F_refSetContactor = false;// flag for the reference time.
unsigned long previousMillis = 0;
int previousState;

//PIN NUMBERS:
const int ADC_KEYPAD = 0;//   Keypad ADC input read pin
const int ADC_VBATTERY = 1;// Battery Voltage ADC read pin
const int CONTACTOR = 8;//    Contactor relay switch output pin
const int BUZZER = 2;//       Buzzer switch output pin
const int LCD_BACKLIGHT = 10;//Backlight switch output pin

//VARIABLES FOR RTC:
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String RTC_year;
String RTC_month;
String RTC_day;
String RTC_hours = "00";
String RTC_minutes = "00";
String RTC_seconds = "00";
int setYear, setMonth, setDay, setHours, setMinutes, setSeconds;
String strYear;
String strMonth;
String strDay;
String strHours;
String strMinutes;
String strSeconds;
int refLCDhours, refLCDminutes, refLCDseconds;
int refConthours, refContminutes, refContseconds;
int rtchours, rtcminutes, rtcseconds;
int h;
    int hh;
//int refSecondsContactor;

//VARIALBES FOR VOLTAGE READER:
float vout = 0.00;
float vin = 0.00;
float R1 = 110000;
float R2 = 10000;
float Vbattery = 0;

//VARIABLES FOR THE KEYPAD:
char currentKey = ' ';
char inputMonth1, inputMonth2;
char inputDay1, inputDay2;
char inputDateNums[8] = {' ',' ',' ',' ',' ',' ',' ',' '};//this variable gets changed ALOT!!! always read from eeprom storage and load it inside this variable
char inputTimeNums[6] = {' ',' ',' ',' ',' ',' '};//this variable gets changed ALOT!!! always read from eeprom storage and load it inside this variable

//TIMER VARIABLES:
int LCDtimer[6];
int RoutineTimerStart[6];
int RoutineTimerEnd[6];
int cutoffVoltage[3];



void InitSystemTimer(void)
{
  currentMillis = millis();
}

void CheckTimer(void)
{
  
  if (currentMillis - previousMillis >= 1000) 
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    //Set 1 Second timer flags HERE:


  }
  if (currentMillis - previousMillis >= 5000) 
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    //Set 5 Second timer flags HERE:


  }
}

String convertRTCtime(String hours, String minutes, String seconds)
{
  int hour1, minute1, second1;
  String output1, output2, output3, total, zero = "0";
  
  hour1 = hours.toInt();
  minute1 = minutes.toInt();
  second1 = seconds.toInt();

  Serial.println(hour1);
  Serial.println(minute1);
  Serial.println(second1);
  Serial.println("\n\n");
  
  if( hour1 < 10)
  {
    //convert back to string:
    hours = String(hour1);
    //add the placeholder zero:
    hours = zero + hours;
//    Serial.println(hours);
  }
  if( minute1 < 10)
  {
    //convert back to string:
    minutes = String(minute1);
    //add the placeholder zero:
    minutes = zero + minutes;
//    Serial.println(minutes);
  }
  if( second1 < 10)
  {
    //convert back to string:
    seconds = String(second1);
    //add the placeholder zero:
    seconds = zero + seconds;
//    Serial.println(seconds);
  }

  //Now add al the strings together:
  total = hours + minutes + seconds;
//  Serial.print("Check total converted time: ");
//  Serial.println(total);
  return (total);
  
}

String getTimeFromEEPROM(int list[], int range)
{
  //    HD        TD        D        H        T         E
  String byteOne, byteTwo, byteThree, byteFour, byteFive, byteSix;
  String total;

  if(range == 3)//this means it the the cutoff voltage list
  {
    byteOne = list[0];// example byteOne = 1
    byteTwo = list[1];// example byteTwo = 5
    byteThree = list[2];// example byteOne = 2
  }
  if(range == 6)//This means it is the timer values list
  {
    byteOne   = String(list[0]);// example byteOne = 1
    byteTwo   = String(list[1]);// example byteTwo = 5
    byteThree = String(list[2]);// example byteOne = 1
    byteFour  = String(list[3]);// example byteTwo = 5
    byteFive  = String(list[4]);// example byteOne = 1
    byteSix   = String(list[5]);// example byteTwo = 5

    total = byteOne + byteTwo + byteThree + byteFour + byteFive + byteSix;
    //Serial.print("CONVERTED EEPROM VALUES TO STRING: ");
    //Serial.println(total);
    return total;
  }

  
  
}

String ModulusOfNumber_toStringNumber(int number)
{
  int E, T, H, D;

  E = number % 10;
  number /= 10;
  T = number % 10;
  number /= 10;
  H = number % 10;
  number /= 10;
  D = number % 10;

  String num = String(D) + String(H) + String(T) + String(E);
  return num;
}


//########################################################################################
//#######   ADC READ INPUTS   ############################################################
//########################################################################################

int BatteryVoltageLevel(void)
{
  //map(value, fromLow, fromHigh, toLow, toHigh)
  vout = (analogRead(ADC_VBATTERY) * 5.0) / 1024.0; // see text
  vin = vout / (R2/(R1+R2)); 
  if (vin<0.09) {
    vin=0.0;//statement to quash undesired reading !
  }
  //Return the ADC input value read from the battery bank voltage level:
  return map(vin, 0,5, 0,60);
}

int ADC_Read_Keypad_Input(void)
{
  //Return the ADC input value read from the keypad
  return analogRead(ADC_KEYPAD);
}

//########################################################################################
//#######   ADC BATTERY VOLTAGE FUNCTIONS   ##############################################
//########################################################################################

void Battery_Voltage(void)
{
  //Checking voltage level:
  if (BatteryVoltageLevel() < 12)
  {
    //if voltage is below set voltage, switch over to ESKOM:
    F_SwitchContactorOFF = true;
    //Change enum to check switching timer for switch contactor
    STATES = S_switchContactor;
  }
  else if (BatteryVoltageLevel() >= 13)
  {
    //if voltage is above set voltage, switch over to SOLAR:
    F_SwitchContactorOFF = false;
    //Change enum to check switching timer for switch contactor
    STATES = S_switchContactor;
  }
}

//########################################################################################
//#######   ADC KEYPAD FUNCTIONS   #######################################################
//########################################################################################

void Keypad_Key(int Key)
{
  if (Key > 0 && F_KeyPressed != true)
  {
    if (Key > 59 && Key < 70)//0
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '0';
      // Serial.println("Key pressed: 0");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");

      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("0");
    }
    if (Key > 700 && Key < 770)//1
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '1';
      // Serial.println("Key pressed: 1");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");

      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("1");
    }
    if (Key > 590 && Key < 599)//2
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '2';
      // Serial.print("Key pressed: 2");
      // Serial.print("\tADC: ");
      // Serial.println(Key);
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("2");
    }
    if (Key > 490 && Key < 520)//3
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '3';
      // Serial.println("Key pressed: 3");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("3");
    }
    if (Key > 560 && Key < 570)//4
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '4';
      // Serial.print("Key pressed: 4");
      // Serial.print("\tADC: ");
      // Serial.println(Key);
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("4");
    }
    if (Key > 390 && Key < 410)//5
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '5';
      // Serial.println("Key pressed:5");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("5");
    }
    if (Key > 290 && Key < 312)//6
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '6';
      // Serial.println("Key pressed: 6");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("6");
    }
    if (Key > 215 && Key < 239)//7
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '7';
      // Serial.println("Key pressed: 7");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("7");
    }
    if (Key > 158 && Key < 170)//8
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '8';
      // Serial.println("Key pressed: 8");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("8");
    }
    if (Key > 120 && Key < 130)//9
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '9';
      // Serial.println("Key pressed: 9");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("9");
    }
    if (Key > 420 && Key < 440)//A
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = 'A';
      // Serial.println("Key pressed: A");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("A");
    }
    if (Key > 230 && Key < 250)//B
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = 'B';
      // Serial.print("Key pressed: B");
      // Serial.print("\tADC: ");
      // Serial.println(Key);
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("B");
    }
    if (Key > 90 && Key < 110)//C
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = 'C';
      // Serial.print("Key pressed: C");
      // Serial.print("\tADC: ");
      // Serial.println(Key);
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("C");
    }
    if (Key > 39 && Key < 43)//D
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = 'D';
      // Serial.println("Key pressed: D");
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("D");
    }
    if (Key > 44 && Key < 52)//#
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '#';
      // Serial.print("Key pressed: #");
      // Serial.print("\tADC: ");
      // Serial.println(Key);
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("#");
    }
    if (Key > 70 && Key < 80)//*
    {
      F_KeyPressed = 1;//the key is pressed or being pressed! you need to release the key to carry on
      currentKey = '*';
      // Serial.print("Key pressed: *");
      // Serial.print("\tADC: ");
      // Serial.println(Key);
      // Nokia_SetCursor(15,4);
      // Nokia_DrawText("Key -> ");
      
      // Nokia_ClearLine(15,57,0);
      // Nokia_SetCursor(15,57);
      // Nokia_DrawText("*");
    }
  }
  else if (Key == 0)
  {
    F_KeyPressed = false;
    currentKey == ' ';
  }

  
  
}

//########################################################################################
//#######   LCD DISPLAY FUNCTIONS   ######################################################
//########################################################################################

void Nokia_Init(void)
{
  display.begin();
  display.clearDisplay();
  // you can change the contrast around to adapt the display
  // for the best viewing!
  display.setContrast(50);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);
// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
//Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);
// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!
}

void Nokia_ViewDisplay(void)
{
  display.display();
}

void Nokia_ClearDisplay(void)
{
  display.clearDisplay();
  display.display();
}

// void Nokia_DisplayMainScreen(void)
// {
//   Nokia_ClearDisplay();
//   Nokia_drawMainrect();
//   Nokia_SetCursor(2,4);
//   Nokia_DrawText(" MAIN  CLOCK ");
//   Nokia_DrawLine(0,10,84,0);
//   Nokia_DrawLine(0,20,84,0);

//   Nokia_SetCursor(22,4);
//   Nokia_DrawText("BATTERY VOLTS");
//   Nokia_DrawLine(0,30,84,0);

//   Nokia_SetCursor(35,18);
//   //Nokia_DrawText("47.5 Vdc");
// }

void Nokia_ClearLine(int col, int row, int length)//2, 27, 4
{
  display.setCursor(row, col);
  display.drawRect(row, col, 5, 7, BLACK);
  for ( int i = 0; i < (length*6+1); (i = i + 6))
  {
    display.fillRect(row + i, col, 5, 7, WHITE);
  }

  display.display();
}

void Nokia_SetCursor(int col, int row)
{
  display.setCursor(row, col);

  display.display();
}

void Nokia_DrawText(char string[])
{
  display.write(string);

  display.display();
}

void Nokia_PrintText(char ch)
{
  display.print(ch);
}

void Nokia_PrintString(String ch)
{
  display.print(ch);
}

void Nokia_PrintNum(int num)
{
  display.print(num);
}

void Nokia_DrawChar(int ch)//used to draw the up/down arrows
{
  display.write(ch);
}

void Nokia_Display_RTC_Time(int col, int row)
{
  Nokia_ClearLine(col, row,8);
  //delay(250);
  Nokia_SetCursor(col, row);
  //delay(250);
  Nokia_PrintString(RTC_hours);
  //delay(250);
  Nokia_DrawText(":");
  //delay(250);
  Nokia_PrintString(RTC_minutes);
  //delay(250);
  Nokia_DrawText(":");
  //delay(250);
  Nokia_PrintString(RTC_seconds);
  display.display();
  //delay(250);
  //display.display();
}

void Nokia_Display_Volts(int col, int row)
{
  Nokia_ClearLine(col, row,5);
  Nokia_SetCursor(col, row);
  Nokia_PrintNum(BatteryVoltageLevel());
  //Nokia_PrintString(BatteryVoltageLevel());
  Nokia_DrawText(" V");
  //Serial.println(BatteryVoltageLevel());
}

void Nokia_UpdateDateTimeSection(void)
{
  int spacesToMove = 0;
  int chSpaces[] = {0,6,12,18,30,36,48,54};//loactions for this -> yyyy-mm-dd
  bool Done = false;

  Nokia_SetCursor(15,2);
  Nokia_DrawText("Set Date:");
  Nokia_SetCursor(25,11);
  Nokia_DrawText("YYYY-MM-DD");

  while(Done != true)//keep repeating until spaces to move = 0
  {
    //Write the new location for the ch:
    Nokia_SetCursor(35,(11+chSpaces[spacesToMove]));
    Nokia_DrawChar(24);//up arrow character
    //Get the user input from the keypad:
    Keypad_Key(ADC_Read_Keypad_Input());
    //update/overwrite the section on the screen where user need to change values:
    //First clear the input space holder to overwrite:
    //Nokia_ClearLine(26,(11+chSpaces[spacesToMove]),0);
    //Then overwrite this location with user input:
    if( currentKey != ' ')
    {
      Nokia_ClearLine(25,(11+chSpaces[spacesToMove]),0);
      Nokia_SetCursor(25,(11+chSpaces[spacesToMove]));
      Nokia_PrintText(currentKey);
      //Now delete old ch location:
      Nokia_ClearLine(35,(11+chSpaces[spacesToMove]),0);
      
      //Save the user input:
      inputDateNums[spacesToMove] = currentKey;
      spacesToMove += 1;//increment the counter variable
      currentKey = ' ';
      if(spacesToMove == 8)
      {
        spacesToMove = 10;
      }
    }

    if(spacesToMove > 7)// if all spaces/fields are filled, then 
    {
      //Exit this while loop to continue with rest of the program
      Done = true;
    }
    delay(100);
  }

  //NOW LETS do the same with the time
  spacesToMove = 0;
  int chSpaces1[] = {0,6, 18,24,  36,42};//loactions for this -> yyyy-mm-dd
  Done = false;

  Nokia_ClearLine(15,2,9);
  Nokia_SetCursor(15,2);
  Nokia_DrawText("Set Time:");
  Nokia_ClearLine(25,11,9);
  Nokia_SetCursor(25,11);
  Nokia_DrawText("HH:MM:SS");//0,6, 18,24,  36,42

  while(Done != true)//keep repeating until spaces to move = 0
  {
    //Write the new location for the ch:
    Nokia_SetCursor(35,(11+chSpaces1[spacesToMove]));
    Nokia_DrawChar(24);//up arrow character
    //Get the user input from the keypad:
    Keypad_Key(ADC_Read_Keypad_Input());
    //update/overwrite the section on the screen where user need to change values:
    //First clear the input space holder to overwrite:
    //Nokia_ClearLine(26,(11+chSpaces[spacesToMove]),0);
    //Then overwrite this location with user input:
    if( currentKey != ' ')
    {
      Nokia_ClearLine(25,(11+chSpaces1[spacesToMove]),0);
      Nokia_SetCursor(25,(11+chSpaces1[spacesToMove]));
      Nokia_PrintText(currentKey);
      //Now delete old ch location:
      Nokia_ClearLine(35,(11+chSpaces1[spacesToMove]),0);
      
      //Save the user input:
      inputTimeNums[spacesToMove] = currentKey;
      spacesToMove += 1;//increment the counter variable
      currentKey = ' ';
      if(spacesToMove == 6)
      {
        spacesToMove = 10;
      }
    }

    if(spacesToMove > 5)// if all spaces/fields are filled, then 
    {
      //Exit this while loop to continue with rest of the program
      Done = true;
    }
    delay(100);
  }

  //Write date and time to RTC:
  strYear += inputDateNums[0];
  strYear += inputDateNums[1];
  strYear += inputDateNums[2];
  strYear += inputDateNums[3];

  setYear = strYear.toInt();

  if( inputDateNums[4] != '0')
  {
    strMonth += inputDateNums[4];
  }
  strMonth += inputDateNums[5];

  setMonth = strMonth.toInt();

  if( inputDateNums[6] != '0')
  {
    strDay += inputDateNums[6];
  }
  strDay += inputDateNums[7];

  setDay = strDay.toInt();

  if( inputTimeNums[0] != '0')
  {
    strHours += inputTimeNums[0];
  }
  strHours += inputTimeNums[1];

  setHours = strHours.toInt();

  if( inputTimeNums[2] != '0')
  {
   strMinutes += inputTimeNums[2];
  }
  strMinutes += inputTimeNums[3];

  setMinutes = strMinutes.toInt();

  if( inputTimeNums[4] != '0')
  {
    strSeconds += inputTimeNums[4];
  }
  strSeconds += inputTimeNums[5];

  setSeconds = strSeconds.toInt();

  RTC_setDateTime(setYear, setMonth, setDay, setHours, setMinutes, setSeconds);

  Nokia_ClearLine(15,2,9);
  Nokia_ClearLine(25,11,9);

}

void Nokia_SetTimerSection(void)
{
  int spacesToMove = 0;
  int chSpaces1[] = {0,6, 18,24,  36,42};//loactions for this -> hh-mm-ss
  bool Done = false;

  Nokia_ClearLine(15,2,11);
  Nokia_SetCursor(15,2);
  Nokia_DrawText("Set Start:");
  Nokia_ClearLine(25,11,11);
  Nokia_SetCursor(25,11);
  Nokia_DrawText("HH:MM:SS");//0,6, 18,24,  36,42

  while(Done != true)//keep repeating until spaces to move = 0
  {
    //Write the new location for the ch:
    Nokia_SetCursor(35,(11+chSpaces1[spacesToMove]));
    Nokia_DrawChar(24);//up arrow character
    //Get the user input from the keypad:
    Keypad_Key(ADC_Read_Keypad_Input());
    //update/overwrite the section on the screen where user need to change values:
    //First clear the input space holder to overwrite:
    //Nokia_ClearLine(26,(11+chSpaces[spacesToMove]),0);
    //Then overwrite this location with user input:
    if( currentKey != ' ')
    {
      Nokia_ClearLine(25,(11+chSpaces1[spacesToMove]),0);
      Nokia_SetCursor(25,(11+chSpaces1[spacesToMove]));
      Nokia_PrintText(currentKey);
      //Now delete old ch location:
      Nokia_ClearLine(35,(11+chSpaces1[spacesToMove]),0);
      
      //Save the user input:
      inputTimeNums[spacesToMove] = currentKey;
      //Serial.println(inputTimeNums[spacesToMove]);//FOR DEBUGGING ONLY
      spacesToMove += 1;//increment the counter variable
      currentKey = ' ';
      if(spacesToMove == 6)
      {
        spacesToMove = 10;
      }
    }

    if(spacesToMove > 5)// if all spaces/fields are filled, then 
    {
      //Exit this while loop to continue with rest of the program
      Done = true;
    }
    delay(100);
  }

  //Save the timer start times to the eeprom:
  //HOURS
  internal_EEPROM_update(0x01,inputTimeNums[0]);
//  Serial.println(inputTimeNums[0]);
//  Serial.print("Start Time Hours directly read from eeprom: ");
//  Serial.println(internal_EEPROM_read(0));
//  Serial.println("\n\n");
  internal_EEPROM_update(0x02,inputTimeNums[1]);
  //MINUTES
  internal_EEPROM_update(0x03,inputTimeNums[2]);
  internal_EEPROM_update(0x04,inputTimeNums[3]);
  //SECONDS
  internal_EEPROM_update(0x05,inputTimeNums[4]);
  internal_EEPROM_update(0x06,inputTimeNums[5]);
  delay(10);

  Done  = false;
  spacesToMove = 0;
  Nokia_ClearLine(15,2,11);
  Nokia_ClearLine(15,2,11);
  Nokia_SetCursor(15,2);
  Nokia_DrawText("Set End:");
  Nokia_ClearLine(25,11,9);
  Nokia_SetCursor(25,11);
  Nokia_DrawText("HH:MM:SS");//0,6, 18,24,  36,42

  while(Done != true)//keep repeating until spaces to move = 0
  {
    //Write the new location for the ch:
    Nokia_SetCursor(35,(11+chSpaces1[spacesToMove]));
    Nokia_DrawChar(24);//up arrow character
    //Get the user input from the keypad:
    Keypad_Key(ADC_Read_Keypad_Input());
    //update/overwrite the section on the screen where user need to change values:
    //First clear the input space holder to overwrite:
    //Nokia_ClearLine(26,(11+chSpaces[spacesToMove]),0);
    //Then overwrite this location with user input:
    if( currentKey != ' ')
    {
      Nokia_ClearLine(25,(11+chSpaces1[spacesToMove]),0);
      Nokia_SetCursor(25,(11+chSpaces1[spacesToMove]));
      Nokia_PrintText(currentKey);
      //Now delete old ch location:
      Nokia_ClearLine(35,(11+chSpaces1[spacesToMove]),0);
      
      //Save the user input:
      inputTimeNums[spacesToMove] = currentKey;
      //Serial.println(inputTimeNums[spacesToMove]);//FOR DEBUGGING ONLY
      spacesToMove += 1;//increment the counter variable
      currentKey = ' ';
      if(spacesToMove == 6)
      {
        spacesToMove = 10;
      }
    }

    if(spacesToMove > 5)// if all spaces/fields are filled, then 
    {
      //Exit this while loop to continue with rest of the program
      Done = true;
    }
    delay(100);
  }

  //Save the timer finish times to the eeprom:
  //HOURS
  internal_EEPROM_update(0x07,inputTimeNums[0]);
  internal_EEPROM_update(0x08,inputTimeNums[1]);
  //MINUTES
  internal_EEPROM_update(0x09,inputTimeNums[2]);
  internal_EEPROM_update(0x0A,inputTimeNums[3]);
  //SECONDS
  internal_EEPROM_update(0x0B,inputTimeNums[4]);
  internal_EEPROM_update(0x0C,inputTimeNums[5]);
  delay(10);
}

void Nokia_SetBatteryVoltageCutoff(void)
{
  bool Done = false;
  int initVolts = 10;
  int E, T, H;

  Nokia_ClearLine(15,2,11);
  Nokia_SetCursor(15,2);
  Nokia_DrawText("Set Bat V:");
  Nokia_ClearLine(25,11,11);
  Nokia_SetCursor(32,10);
  Nokia_DrawText("0010 V");//0,6, 18 VV V

  //Show user how to increase and de crease voltage:
  Nokia_SetCursor(27,48);
  Nokia_DrawChar(24);//up arrow character
  Nokia_ViewDisplay();
  
  Nokia_SetCursor(35,48);
  Nokia_DrawChar(25);//down arrow character
  Nokia_ViewDisplay();

  Nokia_SetCursor(27,55);
  Nokia_DrawChar(65);//A character
  Nokia_ViewDisplay();

  Nokia_SetCursor(35,55);
  Nokia_DrawChar(66);//B character
  Nokia_ViewDisplay();

  Nokia_SetCursor(27,62);
  Nokia_DrawChar(67);//C character
  Nokia_ViewDisplay();

  Nokia_SetCursor(35,62);
  Nokia_DrawChar(68);//D character
  Nokia_ViewDisplay();
  
  while(Done != true)
  {
    
    //keep user in this loop until button # is pressed:
    Keypad_Key(ADC_Read_Keypad_Input());//this will give me the current pressed key

    if(currentKey == '#')
    {
      Done = true;//exit the while loop
    }

    if(currentKey == 'A')//increase voltage by 1
    {
      initVolts += 1;
       
      Nokia_ClearLine(32,10,6);
      Nokia_SetCursor(32,10);
      Nokia_PrintString(ModulusOfNumber_toStringNumber(initVolts));
      Nokia_DrawText(" V");//0,6, 18 VV V
      currentKey = ' ';
    }
    if(currentKey == 'B')//increase voltage by 1
    {
      initVolts -= 1;
       
      Nokia_ClearLine(32,10,6);
      Nokia_SetCursor(32,10);
      Nokia_PrintString(ModulusOfNumber_toStringNumber(initVolts));
      Nokia_DrawText(" V");//0,6, 18 VV V
      currentKey = ' ';
    }
    if(currentKey == 'C')//increase voltage by 10
    {
      initVolts += 10;
       
      Nokia_ClearLine(32,10,6);
      Nokia_SetCursor(32,10);
      Nokia_PrintString(ModulusOfNumber_toStringNumber(initVolts));
      Nokia_DrawText(" V");//0,6, 18 VV V
      currentKey = ' ';
    }
    if(currentKey == 'D')//decrease voltage by 10
    {
      initVolts -= 10;
       
      Nokia_ClearLine(32,10,6);
      Nokia_SetCursor(32,10);
      Nokia_PrintString(ModulusOfNumber_toStringNumber(initVolts));
      Nokia_DrawText(" V");//0,6, 18 VV V
      currentKey = ' ';
    }
    delay(100);
  }

  //Save the set battery voltage:
  E = initVolts % 10;// get the ones
  initVolts /= 10;// remove the ones

  T = initVolts % 10;// get the tenths
  initVolts /= 10;// remove the tenths

  H = initVolts % 10;// get the hundreds

  internal_EEPROM_update(0x13,E);
  internal_EEPROM_update(0x14,T);
  internal_EEPROM_update(0x15,H);
}

void Nokia_SetBacklightTimer(void)
{
  int spacesToMove = 0;
  int chSpaces1[] = {0,6, 18,24,  36,42};//loactions for this -> hh-mm-ss
  bool Done = false;

  Nokia_ClearLine(15,2,11);
  Nokia_SetCursor(15,2);
  Nokia_DrawText("Set B Timer:");
  Nokia_ClearLine(25,11,11);
  Nokia_SetCursor(25,11);
  Nokia_DrawText("HH:MM:SS");//0,6, 18,24,  36,42

  while(Done != true)//keep repeating until spaces to move = 0
  {
    //Write the new location for the ch:
    Nokia_SetCursor(35,(11+chSpaces1[spacesToMove]));
    Nokia_DrawChar(24);//up arrow character
    //Get the user input from the keypad:
    Keypad_Key(ADC_Read_Keypad_Input());
    //update/overwrite the section on the screen where user need to change values:
    //First clear the input space holder to overwrite:
    //Nokia_ClearLine(26,(11+chSpaces[spacesToMove]),0);
    //Then overwrite this location with user input:
    if( currentKey != ' ')
    {
      Nokia_ClearLine(25,(11+chSpaces1[spacesToMove]),0);
      Nokia_SetCursor(25,(11+chSpaces1[spacesToMove]));
      Nokia_PrintText(currentKey);
      //Now delete old ch location:
      Nokia_ClearLine(35,(11+chSpaces1[spacesToMove]),0);
      
      //Save the user input:
      inputTimeNums[spacesToMove] = currentKey;
      spacesToMove += 1;//increment the counter variable
      currentKey = ' ';
      if(spacesToMove == 6)
      {
        spacesToMove = 10;
      }
    }

    if(spacesToMove > 5)// if all spaces/fields are filled, then 
    {
      //Exit this while loop to continue with rest of the program
      Done = true;
    }
    delay(100);
  }

  //Save the timer start times to the eeprom:
  //HOURS
  internal_EEPROM_update(0x0D,inputTimeNums[0]);
  internal_EEPROM_update(0x0E,inputTimeNums[1]);
  //MINUTES
  internal_EEPROM_update(0x0F,inputTimeNums[2]);
  internal_EEPROM_update(0x10,inputTimeNums[3]);
  //SECONDS
  internal_EEPROM_update(0x11,inputTimeNums[4]);
  internal_EEPROM_update(0x12,inputTimeNums[5]);
  delay(10);
  
}

void Nokia_drawMainrect(void) 
{
  display.drawRect(0, 0, display.width(), display.height(), BLACK);
  display.display();
}

// void Nokia_testdrawchar(void) 
// {
//   display.setTextSize(1);
//   display.setTextColor(BLACK);
//   display.setCursor(0,0);

//   for (uint8_t i=24; i < 168; i++) {
//     if (i == '\n') continue;
//     display.write(i);
//     //if ((i > 0) && (i % 14 == 0))
//       //display.println();
//   }    
//   display.display();
// }

void Nokia_DrawLine(int xStart, int yStart, int xEnd, int yEnd)
{
  display.drawLine(xStart, yStart, xStart + xEnd, yStart + yEnd, BLACK);
  display.display();
}

//#############################################################################################
//#########   I2C EEPROM   ####################################################################
//#############################################################################################

// void I2C_Finder(void)
// {
//   bool done = false;

//   Serial.println("\nI2C Scanner");
//   while (done != true)
//   {
//     int nDevices = 0;
//     Serial.println("Scanning...");
//     for (byte address = 1; address < 127; ++address) 
//     {
//       // The i2c_scanner uses the return value of
//       // the Write.endTransmisstion to see if
//       // a device did acknowledge to the address.
//       Wire.beginTransmission(address);
//       byte error = Wire.endTransmission();

//       if (error == 0) 
//       {
//         Serial.print("I2C device found at address 0x");
//         if (address < 16) 
//         {
//           Serial.print("0");
//         }
//         Serial.print(address, HEX);
//         Serial.println("  !");

//         ++nDevices;
//       }
//       else if (error == 4) 
//       {
//         Serial.print("Unknown error at address 0x");
//         if (address < 16) 
//         {
//           Serial.print("0");
//         }
//         Serial.println(address, HEX);
//       }
//     } 
//     if (nDevices == 0) 
//     {
//       Serial.println("No I2C devices found\n");
//     } 
//     else 
//     {
//       Serial.println("done\n");
//       done = true;
//     }
//   }
// }

void internal_EEPROM_update(int address, int value)
{
  EEPROM.update(address, value);
  delay(500);
}

byte internal_EEPROM_read(int address)
{
  return EEPROM.read(address);
}
//EEPROM ADDRESSES ON CARD:
/*
    0x50
    0x51
    0x52
    0x53
    0x54
    0x55
    0x56
    0x57
*/

void ReadtimerValues(int Main_Loop_TimerStart, int Main_Loop_TimerEnd, int LCD_Backlight_startAddress, int SetVoltage_Address)
{
  int lcdbacklighttimer = 13;//address where the LCD back light timer HH MM SS are stored
  int mainlooptimerstart = 1;//address of the main loop timer start time
  int mainlooptimerend = 7;//address of the main loop timer end time
  int cutoffvoltage = 19;//address of the set cutoff voltage E T H
  Serial.println("Load timer values from EEPROM:\n\n");
  for( int i = 0; i < 6; i++)
  {
    LCDtimer[i] = (internal_EEPROM_read(lcdbacklighttimer) - 48);
    lcdbacklighttimer++;//go to the next address
//    Serial.print("LCDtimer List: ");
//    Serial.println(LCDtimer[i]);
    delay(100);
    RoutineTimerStart[i] = (internal_EEPROM_read(mainlooptimerstart) - 48);
    mainlooptimerstart++;//go to the next address
//    Serial.print("Start List: ");
//    Serial.println(RoutineTimerStart[i]);
    delay(100);
    RoutineTimerEnd[i] = (internal_EEPROM_read(mainlooptimerend) - 48);
    mainlooptimerend++;//go to the next address    
//    Serial.print("End List: ");
//    Serial.println(RoutineTimerEnd[i]);
    delay(100);
  }
//  Serial.print("Read value from eeprom before conversion: ");
//  Serial.println(RoutineTimerStart[0]);
//  Serial.print("Read value from eeprom after conversion: ");
//  Serial.println(String(RoutineTimerStart[0]));
  

  for( int i = 0; i < 3; i++)
  {
    cutoffVoltage[i] = (internal_EEPROM_read(cutoffvoltage)- 48);
    cutoffvoltage++;//go to the next address
//    Serial.print("Cutoff List: ");
//    Serial.println(cutoffVoltage[i]);
    delay(100);
  }

  //FOR DEBUGING ONLY:
//  Serial.println("LCD Timer settings");
//  Serial.print(LCDtimer[0]);
//  Serial.print("  ");
//  Serial.print(LCDtimer[1]);
//  Serial.print(" : ");
//  Serial.print(LCDtimer[2]);
//  Serial.print("  ");
//  Serial.print(LCDtimer[3]);
//  Serial.print(" : ");
//  Serial.print(LCDtimer[4]);
//  Serial.print("  ");
//  Serial.println(LCDtimer[5]);
//
//  Serial.println("Start Time:");
//  Serial.print(RoutineTimerStart[0]);
//  Serial.print("  ");
//  Serial.print(RoutineTimerStart[1]);
//  Serial.print(" : ");
//  Serial.print(RoutineTimerStart[2]);
//  Serial.print("  ");
//  Serial.print(RoutineTimerStart[3]);
//  Serial.print(" : ");
//  Serial.print(RoutineTimerStart[4]);
//  Serial.print("  ");
//  Serial.println(RoutineTimerStart[5]);
//
//  Serial.println("End Time");
//  Serial.print(RoutineTimerEnd[0]);
//  Serial.print("  ");
//  Serial.print(RoutineTimerEnd[1]);
//  Serial.print(" : ");
//  Serial.print(RoutineTimerEnd[2]);
//  Serial.print("  ");
//  Serial.print(RoutineTimerEnd[3]);
//  Serial.print(" : ");
//  Serial.print(RoutineTimerEnd[4]);
//  Serial.print("  ");
//  Serial.println(RoutineTimerEnd[5]);
}

void DefaultPassword(void)
{
  internal_EEPROM_update(0x21,0x2A);//* star character
  internal_EEPROM_update(0x22,0x2A);//* star character
  internal_EEPROM_update(0x23,0x2A);//* star character
  internal_EEPROM_update(0x24,0x2A);//* star character
}

//#############################################################################################
//#########   REAL TIME CLOCK   ###############################################################
//#############################################################################################

void RTC_setDateTime(unsigned int year, unsigned int month, unsigned int day, unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  if (rtc.isrunning())
  {
    rtc.adjust(DateTime(year, month, day, hours, minutes, seconds));
  }
  else
  {
    Serial.println("RTC is NOT running!!!");
  }
}


void RTC_getInfo(void)
{
  DateTime now = rtc.now();
  //delay(100);
  RTC_year = String(now.year());
  RTC_month = String(now.month());
  RTC_day = String(now.day());

  RTC_hours = now.hour();
  RTC_minutes = now.minute();
  RTC_seconds = now.second();
  delay(250);
  if (RTC_hours == "")
  {
    RTC_hours = "00";
  }
  if (RTC_minutes == "")
  {
    RTC_minutes = "00";
  }
  if (RTC_seconds == "")
  {
    RTC_seconds = "00";
  }

  Serial.print("Hours:");
  Serial.println(RTC_hours);
  Serial.print("Minutes:");
  Serial.println(RTC_minutes);
  Serial.print("Seconds:");
  Serial.println(RTC_seconds);
}

void DisplayRefNow(void)
{
  // RTC_getInfo();
  // TimeSpan tsContactor (0,RTC_hours.toInt(),RTC_minutes.toInt(),RTC_seconds.toInt());//days hours minutes seconds
}

void DisplayRefLater(const TimeSpan& ts)
{
  
  // RTC_getInfo();
  // TimeSpan tsNow (0,RTC_hours.toInt(), RTC_minutes.toInt(), RTC_seconds.toInt()); 

  // TimeSpan tsDiff = tsNow - ts;
  // Serial.print("Print tsContactor hours: ");
  // Serial.println(ts.hours());
  // Serial.print("Print tsNow hours: ");
  // Serial.println(tsNow.hours());
  
  // Serial.print("Printing the hours: ");
  // Serial.println(tsDiff.hours(), DEC);
}

void ShowMainScreen(void)
{
  //Done setting the timers, go to the main display screen:
    Nokia_ClearDisplay();
    Nokia_drawMainrect();
    Nokia_SetCursor(2,4);
    Nokia_DrawText(" MAIN  CLOCK ");
    Nokia_DrawLine(0,10,84,0);
    Nokia_Display_RTC_Time(12,19);
    Nokia_DrawLine(0,20,84,0);

    Nokia_SetCursor(22,4);
    Nokia_DrawText("BATTERY VOLTS");
    Nokia_DrawLine(0,30,84,0);
    Nokia_Display_Volts(35,35);
    Nokia_SetCursor(35,18);
    //Nokia_DrawText("47.5 Vdc");
    //Serial.println("Done drawing screen stuff");
}

void MasterInit(void)
{
  //this means the device is running for the very first time.
    //Run the setup function to initialise the clock and timers:
    Nokia_ClearDisplay();
    Nokia_ViewDisplay();
    Nokia_drawMainrect();
  
    Nokia_SetCursor(2,4);
    Nokia_DrawText("RECYCLE TIMER");
    Nokia_DrawLine(0,10,84,0);
    delay(2000);
    
    // Nokia_SetCursor(12,15);
    // Nokia_DrawText("Main Menu");
    // Nokia_DrawLine(14,20,54,0);
    Nokia_ClearLine(2,4,12);
    Nokia_SetCursor(2,27);
    Nokia_DrawText("SETUP");
    Nokia_DrawLine(0,10,84,0); 

    //Set the date and time for the unit:
    Nokia_UpdateDateTimeSection();
    Nokia_ClearLine(15,2,10);
    Nokia_SetCursor(15,2);
    Nokia_DrawText("Clock set!");
    delay(1000);

    Nokia_ClearLine(15,2,10);
    Nokia_SetCursor(15,2);
    Nokia_DrawText("Set Timer:");
    delay(1000);

    //Set the main looping timer:
    Nokia_SetTimerSection();// Start: 0 1   2 3   4 5    End: 6 7    8 9   10 11   HH MM SS
    Nokia_ClearLine(15,2,10);
    Nokia_SetCursor(15,2);
    Nokia_DrawText("Timer set!");
    delay(1000);

    //Set the LCD backlight "on" timer:
    Nokia_ClearLine(15,2,11);
    Nokia_SetCursor(15,2);
    Nokia_SetBacklightTimer();// 12 13    14 15     16 17
    Nokia_ClearLine(15,2,11);
    Nokia_SetCursor(15,2);
    Nokia_DrawText("Timer set!");
    delay(1000);


    //Set the detecting voltage level:
    Nokia_SetBatteryVoltageCutoff();// E - 18   T - 19    H - 20
    Nokia_ClearDisplay();
    Nokia_drawMainrect();
    Nokia_SetCursor(15,2);
    Nokia_DrawText("Cutoff set!");
    delay(1000);

    Nokia_ClearDisplay();
    Nokia_drawMainrect();
    Nokia_SetCursor(12,4);
    Nokia_DrawText("All timers");
    Nokia_SetCursor(22,4);
    Nokia_DrawText("are set!");
    delay(1000);

    //Done setting the timers, go to the main display screen:
    Nokia_ClearDisplay();
    Nokia_drawMainrect();
    Nokia_SetCursor(2,4);
    Nokia_DrawText(" MAIN  CLOCK ");
    Nokia_DrawLine(0,10,84,0);
    Nokia_DrawLine(0,20,84,0);

    Nokia_SetCursor(22,4);
    Nokia_DrawText("BATTERY VOLTS");
    Nokia_DrawLine(0,30,84,0);

    //Nokia_SetCursor(35,18);
    //Nokia_DrawText("00.0 Vdc");

    //Write 1st startup bit to eeprom, init settings are set,
    //no need to do it again if power loss.
    internal_EEPROM_update(0,0);
}

void Master_Reset_Init(void)
{
  //First Init the LCD to be able to display everything on the screen:
  //Nokia_Init();
  
  void InitSystemTimer(void);
  //Serial.println("check booting if statement");
  //Read the 1st time boot flag bit to see if the memory variable has changed:
  if(internal_EEPROM_read(0) != 1 && internal_EEPROM_read(0) != 0)
  {
    //Serial.println("Inside booting if statement");
    //if 1st startup bit is unknown, this means that device is 1st booted:
    internal_EEPROM_update(0,1);//write the startup bit to 1 to start initialize the settings
    //Set enum flag to run the init master reset function:
    //Serial.println("Changing state to init Setup");
    STATES = S_initsetup1;// going to set all settings
  }
  else if(internal_EEPROM_read(0) == 0)
  {
    //Serial.println("Inside booting else statement");
    //else if the startup bit is 0, the init settings was setup,
    //So do not write the init settings again and set the 
    //enum flag to run run the main program
    //Serial.println("Changing state to LCD display stuff");
    STATES = S_mainmenu;//going to display current readings
  }
  else if(internal_EEPROM_read(0) == 1)
  {
    //The EEPROM is set to 1, which means the init setup was not complete
    STATES = S_initsetup1;// going to set all settings
  }
  //Serial.println("Booting if statement complete.");
  delay(10);
  //for debugging only:
  //Serial.println(internal_EEPROM_read(0));
}



//#############################################################################################
//#########   OPERATING SYSTEM   ##############################################################
//#############################################################################################
void OS_Setup(void)
{
  //Serial.println("Entering OS_Setup");
  //setup the pins on the microcontoller:
  pinMode(ADC_KEYPAD, INPUT);
  pinMode(ADC_VBATTERY, INPUT);
  pinMode(CONTACTOR, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT);

  //set the LCD backlight flag to true:
  F_BackLight = true;
  //switch on the LCD backlight:
  digitalWrite(LCD_BACKLIGHT, HIGH);
  //set the password: 
  DefaultPassword();
  // Get the main loop timer start time HH MM SS
  // Get the main loop timer end timer HH MM SS
  // Get LCD backlight timer HH MM SS
  // Get the cut off voltage value E T H
  ReadtimerValues(1,7,13,19);
  //set enum flag to run the master init setup:
  //Set the flag for the statemachine:
  STATES = S_initsetup0;
  
  
}


void OS_Run(void)
{
  //This is where  the main program is going to run:
  //Read keypad input
  //Read battery voltage
  //Read Main time clock
  //Repeat process

  Serial.println("Going into switch");
  switch (STATES)
  {
  case 0://Check if it is 1st time boot
    //Serial.println("Checking booting state");
    Master_Reset_Init();
    break;
  case 1://Setup the settings
    //Serial.println("Setup all settings");
    MasterInit();
    //Serial.println("Settings Done\nchanging state to display menu");
    //update the timers:
    ReadtimerValues(1,7,13,19);
    //When the settings are done, go to main loop
    STATES = S_mainmenu;//case 2
    break;
  case 2://Display the main menu on LCD
    //Serial.println("Display main stuff on screen");
    //Check the battery bank voltage level
    //Display the time, and voltage level
    //Start the LCD screen On/Off timer:
    F_StartBacklightTimer = true;

    ShowMainScreen();
    //Serial.println("changing state to idle mode");
    //go to idle screen:
    STATES = S_idlescreen;//case 9
    break;
  case 3://switch the LCD backlight on/off
    //check the flag bit of the backlight LED:
    if( F_BackLight == false)
    {
      //if the flag was false, this means the backlight is off
      //so switch it on:
      F_BackLight = true;
      digitalWrite(LCD_BACKLIGHT, HIGH);
      
      //Change the previous state to IDLE SCREEN:
      previousState = S_idlescreen;
      //Check if user wants to do a master reset:
      STATES = S_DoFactoryReset;//case 7
    }
    else
    {
      //else switch off the backlight:
      F_BackLight = false;
      digitalWrite(LCD_BACKLIGHT, LOW);
      //Change the previous state to IDLE SCREEN:
      previousState = S_idlescreen;
      //Check the voltage level:
      STATES = S_CheckVoltage;
    }
    break;
  case 4://Check Voltage:
    Battery_Voltage();//States are changing inside this function
    //STATES = S_CheckTimers;//case 4 - moved to cases 6
    //inside this function above, states change to case 6
    break;
  case 5://Check Timers:
    Serial.println("RTC time conversion: ");
    Serial.println(convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds));
    Serial.println("EEPROM start time conversion: ");
    Serial.println((getTimeFromEEPROM(RoutineTimerStart,6)));
    Serial.println("EEPROM end time conversion: ");
    Serial.println((getTimeFromEEPROM(RoutineTimerEnd,6)));
    Serial.println("convert to int: ");
    Serial.println((getTimeFromEEPROM(RoutineTimerEnd,6)).toInt());
    //For now check the main recycle timer if it is between 06h00 and 18h00:
    if( ((getTimeFromEEPROM(RoutineTimerStart,6)).toInt()) <= (convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds)).toInt() )//if start time is smaller than current time
    {
      Serial.println("Start Hour range is correct...");
      if( (getTimeFromEEPROM(RoutineTimerEnd, 6)) >= (convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds)) )
      {
        Serial.println("End Hour range is correct...");
        if( ((getTimeFromEEPROM(RoutineTimerStart, 6))) <= (convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds)) )
        {
          //Serial.println("Start Minute range is correct...");
          if( (getTimeFromEEPROM(RoutineTimerEnd, 6)) >= (convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds)) )
          {
            //Serial.println("End Minute range is correct...");
            if( ((getTimeFromEEPROM(RoutineTimerStart, 6))) <= (convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds)) )
            {
              //Serial.println("Start Second range is correct...");
              if( (getTimeFromEEPROM(RoutineTimerEnd, 6)) >= (convertRTCtime(RTC_hours, RTC_minutes, RTC_seconds)) ) 
              {
//                Serial.println("End Second range is correct, switching relay...");
//                Serial.println("Relay must be on");
                //if the hours is between the start time and end time, 
                //and the same with minutes and seconds, then keep the relay energized:
                F_SwitchContactorOFF = false;      
              }//end if end seconds 
              else
              {
//                Serial.println("End seconds range is not correct...");
//                Serial.println("Relay must be off");
                //if the hours is NOT between the start time and end time, 
                //and the same with minutes and seconds, then keep the relay OFF:
                F_SwitchContactorOFF = true;
                //Change enum to check switching timer for switch contactor
                STATES = S_switchContactor;
              }//end else start seconds     
            }//end if start seconds
            else
            {
//              Serial.println("Start seconds range is not correct...");
//              Serial.println("Relay must be off");
              //if the hours is NOT between the start time and end time, 
              //and the same with minutes and seconds, then keep the relay OFF:
              F_SwitchContactorOFF = true;
              //Change enum to check switching timer for switch contactor
              STATES = S_switchContactor;
            }//end else start seconds
          }//end if end minutes
          else
          {
//            Serial.println("End minute range is not correct...");
//            Serial.println("Relay must be off");
            //if the hours is NOT between the start time and end time, 
            //and the same with minutes and seconds, then keep the relay OFF:
            F_SwitchContactorOFF = true;
            //Change enum to check switching timer for switch contactor
            STATES = S_switchContactor;
          }//end else end minutes
        }//end if start minutes
        else
        {
//          Serial.println("Start minute range is not correct...");
//          Serial.println("Relay must be off");
          //if the hours is NOT between the start time and end time, 
          //and the same with minutes and seconds, then keep the relay OFF:
          F_SwitchContactorOFF = true;
          //Change enum to check switching timer for switch contactor
          STATES = S_switchContactor;
        }//end else start minutes
      }//end if end hours
      else
      {
//        Serial.println("End hour range is not correct...");
//        Serial.println("Relay must be off");
        //if the hours is NOT between the start time and end time, 
        //and the same with minutes and seconds, then keep the relay OFF:
        F_SwitchContactorOFF = true;
        //Change enum to check switching timer for switch contactor
        STATES = S_switchContactor;
      }//end else end hours
    }//end if start hours
    else
    {
//      Serial.println("Start hour range is not correct...");
//      Serial.println("Relay must be off");
      //if the hours is NOT between the start time and end time, 
      //and the same with minutes and seconds, then keep the relay OFF:
      F_SwitchContactorOFF = true;
      //Change enum to check switching timer for switch contactor
      STATES = S_switchContactor;
    }//end else start hours
    
      
      
      //Change enum to check switching timer for switch contactor
      STATES = S_switchContactor;
      //delay(10);
      //RTC_getInfo();
    
    break;
  case 6://Switch Contactor ON/OFF this state here is controlled
    // inside the battery_voltage() function in case 4
    //check the status of the flag:
    if( F_SwitchContactorOFF == true)
    {
      //if the flag is set to true, switch to ESKOM power
      digitalWrite(CONTACTOR, LOW);// contactor is not energized
      F_StartContactorTimer = true;
    }
    else
    {
      //if the flag is not set to true, switch to SOLAR power
      digitalWrite(CONTACTOR, HIGH);// contactor is energized
    }
    //after checking the battery voltage, check the timers:
    //RTC_getInfo();
    //delay(10);
    STATES = S_idlescreen;// S_CheckTimers;// case 5
    break;
  case 7://Reset ALL settings on device -> After reset must do init setup again
      //If * key is pressed, start polling timer for this key
      //when timer is done while this key was pressed, reset
      //To default, and re do settings:
      //Read the 1st keypress to switch on the LCD and display
      Keypad_Key(ADC_Read_Keypad_Input());//get the key input
      if(currentKey != ' ')//this means the user pushed a button, switch on the lcd
      {
        if(currentKey == '*')
        {
          RTC_getInfo();
          String holdSeconds = ""; 
          int check = 0, hold = 0;
          check = RTC_seconds.toInt();
          while (currentKey == '*')
          {
            //if user keeps * key pressed for 5 seconds,
            //Reset the whole unit:
            RTC_getInfo();
            hold = (check - (RTC_seconds.toInt()));
            if( hold == -5)//if 5 seconds has passed, then:
            {
              //Reset the settings flag:
              internal_EEPROM_update(0,9);
              //Change state to init setup
              STATES = S_initsetup0;// case 0
              currentKey = ' ';//break out of the loop
              break;//just to make sure you break out the loop
            }
          }
        }
      }
      
    break;
  case 8://Read any keypresses
    //Read the 1st keypress to switch on the LCD and display
    Keypad_Key(ADC_Read_Keypad_Input());//get the key input
    if(currentKey != ' ')//this means the user pushed a button, switch on the lcd
    {
      //Change state to switch on LCD backlight
      STATES = S_LCDbacklightSwitch;// case 3
      //set the backlight timer to true, to start the timer cycle:
      F_StartBacklightTimer = true;
    }
    else
    {
      //a button was not pressed, so go to read the voltage:
      STATES = S_CheckVoltage;// case 4
    }
    break;
  case 9://IDLE STATE, Displaying time and voltage
    RTC_getInfo();
    //RTC_getInfo();
//    DateTime now = rtc.now();
//    RTC_hours = now.hour();
//    RTC_minutes = now.minute();
//    RTC_seconds = now.second();

    
    //delay(100);
    Nokia_Display_RTC_Time(12,19);
    //delay(100);
    Nokia_Display_Volts(35,35);
    //delay(100);
    //Change state to check on all timer flags, then go check the voltage:
    //Save the next state:
    //previousState = S_CheckVoltage;// case 4
    //Change state to check if buttons are pressed
    STATES = S_CheckTimers;//S_CheckKeypress;// case 8
    break;
  case 10:
    //
    break;
  default:
    break;
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();
  Nokia_Init();
  OS_Setup();
  //RTC_setDateTime(2021, 02, 23, 18, 45, 30);
  delay(10);
  internal_EEPROM_update(0,9);//hard reset the config settings
  //STATES = S_CheckTimers;
//  Serial.println("Check timers before If statement");
//  Serial.print("\nStart time hour: ");
//  Serial.print(getTimeFromEEPROM(RoutineTimerStart, 0, 1));
//  Serial.println("\n");
//  Serial.print("Start time minute: ");
//  Serial.print(getTimeFromEEPROM(RoutineTimerStart, 2, 3));
//  Serial.println("\n");
//  Serial.print("Start time second: ");
//  Serial.print(getTimeFromEEPROM(RoutineTimerStart, 4, 5));
//  
//  Serial.print("\nEnd time hour: ");
//  Serial.println(getTimeFromEEPROM(RoutineTimerEnd, 0, 1));
//  Serial.print("End time minute: ");
//  Serial.println(getTimeFromEEPROM(RoutineTimerEnd, 2, 3));
//  Serial.print("End time second: ");
//  Serial.println(getTimeFromEEPROM(RoutineTimerEnd, 4, 5));
  digitalWrite(CONTACTOR, LOW);// contactor is energized
    
}

void loop() {
  // put your main code here, to run repeatedly:
  // put your main code here, to run repeatedly:
  delay(1000);
  
  //STATE MACHINE:
  OS_Run();
  //Serial.println("Did run the OS statemachine");
}
