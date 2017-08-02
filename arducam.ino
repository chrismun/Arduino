#include <SoftwareSerial.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

//This demo can only work on OV2640_MINI_2MP or OV5642_MINI_5MP or OV5642_MINI_5MP_BIT_ROTATION_FIXED platform.
// #if !(defined OV5642_MINI_5MP || defined OV5642_MINI_5MP_BIT_ROTATION_FIXED || defined OV2640_MINI_2MP) 
//   #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
// #endif 
const int SPI_CS = 7; 
SoftwareSerial Xbee(2, 3); // RX, TX
boolean loop1 = true;

#if defined (OV2640_MINI_2MP)
 ArduCAM myCAM( OV2640, SPI_CS );
#else 
 ArduCAM myCAM( OV5642, SPI_CS );
#endif

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
#if defined(__SAM3X8E__)
  Wire1.begin();
  Serial.begin(115200);
#else
  Wire.begin();
  Serial.begin(115200);
  Xbee.begin(115200); // Set up both ports at this baud. This value is most important for the XBee. Make sure the baud rate matches the config setting of your XBee.
#endif
  Serial.println(F("ACK CMD ArduCAM Start!")); 
  // set the SPI_CS as an output: 
  pinMode(SPI_CS, OUTPUT); 
  // initialize SPI: 
  SPI.begin(); 
  while(loop1){ 
  //Check if the ArduCAM SPI bus is OK 
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55); 
    temp = myCAM.read_reg(ARDUCHIP_TEST1); 
    if (temp != 0x55)
      { 
        Serial.println(F("ACK CMD SPI interface Error!")); 
        delay(1000);
      } else { 
          Serial.println(F("ACK CMD SPI interface OK."));
         loop1 = false;
      } 
  }
loop1 = true;
#if defined (OV2640_MINI_2MP) //Check if the camera module type is OV2640 
  while(loop1){ 
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
      Serial.println(F("ACK CMD Can't find OV2640 module!"));
      delay(1000);
    } else {
      Serial.println(F("ACK CMD OV2640 detected."));
      loop1 = false;
    }
  }
loop1 = true;
  #else while(loop1)
  { //Check if the camera module type is OV5642 
    myCAM.wrSensorReg16_8(0xff, 0x01);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg16_8(OV5642_CHIPID_LOW, &pid);
    if ((vid != 0x56) || (pid != 0x42)) {
      Serial.println(F("ACK CMD Can't find OV5642 module!"));
      delay(1000);
    } else {
      Serial.println(F("ACK CMD OV5642 detected."));
      loop1 = false;
    }
}
#endif 
//Change to JPEG capture mode and initialize the OV5642 module
myCAM.set_format(JPEG); 
myCAM.InitCAM();
#if defined (OV2640_MINI_2MP) 
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
#else 
  myCAM.write_reg(ARDUCHIP_TIM, VSYNC_LEVEL_MASK); 
//VSYNC is active HIGH 
myCAM.OV5642_set_JPEG_size(OV5642_320x240);
#endif 
myCAM.clear_fifo_flag();
#if !(defined (OV2640_MINI_2MP))
  myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
#endif 
  myCAM.set_bit(ARDUCHIP_GPIO,GPIO_PWDN_MASK); 

}



void loop() {
  
  uint8_t temp = 0xff, temp_last = 0; uint8_t start_capture = 0; temp = Serial.read();
  myCAM.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  myCAM.OV5642_set_JPEG_size(OV5642_640x480);
  delay(1000);
  //Serial.println(F("ACK CMD switch to OV5642_640x480"));
  myCAM.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  start_capture = 1;
  Serial.println(F("ACK CMD CAM start single shot."));
  myCAM.clear_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
  delay(800);
  Serial.println("Start_capture=1");
  if (start_capture == 1)
  {
    Serial.println("capture 'if' statement");
    myCAM.flush_fifo();
    Serial.println("capture 'if' statement 2");
    myCAM.clear_fifo_flag();
    Serial.println("capture 'if' statement 3"); 
    //Start capture 
    myCAM.start_capture(); 
    Serial.println("capture has been started"); 
    delay(10000);
  }
  if (myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
  {
    myCAM.set_bit(ARDUCHIP_GPIO, GPIO_PWDN_MASK);
    temp = 0; 
    Serial.println(F("ACK IMG")); 
    while ( (temp != 0xD9) | (temp_last != 0xFF) ) 
    //D9 in hex = 217 in decimal. FF=255 
    //according to the ArduCam Support, the head of the image data is 0xFF 9xD8
    //and the end of the data is 0xFF 0xD9 
    {
      temp_last = temp; 
      temp = myCAM.read_fifo();
      Xbee.write(temp); 
      Serial.write(temp); 
      delayMicroseconds(10); 
    } 
     Serial.println(F("ACK CMD CAM Capture Done.")); 
     //Clear the capture done flag 
     myCAM.clear_fifo_flag(); 
     start_capture = 0;
     
  }
 }
