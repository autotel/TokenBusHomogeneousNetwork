#include "SendOnlySoftwareSerial.h"
#include "ReceiveOnlySoftwareSerial.h"

//only candidates for softwareSerial RX on a Mega:
//  10, 11, 12, 13,  50, 51, 52, 53,  62, 63, 64, 65, 66, 67, 68, 69
// 168 and 328 Arduinos
#if defined(__AVR_ATmega168__) ||defined(__AVR_ATmega168P__) ||defined(__AVR_ATmega328P__)
  #define TIP 3
  #define COMPIN 4
  #define TOP 5
  #define DEBUGPIN 10
  #define SERIALDEBUG false

// Mega 1280 & 2560
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define TIP 9
  #define COMPIN 10
  #define TOP 11
  #define DEBUGPIN 40
  #define SERIALDEBUG true
#endif

SendOnlySoftwareSerial    comTX = SendOnlySoftwareSerial(COMPIN);
ReceiveOnlySoftwareSerial comRX = ReceiveOnlySoftwareSerial(COMPIN);

unsigned char OwnID=0;

void setup(){

  #if SERIALDEBUG
    Serial.begin(9600);            // to see who's connected
    Serial.println("start");
  #endif
  //TIP and TOP are inverse logic
  pinMode(TIP,INPUT_PULLUP);
  pinMode(DEBUGPIN,OUTPUT);
  // outputMode();
  // comTX.write('a');
  // comTX.write('a');
  // comTX.write('a');
  // comTX.write('a');
  // inputMode();
}
char test=0;
void loop(){

  if( comRX.available() ){
    int i = int(comRX.read());
    #if SERIALDEBUG
      Serial.write(i);
    #else
      int i = int(comRX.read());
      if( i == OwnID ){
        outputMode();
        comTX.print("I am ");
        comTX.println(OwnID);
        inputMode();
        digitalWrite(DEBUGPIN,HIGH);
        delay(100);
        digitalWrite(DEBUGPIN,LOW);
      }
    #endif
  }

  #if SERIALDEBUG
    if( Serial.available()){
      int i = int(Serial.read())-48;
      outputMode();
      comTX.write(i);
      inputMode();
    }
  #endif
  // outputMode();
  // comTX.write(test++);
  // inputMode();
}

void outputMode(){
  comRX.end();
  pinMode(COMPIN,OUTPUT);
  comTX.begin(9600);
}
void inputMode(){
  comTX.end();
  pinMode(COMPIN,INPUT_PULLUP);
  comRX.begin(9600);
}



