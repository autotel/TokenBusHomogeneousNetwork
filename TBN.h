
#ifndef TBN_h
#define TBN_h

class TBN
{
  public:
    TBN();
    void start();
    //set the data reception listener
    // void onData( std::function<void()> callback );
    void onData(void (*callback)(unsigned char,unsigned char,unsigned char *,unsigned char));
    //read data, clearing it
    void loop();
    //set data available.data is cleared after send.}
    void write(unsigned char data[], unsigned char len);
    bool listen();

  private:
    int _pin;
    unsigned char outGoBytesDue=0;
    void outputMode();
    void inputMode();

    void (*_onReceiveCallback)(unsigned char,unsigned char,unsigned char *,unsigned char);
};

#endif