#include <SoftwareSerial.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

/*
  Author: Chris Munley
  I wrote this code in an attempt to send pictures over Xbee Series 2 RF modules in tandem with arduino. The code should work, as it was put together
  by taking pieces of code from the Arducam library (found on github) along with a few other resouces online. 
  I ran into a few errors writing this code. first, sometimes I would get the SPI Interface error, and I am not sure why. If you get this error
  I suggest either resetting the arducam by unplugging and plugging everything back in and re uploading the sketch to the arduino, also make sure that
  CS pin is set to 7 on the arduino. 
  Also, when sending the bitstream over the rf modules sometimes every bit would become 00 for me, but I think that was due to something like
  the interface error because I struggled to get past all of the confirmation steps that the camera is working in this code.
  The bitstream has to be reconstructed on the PC end after it is compressed and sent, which can probably be done with MATLAB but I am not 
  familiar with it and did not get to try that. 
  https://www.imperialviolet.org/binary/jpeg/   this page and the jpeg wiki should help with rebuilding the bitstream
  you can test the camera to see if it can take pictures at all with the software in the arducam library
  found at: arducam>examples>host_app>ArduCAM_Host_V2.0_Windows

  I also had some troubles with the header files in this code, sometimes they were not found and I am not sure why. Possibly had to do with how I 
  downloaded the library but that I never figured out. I also could not get any of the example codes to work with this setup, which is weird
  because they should have worked. My assumption is that if you can get the example codes in the arducam library to work, you should be able to 
  send the bitsream across the xbees with no troubles, as that is how the code is set up

  In my attempts to make this code work, I removed a few features that were included in the example code such as options to change the resolution of the
  picture. Also, I think this code now only works with the OV5642, so if you use a different camera you will have to go back to the example codes and
  figure out what you need to use for a different camera. I tried to remove as much extra code as I could without damaging the integrity of the program,
  so if you go back to the example code if you are really struggling you can see all of the steps you are supposed to take to get the camera to work because
  it is possible I left something important out. Also, some parts of the code might be extraneous as I did not want to mess up how it worked, so the code could
  definitely use some reworking if you are really dedicated.

  You have to configure the xbees before using them, which I did so they should be good to go, but make sure that all of the baud rates match up so that
  they can understand eachother. 

  To take this code further I would add functionality to save pictures to the SD card since it does not have that yet, and also I would look into power
  saving options since it is supposed to run off solar. Also, I would work on a way to quickly change the rate of pictures, and some type of input to tell
  the arduino if you want to recieve the pictures now or save them to sd card and send them over once you take a certain amount.

  Another advantageous function of the camera would be to add functionality to wire directly to the camera so that you can get 
  live video without waiting on the xbee to catch up. This would be beneficial so that when setting up the camera you can tell exactly 
  what it is looking at. 
  
  If you want to talk to me about the code email me at chrismunley1@gmail.com I will be more than happy to help, I would love to see this project working
  after I struggled for hours on hours troubleshooting errors. 
*/

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
  
  uint8_t temp = 0xff, temp_last = 0; 
  uint8_t start_capture = 0; 
  temp = Serial.read();
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
    // loop for actually sending the bitstream across the xbee
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
