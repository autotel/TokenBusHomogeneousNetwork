#include "TBN.h"
TBN network;

char test=0;
unsigned char testByte=0;

void setup(){
  network.start();
  network.onData(onMessage);
}
void loop(){
  unsigned char testSignal [] = {testByte,testByte+1,testByte+2,testByte+3,testByte+4,testByte+5};
  
  network.write(testSignal,6);//test message. it doesn't write,  but makes it available for next token reception
  network.loop();//there should be an ISR timer rather than monitorization
}
void onMessage(unsigned char origin,unsigned char header,unsigned char * data, unsigned char len){
  testByte=(data[0]+1);
}