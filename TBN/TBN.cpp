#include "Arduino.h"
#include "SendOnlySoftwareSerial.h"
#include "ReceiveOnlySoftwareSerial.h"
#include "TBN.h"
#define tokenTimeout 30
#define BAUDRATE 9600


long lastMessageAt=0;

//states
#define S_CONNECTING 0
#define S_LISTENING 1
#define S_BROADCASTING 2
unsigned char s_current=S_CONNECTING;
//message header first nibbles
#define H_OFFLINE 0x00
#define H_EMPTY 0x10
#define H_BROADCASTED 0x20
#define H_ADDRESSED 0x30
//message header second nibbles
#define L_STRING 0X00 //if message has length zero is undefined length message or string
//byte number roles <origin><header><payload>
#define BN_ORIGIN 0
#define BN_HEADER 1
#define BN_PAYLOAD0 2
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
  #define USEHS false

// Mega 1280 & 2560
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

/*
TODO
Maybe I should use hardwareserial for the input. TO terminate the hardwareserial and
let the pin free for softwareserial, the library needs an edit stated in
http://forum.arduino.cc/index.php?topic=46426.0
*/


  #define TIP 9
  #define COMPIN 15
  #define TOP 11
  #define DEBUGPIN 40
  #define LISTENSTATEDEBUGPIN 41
  #define SERIALDEBUG true
  //use hardware serial instead of ss if the pin is any RX pin
  #define USEHS true
#endif


SendOnlySoftwareSerial    comTX = SendOnlySoftwareSerial(COMPIN);
#if USEHS
  // ReceiveSerial comRX=Serial3();
#else
  ReceiveOnlySoftwareSerial comRX = ReceiveOnlySoftwareSerial(COMPIN);
#endif
unsigned char ownID=0;

unsigned char tentativeID=0;
long initWaitStarted=0;
bool TIPhasBeenLow=false;

#define MAXMSGLEN 8
//last received message
unsigned char incom[MAXMSGLEN];

//message to send next
int outgo[MAXMSGLEN];

//how long to wait until the message is considered to be truncated (only part of the message arrived)
#define truncateTimeout 10
TBN::TBN(){

}
void TBN::start(){
  for(unsigned char a=0; a<MAXMSGLEN; a++){
    incom[a]=0;
  }

  //TIP LOW=hesitate
  //TIP HIGH=transmit/get address
  pinMode(TIP,INPUT_PULLUP);
  pinMode(TOP,OUTPUT);
  //set the TOP low, telling the next node there is a lower node at the left.
  digitalWrite(TOP,LOW);
  pinMode(DEBUGPIN,OUTPUT);
  pinMode(LISTENSTATEDEBUGPIN,OUTPUT);
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

void TBN::loop(){
  // long now=millis();
  switch (s_current) {
    case S_CONNECTING:{
      digitalWrite(DEBUGPIN,HIGH);
      if(digitalRead(TIP)){
        //the node could have been connected right after a row ended. (happens easily if the network has two nodes only)
        //IN that case it would have read no address, and would have received a token, thus
        //will get the address 0. To avoid this, upon token reception, the node must wait one lap more as
        //to make sure that he listened to all the headers in the network.
        if(initWaitStarted==0){
          initWaitStarted=millis();
          digitalWrite(TOP,HIGH);
        }else if(millis()>initWaitStarted+tokenTimeout){
          //to set the id, either the TOP has to never been low, or tentativeID is >0
          if((TIPhasBeenLow&&tentativeID>0)||((!TIPhasBeenLow)&&tentativeID==0)){
            ownID=tentativeID;
            outputMode();
            comTX.write(ownID);
            inputMode();
            digitalWrite(TOP,HIGH);
            lastMessageAt=millis();
            s_current=S_LISTENING;
            digitalWrite(DEBUGPIN,LOW);
            #if SERIALDEBUG
              Serial.print("\ndefinitiveID: ");
              Serial.print(String(tentativeID,DEC));
            #endif
          }
          //maybe this is not necessary:
          if(TIPhasBeenLow&&tentativeID==0){
            initWaitStarted=0;
          }
        }
      }else{
        TIPhasBeenLow=true;
        if(listen()){
          unsigned char vid=((unsigned char)(incom[BN_ORIGIN])) + 1;
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
      if(millis()>lastMessageAt+(2*tokenTimeout)){
        digitalWrite(TOP,LOW);
        if(ownID==0){
          s_current=S_BROADCASTING;
          digitalWrite(DEBUGPIN,LOW);
        }
      }
      if(digitalRead(TIP)){
        if(ownID!=0){
          s_current=S_BROADCASTING;
        }
      }
      if(listen()){
        _onReceiveCallback(incom[BN_ORIGIN],incom[BN_HEADER],& incom[BN_PAYLOAD0],incom[BN_HEADER]&0xF);
        // _midiInCallback(message);
      }


      break;
    }
    case S_BROADCASTING:{
      digitalWrite(LISTENSTATEDEBUGPIN,LOW);
      outputMode();
      unsigned char buf[MAXMSGLEN];
      buf[0]=ownID;
      if(outGoBytesDue==0){
        buf[1]=H_EMPTY;
      }else{
        buf[1]=H_BROADCASTED;
        buf[1]|=outGoBytesDue&0xf;
        for(unsigned char a=0; a<outGoBytesDue; a++){
          buf[a+2]=outgo[a];
        }
        outGoBytesDue=0;
      }
      comTX.write(buf, sizeof(buf));
      inputMode();
      s_current=S_LISTENING;
      // delay(100);
      #if SERIALDEBUG
        Serial.print("\nTX.");
      #endif


      digitalWrite(TOP,HIGH);
      lastMessageAt=millis();
      break;
    }
  }
}
void TBN::onData(void (*callback)(unsigned char,unsigned char,unsigned char *,unsigned char)){
  _onReceiveCallback = callback;
  // _midiInCallback = midiInCallback;
}
//TODO:instead of replacing the array of data, it should be appended in a circular array,
//the device probably wants to send more than one simultaneous signal, and the protocol permits
//n size messages.
void TBN::write(unsigned char data [],unsigned char len){
  outGoBytesDue=len;
  for(unsigned char a=0; a<MAXMSGLEN; a++){
    outgo[a]=0;
  }
  for(unsigned char a=0; a<len; a++){
    outgo[a]=data[a];
  }
}
bool TBN::listen(){
  #if USEHS
    if( Serial3.available() ){
  #else
    if( comRX.available() ){
  #endif
    digitalWrite(TOP,LOW);
    //truncated message timeout mechanism
    unsigned char incomCount=0;
    #if SERIALDEBUG
      Serial.print("\trx:\t");
    #endif
    long lastByteStarted=millis();
    while(incomCount<MAXMSGLEN){
      //TODO:
      //if incomCount==1; MAXMSGLEN= the len part of the message

    #if USEHS
      if( Serial3.available() ){
    #else
      if(comRX.available()){
    #endif
        lastByteStarted=millis();
      #if USEHS
        int i = (unsigned char)(Serial3.read());
      #else
        int i = (unsigned char)(comRX.read());
      #endif
        incom[incomCount]=i;
        #if SERIALDEBUG
          Serial.print(String(i,DEC));
          Serial.write('\t');
        #endif
        incomCount++;
      }
      if(millis()>lastByteStarted+truncateTimeout){
        #if SERIALDEBUG
          Serial.print("truncated");
        #endif
        return false;
      }
    }
    //actually only the node zero needs to car about lat message time
    lastMessageAt=millis();

    return true;
  }
  return false;
}

void TBN::outputMode(){
  #if USEHS
    Serial3.flush();
    Serial3.end();
  #else
    comRX.end();
  #endif
  pinMode(COMPIN,OUTPUT);
  comTX.begin(BAUDRATE);
}
void TBN::inputMode(){

  comTX.end();
  #if USEHS
    Serial3.begin(BAUDRATE);
    Serial3.flush();
  #else
    pinMode(COMPIN,INPUT_PULLUP);
    comRX.begin(BAUDRATE);
  #endif
}


