#include"WCSerialport.h"

//******************************************************************************************************************
//constructor
//******************************************************************************************************************
WSerialport::WSerialport()
{
}
//******************************************************************************************************************
//Destructor
//******************************************************************************************************************
WSerialport::~WSerialport()
{
    //call to closeport 
    s.ClosePort();

}
//******************************************************************************************************************
//open function 
//return 0 on success
//return -1 on failure

//  1)  PortName_p      ----    carries the port name to be opened
//  2)  BaudRate        ----    carries the baudrate
//  3)  ReadMode_p      ----    carries the block/NON block mode
//  4)  StopBits_p      ----    carries the number of stop bits
//  5)  ParityBits_p    ----    carries the kind of parity bits(even ,odd)
//  6)  DataBita_p      ----    carries the number of data bits

//******************************************************************************************************************
int WSerialport::OpenPort(char *PortName_p,int BaudRate,int ReadMode_p,int StopBits_p,int ParityBits_p,int DataBita_p)
{   
    return(s.OpenPort(PortName_p,BaudRate,ReadMode_p,StopBits_p,ParityBits_p,DataBita_p));
}
//******************************************************************************************************************
//close port function
//return 0 on success
//return -1 on failure

//******************************************************************************************************************
int WSerialport::ClosePort()
{
    return(s.ClosePort());
}
//******************************************************************************************************************
//cofigure port function
//return 0 on success
//return -1 on failure
//  1)  BaudRate        ----    carries the baudrate
//  2)  ReadMode_p      ----    carries the block/NON block mode
//  3)  StopBits_p      ----    carries the number of stop bits
//  4)  ParityBits_p    ----    carries the kind of parity bits(even ,odd)
//  5)  DataBita_p      ----    carries the number of data bits
//******************************************************************************************************************
int WSerialport::ConfigurePort(int BaudRate,int ReadMode_p,int StopBits_p,int ParityBits_p,int DataBita_p)
{
    return(s.ConfigurePort(BaudRate,ReadMode_p,StopBits_p,ParityBits_p,DataBita_p));
}
//******************************************************************************************************************
//Readport function
//return 0 on success
//return -1 on failure

//  1)  data            ----    carry the message after reading
//  2)  byte_read       ----    carries the number of bytes read
//  3)  bytes_to_read   ----    carries the number of byte to be read

//******************************************************************************************************************
int WSerialport::ReadPort(char *data,int bytes_to_read,unsigned long *byte_read)
{
    unsigned long byte_read_temp[10];
    if (NULL == byte_read)
    {
        byte_read = byte_read_temp;
    }
    return(s.ReadPort(data,byte_read,bytes_to_read));
}
//******************************************************************************************************************
//write port function
//return 0 on success
//return -1 on failure

//  1)  data            ----    carries the message to be written
//  2)  byte_written    ----    carries the number of bytes writtem
//  3)  bytes_to_write  ----    carries the number of bytes to be write
//******************************************************************************************************************
int WSerialport::WritePort(char *data,int bytes_to_write, unsigned long *byte_written)
{
    unsigned long byte_written_temp[10];
    if (NULL == byte_written)
    {   
        byte_written = byte_written_temp;
    }
    return(s.WritePort(data,byte_written,bytes_to_write));
}


