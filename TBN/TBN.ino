#include "TBN.h"
TBN network;
void setup(){
  network.start();
}
unsigned char testSignal [] = {0x90,0x89,0x88,0x87,0x86,0x85,0x84,0x83};
void loop(){
  network.write(testSignal,8);//test message. it doesn't write,  but makes it available for next token reception
  network.loop();//there should be an ISR timer rather than monitorization
}
void onMessage(unsigned char origin,unsigned char header,unsigned char data){

}