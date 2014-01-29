
#ifndef _WC_SERIALPORT_H
#define _WC_SERIALPORT_H



#include"serial_windows.h"

//******************************************************************************************************************
//wrapper class for serial port 
//******************************************************************************************************************
class WSerialport
{
private:
    #ifdef WIN32
        //serialport object instantiation
        Serialport_Windows s;   
    #elif linux 
        //serialport object instantiation
        Serialport_Linux s;   
    #endif


public:
    WSerialport();
    ~WSerialport();
    //open serial port function
    int OpenPort(char *PortName_p,int BaudRate=BAUDRATE(9600),int ReadMode_p=BLOCK,int StopBits_p=ONESTOPBIT,int ParityBits_p=NOPARITY,int DataBita_p=8);
    //configure serial port function
    int ConfigurePort(int BaudRate_p=BAUDRATE(9600),int ReadMode_p =BLOCK,int StopBits_p=ONESTOPBIT,int ParityBits_p=NOPARITY, int DataBits_p=8);
    //write serial port function
    int ReadPort(char *data,int bytes_to_read,unsigned long *byte_read = NULL);
    //write serial port function
    int WritePort(char *data,int bytes_to_write, unsigned long *byte_written = NULL);
    //close  port function
    int ClosePort();
    
};
//******************************************************************************************************************

#endif
