
#ifndef TBN_h
#define TBN_h

class TBN
{
  public:
    TBN();
    void start();
    //set the data reception listener
    void onData();
    //read data, clearing it
    void loop();
    //set data available.data is cleared after send.}
    void write(unsigned char data[], unsigned char len);
    bool listen();
  private:
    int _pin;
    void outputMode();
    void inputMode();
};

#endif