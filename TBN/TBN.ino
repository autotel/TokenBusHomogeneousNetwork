#include "SendOnlySoftwareSerial.h"
#include "ReceiveOnlySoftwareSerial.h"

#define tokenTimeout 10

long lastMessage=0;

//states
#define S_CONNECTING 0
#define S_LISTENING 1
#define S_BROADCASTING 2
unsigned char s_current=S_CONNECTING;
//message header first nibbles
#define H_OFFLINE 0x00
#define H_EMPTY 0x01
#define H_BROADCASTED 0x02
#define H_ADDRESSED 0x03
//message header second nibbles
#define L_STRING 0X00 //if message has length zero is undefined length message or string

//only candidates for softwareSerial RX on a Mega:
//  10, 11, 12, 13,  50, 51, 52, 53,  62, 63, 64, 65, 66, 67, 68, 69
// 168 and 328 Arduinos
#if defined(__AVR_ATmega168__) ||defined(__AVR_ATmega168P__) ||defined(__AVR_ATmega328P__)
  #define TIP 3
  #define COMPIN 4
  #define TOP 5
  #define DEBUGPIN 10
  #define LISTENSTATEDEBUGPIN 11
  #define SERIALDEBUG true

// Mega 1280 & 2560
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #define TIP 9
  #define COMPIN 10
  #define TOP 11
  #define DEBUGPIN 40
  #define LISTENSTATEDEBUGPIN 41
  #define SERIALDEBUG true
#endif

SendOnlySoftwareSerial    comTX = SendOnlySoftwareSerial(COMPIN);
ReceiveOnlySoftwareSerial comRX = ReceiveOnlySoftwareSerial(COMPIN);

unsigned char ownID=0;

unsigned char tentativeID=0;
long initWaitStarted=0;

void setup(){


  //TIP LOW=hesitate
  //TIP HIGH=transmit/get address
  pinMode(TIP,INPUT_PULLUP);
  pinMode(TOP,OUTPUT);
  //set the TOP low, telling the next node there is a lower node at the left.
  digitalWrite(TOP,LOW);
  pinMode(DEBUGPIN,OUTPUT);
  pinMode(LISTENSTATEDEBUGPIN,OUTPUT);
  // outputMode();
  // comTX.write('a');
  // comTX.write('a');
  // comTX.write('a');
  // comTX.write('a');
  // inputMode();
  #if SERIALDEBUG
    Serial.begin(9600);            // to see who's connected
    Serial.println("start...");

    pinMode(38,OUTPUT);
    pinMode(39,OUTPUT);
    digitalWrite(38,LOW);
    digitalWrite(39,LOW);
  #endif

  //the initial state requires to be listening to other's id until it is my turn.
  inputMode();

  //some time to make sure that all the other peers initialize their TOPS to low.
  //if one arduino starts up faster than other in the left of the chain, it may
  //report that there is no other arduino on the left and mistakenly get the 0 address
  delay(1000);
}
char test=0;
unsigned char testByte=0;
void loop(){
  // long now=millis();
  switch (s_current) {
    case S_CONNECTING:{
      if(digitalRead(TIP)){
        //the node could have been connected right after a row ended. (happens easily if the network has two nodes only)
        //IN that case it would have read no address, and would have received a token, thus
        //will get the address 0. To avoid this, upon token reception, the node must wait one lap more as
        //to make sure that he listened to all the headers in the network.
        if(initWaitStarted==0){
          initWaitStarted=millis();
          digitalWrite(TOP,HIGH);
        }else if(millis()>initWaitStarted+tokenTimeout){
          //the node detected the TOP high already and after waiting more than one timeout time, it is still/again on.
          ownID=tentativeID;
          outputMode();
          comTX.write(ownID);
          inputMode();
          digitalWrite(TOP,HIGH);
          lastMessage=millis();
          s_current=S_LISTENING;
          #if SERIALDEBUG
            Serial.print("\ndefinitiveID: ");
            Serial.print(String(tentativeID,DEC));
          #endif
        }
      }else{
        if( comRX.available() ){
          unsigned char vid=((unsigned char)(comRX.read())) + 1;
          if(vid>tentativeID)
            tentativeID = vid;
          #if SERIALDEBUG
            Serial.print("\ntentativeID: ");
            Serial.print(String(tentativeID,DEC));
          #endif
        }
      }
      break;
    }
    case S_LISTENING:{
      digitalWrite(LISTENSTATEDEBUGPIN,HIGH);
      if(millis()>lastMessage+(2*tokenTimeout)){
        digitalWrite(TOP,LOW);
        if(ownID==0){
          s_current=S_BROADCASTING;
        }
      }
      if(digitalRead(TIP)){
        if(ownID!=0){
          s_current=S_BROADCASTING;
        }
      }
      if( comRX.available() ){
        lastMessage=millis();
        #if SERIALDEBUG
          Serial.print("\n rx:");
        #endif
        while(comRX.available()){
          digitalWrite(TOP,LOW);
          int i = (unsigned char)(comRX.read());
          #if SERIALDEBUG
            Serial.print(String(i,DEC));
            Serial.write(' ');
          #else
            if( i == ownID ){
              s_current=S_BROADCASTING;
              digitalWrite(DEBUGPIN,HIGH);
            }
          #endif
        }
      }
      #if SERIALDEBUG
        if( Serial.available()){
          while(Serial.available()){
            testByte=((unsigned char)(Serial.read())-48);
          }
          s_current=S_BROADCASTING;
        }
      #endif

      break;
    }
    case S_BROADCASTING:{
      digitalWrite(LISTENSTATEDEBUGPIN,LOW);
      outputMode();
      comTX.write(ownID);
      comTX.write(H_BROADCASTED);
      comTX.write(testByte);
      inputMode();
      s_current=S_LISTENING;
      // delay(100);
      #if SERIALDEBUG
        Serial.print("\nTX: ");
      #endif

      digitalWrite(DEBUGPIN,LOW);
      digitalWrite(TOP,HIGH);
      lastMessage=millis();
      break;
    }
  }
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


