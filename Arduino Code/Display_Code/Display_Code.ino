// ----------------------------
// Standard Libraries
// ----------------------------
#include <Ticker.h>

#include <PxMatrix.h>

#include <SD.h>
#include <sd_defines.h>
#include <sd_diskio.h>

#include <BluetoothSerial.h>
#include <BTAddress.h>
#include <BTAdvertisedDevice.h>
#include <BTScan.h>





// ----------------------------
// Animations
// ----------------------------
#include "Faces\Default.h" //Default
#include "Faces\StartUp.h" //StartUp





// ----------------------------
// Variables
// ----------------------------
//For Refreshing the Display
Ticker display_ticker;

//Bluetooth Serial
BluetoothSerial SerialBT;

// Handle received and sent messages
String message = "";
char incomingChar;
String CurrentFace = "";


// Pins for LED MATRIX
#ifdef ESP32
  #define P_LAT 22
  #define P_A 19
  #define P_B 23
  #define P_C 18
  #define P_D 5
  #define P_E 15
  #define P_OE 16
  hw_timer_t * timer = NULL;
  portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
#endif

#define matrix_width 64
#define matrix_height 64

#define RGB 565

#if RGB==565
  #define frame_size matrix_width*matrix_height*2
#else
  #define frame_size matrix_width*matrix_height*3
#endif

uint16_t frame_no=0;
unsigned long anim_offset=0;

union single_double{
  uint8_t two[2];
  uint16_t one;
} this_single_double;

// This defines the 'on' time of the display is us. The larger this number,
// the brighter the display. If too large the ESP will crash
uint8_t display_draw_time=10; //30-70 is usually fine

//PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);
//PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
PxMATRIX display(64,64,P_LAT, P_OE,P_A,P_B,P_C,P_D,P_E);

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);

uint16_t myCOLORS[8]={myRED,myGREEN,myBLUE,myWHITE,myYELLOW,myCYAN,myMAGENTA,myBLACK};



// ISR for display refresh
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}



void display_update_enable(bool is_enable)
{
  if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  }
  else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}



unsigned long getAnimOffset(uint8_t anim_no)
{
  unsigned long offset=0;
  for (uint8_t count=0;count<anim_no;count++)
  {
      offset=offset+animation_lengths[count]*frame_size;
  }
  //Serial.println("anim_no: " + String(anim_no) + ", length: " + String(animation_lengths[anim_no])+ ", offset: " + String(offset));
  return offset;
}



uint8_t line_buffer[matrix_width*2];



// This draws the pixel animation to the frame buffer in animation view
void draw_image ()
{
  unsigned long frame_offset=anim_offset+frame_no*frame_size;
  
  for (int yy=0;yy<matrix_height;yy++){
    memcpy_P(line_buffer, animations+frame_offset+yy*matrix_width*2, matrix_width*2);
    for (int xx=0;xx<matrix_width;xx++){

      this_single_double.two[0]=line_buffer[xx*2];
      this_single_double.two[1]=line_buffer[xx*2+1];
    
      display.drawPixelRGB565(xx,yy, this_single_double.one);
    }
    yield();
  }
}



void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32 LED Matrix"); //Bluetooth device name
  Serial.println("The ESP started, now you can pair it with bluetooth!");

  // Set driver chip type
  display.setDriverChip(FM6124);
  
  //myFile = SD.open(Currentface".txt");
  
  display.clearDisplay();
  display.setTextColor(myCYAN);
  display.setCursor(2,0);
}



void loop() {
  if (SerialBT.available()){
    char incomingChar = SerialBT.read();
    if (incomingChar != '\n'){
      message += String(incomingChar);

      //message = CurrentFace
    }
    else{
      message = "";
    }
    Serial.write(incomingChar);  
  }
  
 for (uint8_t anim_index=0; anim_index<sizeof(animation_lengths)-1;anim_index++)
  {
    // Display image or animation
    anim_offset=getAnimOffset(anim_index);
    frame_no=0;
    unsigned long start_time=millis();
    while ((millis()-start_time)<5000)
    {
       
      draw_image();
      display.showBuffer();
      //Serial.println(anim_offset);
      
      frame_no++;
      if (frame_no>=animation_lengths[anim_index])
        frame_no=0;
      delay(100);
      
    }

    // Display static
    start_time=millis();
    anim_offset=getAnimOffset(5);
    frame_no=0;  
    while ((millis()-start_time)<1000)
    {
      
      draw_image();
      display.showBuffer();
      frame_no++;
      if (frame_no>=animation_lengths[5])
        frame_no=0;
      delay(50);
      
    }
  }
}
