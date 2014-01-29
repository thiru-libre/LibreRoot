
#ifdef WIN32
#include "serial_windows.h"

//****************************************************************************************************
//Baudrate
//return the corresponding macro for the given baudrate

//baudrate-carries the baudrate to be converted
//****************************************************************************************************

int Baudrate(char *baudrate)
{
    
    long y=atoi(baudrate);
    switch(y)
    {
        case 300:return BAUDRATE(300);
        case 600:return BAUDRATE(600);
        case 1200:return BAUDRATE(1200);
        case 2400:return BAUDRATE(2400);
        case 4800:return BAUDRATE(4800);
        case 9600:return BAUDRATE(9600);
        case 19200:return BAUDRATE(19200);
        case 38400:return BAUDRATE(38400);
        case 115200:return BAUDRATE(115200);
        default:return BAUDRATE(9600);
    }
}

//****************************************************************************************************
//  Constructor 
//*****************************************************************************************************
Serialport_Windows::Serialport_Windows()
{

}
//****************************************************************************************************
//Destructor
//****************************************************************************************************

Serialport_Windows::~Serialport_Windows()
{
    ClosePort();
}
//****************************************************************************************************
//OpenPort -function to open serial port  
//  return 0 on successfully open the serial port with given configuration 
//  return -1 on failure to open the device
//  return -2 on failure of the DCB configuration
//  return -3 on failure of the BLOCK/NONBLOCK configuration
//  return -4 on failure to get the current DCB settings

//  1)  PortName_p      ----    carries the port name to be opened
//  2)  BaudRate_p      ----    carries the baudrate
//  3)  ReadMode_p      ----    carries the block/NON block mode
//  4)  StopBits_p      ----    carries the number of stop bits
//  5)  ParityBits_p    ----    carries the kind of parity bits(even ,odd)
//  6)  DataBita_p      ----    carries the number of data bits
//  Serial port is opened in the NON-overlapped mode 
//****************************************************************************************************
int Serialport_Windows::OpenPort (char *PortName_p,int BaudRate_p=BAUDRATE(9600),int ReadMode_p=NONBLOCK,int StopBits_p=ONESTOPBIT,int ParityBits_p=NOPARITY,int DataBits_p=8)
{   

    char buffer[200];
    sprintf(buffer,"Serialport param :  Port : %s  BaudRate: %d ReadMode : %d StopBits : %d ParityBits : %d  DataBits :%d",PortName_p,BaudRate_p,ReadMode_p,StopBits_p,ParityBits_p, DataBits_p);
    //print(buffer);

    //Baudrate
    //  CBR_110     CBR_19200   CBR_300     CBR_38400   CBR_600     CBR_56000
    //  CBR_1200    CBR_57600   CBR_2400    CBR_115200  CBR_4800    CBR_128000
    //  CBR_9600    CBR_256000  CBR_14400  

    BaudRate=BaudRate_p;    
    //Read mode -can be BLOCKED or NONBLOCKED
    ReadMode=ReadMode_p;    
    //stop bits // ONESTOPBIT 1 stop bit ;ONE5STOPBITS 1.5 stop bits ;TWOSTOPBITS 2 stop bits 
    StopBits=StopBits_p;    
    //parity bits   //  EVENPARITY Even   //    MARKPARITY Mark //  NOPARITY No parity                                  
    // ODDPARITY Odd  //    SPACEPARITY  space
    ParityBits =ParityBits_p;
    // 5,6,7,8 bits                                             
    DataBits=DataBits_p;     

    //open the serial  port
    hSerialPort=CreateFile(   
            
            //port name 
            PortName_p,     
            //Access in Both read and write mode
            GENERIC_READ|GENERIC_WRITE,         
            //sharing is disabled
            0,                  
            //handle cannot be inherited for derived class
            NULL,       
            //communication device must use open_existing
            OPEN_EXISTING,
            // not overlapped I/O 
            0,      
            //hTemplate must be NULL for communication device
            NULL
    );      

    //check for the failure for serialport
    if(hSerialPort==INVALID_HANDLE_VALUE)       
    {
        //print("Serial Port  :Open Failed");
        //on failure
        return -1;
    }
    else
    {   
        //on successful opening,configure the port with initialized parameter.
        return ConfigurePort(BaudRate,ReadMode,StopBits,ParityBits,DataBits);
    }
}

//****************************************************************************************************
//ClosePort-function to close the serial port
//return 0 success
//return -1 on failure
//****************************************************************************************************
int Serialport_Windows::ClosePort()
{
    if(!CloseHandle(hSerialPort))
    {
        //print("Port status  :Close Failed");
        //on failure
        return -1;
    }
    else
    {
        //print("Port status  :Closed");
        //on success
        return 0;
    }

}
//****************************************************************************************************
//  ConfigurePort -function to configure the serial port
//  return  0 on successful DCB configuration
//  return -2 on failure of the DCB configuration
//  return -3 on failure of the BLOCK/NONBLOCK configuration
//  return -4 on failure to get the current DCB settings
//  return -5 on failure to get the buffer size requested

//  1)  BaudRate_p      ----    carries the baudrate
//  2)  ReadMode_p      ----    carries the block/NON block mode
//  3)  StopBits_p      ----    carries the number of stop bits
//  4)  ParityBits_p    ----    carries the kind of parity bits(even ,odd)
//  5)  DataBita_p      ----    carries the number of data bits

//with default parameters for    BaudRate =CBR_9600
//                               ReadMode=BLOCK
//                               StopBits=ONESTOPBIT 
//                               ParityBits=NOPARITY
//                               DataBits=8

//****************************************************************************************************

int Serialport_Windows::ConfigurePort(int BaudRate_p=BAUDRATE(9600),int ReadMode_p =NONBLOCK,int StopBits_p=ONESTOPBIT,int ParityBits_p=NOPARITY, int DataBits_p=8)
{
    //DCB datastructure to store the serial port setting
    DCB dcb;
    //clear the dcb structure
    FillMemory(&dcb,sizeof(dcb),0);
    //set the size
    dcb.DCBlength=sizeof(dcb);
    //create an object
    COMMTIMEOUTS cmt;  
    //updating the previous configuration
    ReadMode=ReadMode_p; 
    //get the dcb settings
    if (!GetCommState(hSerialPort, &dcb)) {
            //on failure
            return -4;
    }
    //set the Baudrate
    dcb.BaudRate=BaudRate_p;
    //set the databits
    dcb.ByteSize=DataBits_p;
    //set the parity type
    dcb.Parity=ParityBits_p;
    //set the stops bits
    dcb.StopBits=StopBits_p;

    
    //set the dcb structure with updated settings   
    if(!SetCommState(hSerialPort, &dcb)){   
        
        //print("Port setting :Failed");
        //on failure
        return -2;
    }

    
    if(ReadMode==NONBLOCK)
    {       
        //NONBLOCK mode mode configuration ,wait for (100ms * no of bytes to read)+100ms

        //read timeout in ms
        cmt.ReadIntervalTimeout=TIMEOUT; 
        //time required to read all the bytes in ms
        cmt.ReadTotalTimeoutMultiplier=TIMEOUT;
        //time out for single read operation in ms
        cmt.ReadTotalTimeoutConstant=TIMEOUT;  
        //time out for single write operation in ms 
        cmt.WriteTotalTimeoutConstant=100;  
        //time out to write all the bytes in ms
        cmt.WriteTotalTimeoutMultiplier=10;
        //print("Read Mode    :NONBLOCK");
    }
    else if(ReadMode==BLOCK)
    {
        //BLOCK mode ,All the parameters are set to 0,to disable the timeouts

        //read timeout in ms
        cmt.ReadIntervalTimeout=0; 
        //time required to read all the bytes in ms
        cmt.ReadTotalTimeoutMultiplier= 100;
        //time out for single read operation in ms
        cmt.ReadTotalTimeoutConstant=0;  
        //time out for single write operation in ms 
        cmt.WriteTotalTimeoutConstant=0;
        //time out to write all the bytes in ms
        cmt.WriteTotalTimeoutMultiplier=0; 
        //print("Read Mode    :BLOCK");
    }
    //timeout configuration
    if(!SetCommTimeouts(hSerialPort,&cmt)){    
        
        //print("Read Mode set:Failed");
        //on failure
        return -3;              
    }
    else
    {
        //print("Read Mode set:OK");
        //on success
        return 0;
    }
}
//****************************************************************************************************
//  ReadPort-Function to read the serial port either in BLOCK mode or NONBLOBK mode based on
//  the initial configuration parameter passed while initializing Serialport1 objects
//  return -1 on failure to read operation
//  return 0 on successful read operation

//  1)  data            ----    Message read from the port
//  2)  byte_read       ----    Number of byte read
//  3)  bytes_to_read   ----    Number of bytes to read

//****************************************************************************************************

int Serialport_Windows::ReadPort(char *data,DWORD *byte_read,int bytes_to_read)
{
    if(!ReadFile(hSerialPort,data,bytes_to_read,byte_read,NULL))
    {
        //on failure of read operation
        return -1;
    }
    else
    {
        //on successful read operation
        return 0;
    }
}
//****************************************************************************************************
//  WritePort-Function to write the serial port either in BLOCK mode or NONBLOBK mode based on
//  the initial configuration parameter passed while initializing Serialport1 objects
//  return -1 on failure to read operation
//  return 0 on successful read operation

//  1)  data            ----    Message write the port
//  2)  byte_write      ----    Number of byte written
//  3)  bytes_to_write  ----    Number of bytes to write

//****************************************************************************************************

int Serialport_Windows::WritePort(char *data,unsigned long *byte_write, int byte_to_write)
{
    if(!WriteFile(hSerialPort,data,byte_to_write,byte_write,NULL))
    {
        //on failure ofwrite operation
        return -1;
    }
    else
    {
        //on successful write operation
        return 0;
    }
}

#endif


