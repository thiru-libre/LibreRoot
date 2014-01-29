#ifndef _SERIAL_WINDOWS_H
#define _SERIAL_WINDOWS_H


#ifdef WIN32

    #define BAUDRATE(X) CBR_##X
    //1 second 
    #define TIMEOUT 1  

    #include<iostream>
    #include<windows.h>
    using namespace std;

    //for block reading -infinitely wait for device to return data
    #define BLOCK 1         
    //for non block reading -wait for device to return data for given time out in millisecond
    #define NONBLOCK 2          

//******************************************************************************************************************
//class for Serialport
//******************************************************************************************************************
class Serialport_Windows
{
private:
    //Baudrate
    int BaudRate;               
    //read mode
    int ReadMode;               
    //no of bytes read
    int DataBits;   
    //parity bit
    int ParityBits; 
    //stop bit
    int StopBits;   
    //serial port handle for windows,Seperate datastucture is followed
    HANDLE hSerialPort;         

public:

    Serialport_Windows();
    ~Serialport_Windows();
    //open serial port function
    int OpenPort (char * PortName_p,int BaudRate_p,int ReadMode_p,int StopBits_p,int ParityBits_p, int DataBits_p);
    //configure serial port function
    int ConfigurePort(int BaudRate_p,int ReadMode_p, int StopBits_p,int ParityBits_p,int DataBits_p);
    //Read serial port function
    int ReadPort(char *data,unsigned long *byte_read,int bytes_to_read);
    //write serial port function
    int WritePort(char *data,unsigned long *byte_write, int byte_to_write);
    //close serial port function
    int ClosePort();
    
};

//function to convert the baudrate to correspondig OS value
int Baudrate(char *baudrate);

//******************************************************************************************************************
#endif

#endif