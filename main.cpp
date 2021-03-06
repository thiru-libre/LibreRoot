#include <iostream>
using namespace std;

#include "main.h"
#include <string.h>
#include "WCSerialport.h"
#include "lib_typedefs.h"
#include "conio.h"
#include "iqapi.h"
#include "iqapidefines.h"
#include "iqapihndlbase.h"
#include "iqapihndlmatlab.h"
#include "windows.h"
#include "WCSerialport.h"
#include "stopwatch_timer.h"

#define IP "192.168.100.254"

#define RF_LEFT_PORT 2
#define CHANNEL11 2462e6
#define CHANNEL1 2412e6
#define CHANNEL6 2437E6
#define SAMPLING_FREQ_HZ 80e6
#define SAMPLING_TIME 1e-3
#define SIGNAl_OFDM 1
#define SIGNAL_11B 2
#define SIGNAl_11N 3
#define GMODE 1
#define BMODE 2
#define NMODE 3
#define MODULE_CR 1
#define MODULE_CX 2
#define CAPTURE_FAILURE 0
#define CAPTURE_SUCCESS 1
#define FAIL 0
#define TX_FAIL -1
#define TX_SUCCESS 0
WSerialport Port_obj;

char COMPortName[7];
char Module[4];
char MacId[24];
int channel;
int antenna;
char MAC[15];
iqapiHndl	*hndl = NULL;
iqapiResultOFDM	*resultOfdm = NULL;
iqapiAnalysisOFDM *analysisOfdm=NULL;
iqapiAnalysis11b *analysis11b=NULL;
iqapiResult11b	*result11b = NULL;
double rfFreq;
float ini_Gpowermin=14.0;
float ini_Gpowermax=16.0;
float ini_Bpowermin=15.5;
float ini_Bpowermax=17.5;
float ini_Gevm=-25;
float ini_Bevm=-22;

// Baud rate
#define BAUD_RATE 115200 

int ModuleType=0;
int mode;
FILE *pfile = NULL;
FILE *Pwrlogfile=NULL;
char filepath[40];
volatile bool ReadPort=false;
bool BTuningRequired=false;
bool GTuningRequired=false;
bool initialMeasurement=true;
unsigned int BufferSize=32;
volatile unsigned int dataParseSize=500;
char RxBuff_as8[50]; //To be used as ReadPort Buffer
char temp[1500];
unsigned long ReadCnt = 0;	//Read Count of buffer data
struct IQAPIMeasurementResults
{
	float evm;
	float power;
	float freqerr;
	float pwrArr[6];
	float evmArr[6];
	float freqArr[6];
	char* str[6];
	
}Gmode_testResult,Bmode_testResult;


DWORD readComPort()
{	
	char filepath1[40];
	int j=0,i=0;
	while(MacId[i]!='\0')
	{
		if(MacId[i]!=':')
		{
			MAC[j]=MacId[i];
			//printf("%c",fileLog[j]);
			j++;
		}
		i++;
	}
	MAC[j]='\0';
	sprintf(filepath1,".//COMLog%s.txt",MAC);
	pfile = fopen(filepath1, "a+");
	
	if (pfile == NULL)
	{
		printf("Error reopening the file\n");
	}
	while(1)	
	{	
		unsigned int i,j=0;
		Port_obj.ReadPort(RxBuff_as8,BufferSize,&ReadCnt);
		if(ReadPort)
		{
			temp[0]='\0';
			i=0;
			j=0;
			
			while(i<dataParseSize)
			{
				Port_obj.ReadPort(RxBuff_as8,BufferSize,&ReadCnt);
				while(RxBuff_as8[j]!='\0')
				{
					temp[i]=RxBuff_as8[j];
					printf("%c",RxBuff_as8[j]);
					fprintf(pfile,&RxBuff_as8[j]);
					i++;
					j++;
				}
				j=0;
				RxBuff_as8[0]='\0';
				ReadCnt=0;
			}			
		}

		for (i =0; i < ReadCnt; i++)
		{
			printf("%c",RxBuff_as8[i]);
			fprintf(pfile,&RxBuff_as8[i]);
		}
		RxBuff_as8[0] = '\0';
		ReadCnt = 0;			
	}
	fclose(pfile);
	return 0;
}

void WriteCOMPort(char * WriteCOMPort)
{

	unsigned int i=0;
	while(WriteCOMPort[i]!='\0')
	{
		Port_obj.WritePort(&WriteCOMPort[i],1);
		i++;
	}
	Port_obj.WritePort("\r",1);
}

int InitializeDUT(char * PortName_u8)
{
	int OpenPortNum=Port_obj.OpenPort(PortName_u8, BAUD_RATE, NONBLOCK);
	if (0 !=OpenPortNum)
	{
		printf("\tOpening port is failed with return code %d \r\n",OpenPortNum);		
	}	
	else
		printf("opening port success \r\n");
	return OpenPortNum;
}
int DisconnectDUT()
{
	Port_obj.ClosePort();
	return 0;
}


int InitializeAndConnectIQView()
{
	int errNo=iqapiInit();
	//printf("ErrNO Value in initialising iqview %d\r\n",errNo);

	if(errNo!=IQAPI_ERR_OK)
	{
		
		return errNo;
	}
	else
	{
		//printf("IQ View Libraries Initalized Successfully \r\n");
		if(hndl)
		{
			delete hndl;
			hndl=NULL;
		}
		hndl = new iqapiHndl();
		
		//Establish Connection to IQView over the IP 192.168.100.254
		errNo=hndl->ConInit(IP);
	}
	return errNo;
}

int DiconnectIQVIEW()
{
	hndl->ConClose();
	delete hndl;
	hndl=NULL;
	iqapiTerm();
	return 0;
}
int RX_TransmitSignal(double rfpwr,int sigtype)
{
	int err=0;
	hndl->tx->vsg[0]->rfGainDb =rfpwr;
	char *modFilepath = ".\\mod\\WiFi_OFDM-54.mod";
	err=hndl->SetWave(modFilepath);
	if(err!=0)
	{
		printf("Set path to transmit signal failed \r\n");
		return -1;
	}
	else
	{
		// Mark as single MOD file mod...
		//g_vsgMode = ::VSG_SINGLE_MOD_FILE;
	}
	hndl->tx->vsg[0]->enabled = IQV_RF_ENABLED;
	hndl->SetTxRx();
	int frameCnt = 1000;
	hndl->FrameTx(frameCnt);

	return 0;
}

int TX_CaptureSignals()
{
		int errno;
		hndl->Agc(TRUE);
		errno= hndl->Capture();
		if(mode==GMODE)
		{
			analysisOfdm = new iqapiAnalysisOFDM();
			analysisOfdm->ph_corr_mode=IQV_PH_CORR_SYM_BY_SYM;
			analysisOfdm->ch_estimate = IQV_CH_EST_RAW;
			analysisOfdm->sym_tim_corr = IQV_SYM_TIM_ON;
			analysisOfdm->freq_sync = IQV_FREQ_SYNC_LONG_TRAIN;
			analysisOfdm->ampl_track = IQV_AMPL_TRACK_OFF;
			hndl->analysis = dynamic_cast<iqapiAnalysis *>(analysisOfdm);
			errno=hndl->Analyze();
			//if(errno==0)
			//{
			//	printf("Analyse Success \r\n");
			//	printf("HNDL last error :%s \r\n",hndl->lastErr);
			//}
			if (dynamic_cast<iqapiResultOFDM *>(hndl->results))	
			{
				resultOfdm = dynamic_cast<iqapiResultOFDM *>(hndl->results);
			}
			if (resultOfdm->evmAll && resultOfdm->evmAll->length > 0)
			{
				Gmode_testResult.evm=resultOfdm->evmAll->real[0];
			}
			else
			{
				Gmode_testResult.evm=0;
				return CAPTURE_FAILURE;
			}
			if (resultOfdm->rmsPowerNoGap && resultOfdm->rmsPowerNoGap->length > 0)
			{
				Gmode_testResult.power=resultOfdm->rmsPowerNoGap->real[0];
			}
			else 
			{
				Gmode_testResult.power=0;
				return CAPTURE_FAILURE;
			}
			if (resultOfdm->freqErr)
			{
				Gmode_testResult.freqerr=resultOfdm->freqErr->real[0];
			}
		}
		if(mode==BMODE)
		{
			analysis11b=new iqapiAnalysis11b();
			analysis11b->eq_taps = IQV_EQ_OFF;
			analysis11b->DCremove11b_flag = IQV_DC_REMOVAL_ON;
			analysis11b->method_11b = IQV_11B_RMS_ERROR_VECTOR;
			hndl->analysis = dynamic_cast<iqapiAnalysis *>(analysis11b);
			errno=hndl->Analyze();
			

			if (dynamic_cast<iqapiResult11b *>(hndl->results))	
			{
				result11b = dynamic_cast<iqapiResult11b *>(hndl->results);
			}
			
			if (result11b->evmAll && result11b->evmAll->length > 0)
				Bmode_testResult.evm=result11b->evmAll->real[0];
			else
				return CAPTURE_FAILURE;
		
			if (result11b->rmsPowerNoGap && result11b->rmsPowerNoGap->length > 0)
				Bmode_testResult.power=result11b->rmsPowerNoGap->real[0];
			else
				return CAPTURE_FAILURE;
			if (result11b->freqErr)
			{
				Bmode_testResult.freqerr=result11b->freqErr->real[0];
			}

			//printf("BMODE Done\r\n");
		}
		
		return CAPTURE_SUCCESS;
}

int setIQViewTX()
{
	int err=0;
	if(channel==11)
	{
		rfFreq=CHANNEL11;
	}
	else if(channel==6)
		rfFreq=CHANNEL6;
	else if (channel==1)
		rfFreq=CHANNEL1;
	else 
		rfFreq=CHANNEL11;

		hndl->tx->rfFreqHz = rfFreq;			
		hndl->tx->freqShiftHz = 0;
		hndl->tx->txMode = IQV_INPUT_MOD_DAC_RF;	 
        hndl->tx->gapPowerOff = IQV_GAP_POWER_OFF;
		hndl->tx->vsg[0]->port =IQV_PORT_RIGHT;
		hndl->rx->vsa[0]->port =IQV_PORT_LEFT;
		hndl->tx->vsg[0]->rfGainDb = -25;
		hndl->tx->vsg[0]->enabled = IQV_RF_ENABLED;
		hndl->SetTxRx();

	return 0;
}
int setIQViewRX()
{

	//printf("In set TxRx \r\n");
	if(channel==11)
	{
		rfFreq=CHANNEL11;
	}
	else if(channel==6)
		rfFreq=CHANNEL6;
	else if (channel==1)
		rfFreq=CHANNEL1;
	else 
		rfFreq=CHANNEL11;
	hndl->rx->powerMode = IQV_VSA_TYPE_1;
	hndl->rx->vsa[0]->port=IQV_PORT_RIGHT;
	hndl->tx->vsg[0]->port=IQV_PORT_LEFT;
	hndl->rx->rxMode = IQV_INPUT_ADC_RF;
	hndl->rx->rfFreqHz=rfFreq;
	
	hndl->rx->freqShiftHz=0;
	hndl->rx->vsa[0]->extAttenDb=0;
	hndl->rx->triggerPreTime=10e-6;
	hndl->rx->triggerLevelDb=-25;
	hndl->rx->triggerType=IQV_TRIG_TYPE_IF2_NO_CAL;
	hndl->rx->samplingTimeSecs=SAMPLING_TIME;
	hndl->rx->sampleFreqHz=SAMPLING_FREQ_HZ;
	hndl->rx->vsa[0]->enabled=IQV_RF_ENABLED;
	hndl->SetTxRx();
	return 0;
}
int WritePwrLogs(char * testresult)
{
	float power=0.0;
	float evm=0.0;
	float freqerr=0.0;
	float pwrArr[6];
	float evmArr[6];
	float freqArr[6];
	char * strArr[6];
	int i=0,j=0;
	Gmode_testResult.str[0]="Initial Measurement";
	Bmode_testResult.str[0]="Initial Measurement";
	
	Pwrlogfile = fopen(filepath, "a+");
	if (Pwrlogfile == NULL)
	{
		printf("Error reopening the file\n");
	}
	
	if(mode==GMODE)
	{
		i=0;
		power=Gmode_testResult.power;
		evm=Gmode_testResult.evm;
		freqerr=Gmode_testResult.freqerr;
		while(Gmode_testResult.str[i]!='\0')
		{
			strArr[i]=Gmode_testResult.str[i];
			pwrArr[i]=Gmode_testResult.pwrArr[i];
			evmArr[i]=Gmode_testResult.evmArr[i];
			freqArr[i]=Gmode_testResult.freqArr[i];
			i++;
			j++;
		}

	}
	else if(mode==BMODE)
	{
		i=0;
		j=0;
		power=Bmode_testResult.power;
		evm=Bmode_testResult.evm;
		freqerr=Bmode_testResult.freqerr;
		while(Bmode_testResult.str[i]!='\0')
		{
			strArr[i]=Bmode_testResult.str[i];
			pwrArr[i]=Bmode_testResult.pwrArr[i];
			evmArr[i]=Bmode_testResult.evmArr[i];
			freqArr[i]=Bmode_testResult.freqArr[i];
			i++;
			j++;
		}
		
	}
	fprintf(Pwrlogfile,"*************************************************************************************\r\n");
	fprintf(Pwrlogfile,"\r\n");
	fprintf(Pwrlogfile,MacId);
	fprintf(Pwrlogfile,"\r\n");
	fprintf(Pwrlogfile,testresult);
	fprintf(Pwrlogfile,"\r\n");
	i=0;
	while(i<j)
	{	
		
		fprintf(Pwrlogfile,strArr[i]);
		fprintf(Pwrlogfile,"\r\n");		
		fprintf(Pwrlogfile,"The Power Value is :  ");
		fprintf(Pwrlogfile,"%.2f",pwrArr[i]);
		fprintf(Pwrlogfile,"\r\n");
		fprintf(Pwrlogfile,"The EVM Value is :  ");
		fprintf(Pwrlogfile,"%.2f",evmArr[i]);
		fprintf(Pwrlogfile,"\r\n");
		fprintf(Pwrlogfile,"The Frequency error Value is :  ");
		fprintf(Pwrlogfile,"%.2f",freqArr[i]);
		fprintf(Pwrlogfile,"\r\n");
		fprintf(Pwrlogfile,"\r\n");
		i++;
	}

	fprintf(Pwrlogfile,"*************************************************************************************\r\n");
	
	fclose(Pwrlogfile);
	return 0;
}

int PowerLevelLogs(int i)
{
	Pwrlogfile = fopen(filepath, "a+");
	if (Pwrlogfile == NULL)
	{
		printf("Error reopening the file\n");
	}
	if(mode==GMODE)
	{	
		fprintf(Pwrlogfile,"The Power at %d level is %.2f \r\n",i,Gmode_testResult.pwrArr[i]);
		fprintf(Pwrlogfile,"The evm at %d level is %.2f \r\n",i,Gmode_testResult.evmArr[i]);
		fprintf(Pwrlogfile,"The freqerr at %d level is %.2f \r\n",i,Gmode_testResult.freqArr[i]);
		fprintf(Pwrlogfile,"\r\n");
	}
	if(mode==BMODE)
	{	
		fprintf(Pwrlogfile,"The Power at %d level is %.2f \r\n",i,Bmode_testResult.pwrArr[i]);
		fprintf(Pwrlogfile,"The evm at %d level is %.2f \r\n",i,Bmode_testResult.evmArr[i]);
		fprintf(Pwrlogfile,"The freqerr at %d level is %.2f \r\n",i,Bmode_testResult.freqArr[i]);
		fprintf(Pwrlogfile,"\r\n");
	}
	fclose(Pwrlogfile);
	return 0;
}

int RXTest1dB()
{
	double rfpwrmin=0.0;
	double rfpwrmax=0.0;
	char * result=NULL;
	char * PacketRecdCount=NULL;
	int PacketCount=0;
	int i=0;
	printf("In Rx Test \r\n");
	//printf("Enter the min power level for RX Test: ");
	//scanf("%lf",&rfpwrmin);
	//printf("Enter the max power level for RX Test: ");
	//scanf("%lf",&rfpwrmax);
	double rfpwr=-65;
	int signalType=1;
	char * DataToSend="sys wlan test startRx 11";
	WriteCOMPort(DataToSend);
	Sleep(300);
	
	while(rfpwr<-87)
	{
		if(!RX_TransmitSignal(rfpwr,signalType))
		{
			return FAIL;
		}
		else
		{
			temp[0]='\0';
			ReadPort=true;
			DataToSend="sys wlan mib get 20 16 0";
			WriteCOMPort(DataToSend);
			Sleep(300);
			ReadPort=false;
			result = strtok( temp, "\n" );
			while( temp != NULL ) 
			{
				result = strtok( NULL, "\n" );				
				char *CheckString=strstr(result,"Value");
				if(CheckString)
				{
					PacketRecdCount=result;
					break;
				}				
			}
			printf("Packet Received Count : %d",PacketCount);			
			DataToSend="sys wlan mib setu32 20 16 0 0";
			WriteCOMPort(DataToSend);
			Sleep(300);
			///rfpwr-=5;
		}
	}
}

int TXTest1dB()
{
	//Check if B and G mode Tuning is done

	int testtime = 0;
	int i=0;
	char sendData[40];
	
	temp[0] = '\0';
	float tempPower;
	int l;
	ReadPort=true;
	char * DataToSend="sys wlan ctrl Tuning_Status";
	WriteCOMPort(DataToSend);
	Sleep(300);	
	ReadPort=false;	
	char * CheckSUBString="G Mode Tuning NOT done";
	char *CheckString1=strstr(temp,CheckSUBString);
	if(CheckString1)
	{
		GTuningRequired=true;
	}
	CheckSUBString="B Mode Tuning NOT done";
	char *CheckString2=strstr(temp,CheckSUBString);
	if(CheckString2)
	{
		BTuningRequired=true;	
	}
	if(!GTuningRequired)
	{
		DataToSend="sys wlan ctrl Reset_Tx_Power_Tuning_G";
		WriteCOMPort(DataToSend);
		Sleep(1000);
	}
	if(!BTuningRequired)
	{
		DataToSend="sys wlan ctrl Reset_Tx_Power_Tuning_B";
		WriteCOMPort(DataToSend);
		Sleep(1000);	
	}
	
	temp[0] = '\0';
	DataToSend="cd \\";
	WriteCOMPort(DataToSend);
	Sleep(300);
	DataToSend="cd  cne/Networking/DrvCfg/WlanCfg/";
	WriteCOMPort(DataToSend);
	Sleep(300);
	if(ModuleType==MODULE_CR)
	{
		DataToSend="set Module CR";
		WriteCOMPort(DataToSend);
		Sleep(300);
		
		DataToSend ="set antenna 10";
		WriteCOMPort(DataToSend);
		Sleep(300);

		DataToSend ="sys wlan mib setu32 22 6 0 0";
		WriteCOMPort(DataToSend);
		Sleep(300);

		DataToSend ="sys wlan mib setu8 22 5 0 0x21";
		WriteCOMPort(DataToSend);
		Sleep(300);
	}
	else if(ModuleType==MODULE_CX)
	{
		DataToSend = "set Module CX";
		WriteCOMPort(DataToSend);
		Sleep(300);

		DataToSend = "set antenna 22";
		WriteCOMPort(DataToSend);
		Sleep(300);

		DataToSend = "sys wlan mib setu32 22 6 0 1";
		WriteCOMPort(DataToSend);
		Sleep(300);

		DataToSend = "sys wlan mib setu8 22 5 0 0x21";
		WriteCOMPort(DataToSend);
		Sleep(300);
	}
	DataToSend ="sys wlan test static";
	WriteCOMPort(DataToSend);
	Sleep(300);
	DataToSend ="netgen";
	WriteCOMPort(DataToSend);
	Sleep(300);
	DataToSend ="sys wlan ctrl infra 2";
	WriteCOMPort(DataToSend);
	Sleep(300);
	DataToSend ="sys wlan ctrl ssid mytestssid";
	WriteCOMPort(DataToSend);
	Sleep(1000);

	//start the TX program

	///////////////******************** G MODE ******************** ///////////////
	dataParseSize=1300;
	ReadPort=true;
	
		sprintf(sendData,"sys wlan test startTx %d 8 200 %d 1 1",channel,antenna);
		//DataToSend="sys wlan test startTx 11 8 60 10 1 1";
		WriteCOMPort(sendData);
		Sleep(1000);
		
		for (l = 0; l < 10; l++)
		{
			CheckSUBString="connected with";	
			char *CheckString3=strstr(temp,CheckSUBString);
			if(CheckString3!=NULL)
			{
				break;	
			}
			Sleep(100);
		}
		ReadPort=false;
		if(l >= 10)
			return TX_FAIL;
		Sleep(1000);
	
	while(1)
	{	
		mode = GMODE;
		testtime = 0;
		while(1)
		{
			//CatchTX: Catch TX power
			if(!TX_CaptureSignals())
			{
				if(testtime > 20)
				{
					//show test result to the program interface
					char * testResult ="Test Failed because it couldn't connect to IQVIew";					
					//printf("The Power is %.2f\r\n",Gmode_testResult.power);
					//printf("The evm is %.2f\r\n",Gmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Gmode_testResult.freqerr);
					//printf("Test Failed because it couldn't connect to IQVIew \r\n");
					WritePwrLogs(testResult);
					return TX_FAIL;
				}
				else
				{
					testtime++;
					Sleep(600);
					continue;
				}
			}
			else if((Gmode_testResult.evm < -40) || (Gmode_testResult.power < 8))
			{
				if(testtime > 20)
				{
					//show test result to the program interface
					char * testResult ="Catch Power Failed. The power or evm is not at desired level";
					//printf("The Power is %.2f\r\n",Gmode_testResult.power);
					//printf("The evm is %.2f\r\n",Gmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Gmode_testResult.freqerr);
					//printf("Catch Power Failed. The power or evm is not at desired level\r\n");
					WritePwrLogs(testResult);
					return TX_FAIL;
				}
				else
				{
					testtime++;
					Sleep(600);
					continue;
				}
			}
			else
			{
				break;
			}
		}

		Gmode_testResult.pwrArr[i]=Gmode_testResult.power;
		Gmode_testResult.evmArr[i]=Gmode_testResult.evm;
		Gmode_testResult.freqArr[i]=Gmode_testResult.freqerr;
		//Adjust only 5 times
		if(i > 4)
		{
			//show test result to the program interface
			
			char * testResult ="Catch Power Failed. The power or evm is not at desired level after adjusting 5 times";
			//printf("The Power is %.2f\r\n",Gmode_testResult.power);
			//printf("The evm is %.2f\r\n",Gmode_testResult.evm);
			//printf("The frequency error is %.2f\r\n",Gmode_testResult.freqerr);
			//printf("Catch Power Failed. The power or evm is not at desired level after adjusting 5 times\r\n");
			WritePwrLogs(testResult);
			return TX_FAIL;
		}
		i++;
	//////////////////////////////////////Determine whether to do smart step tuning
		if(1)
		{
			if(i > 1)
			{
				if(Gmode_testResult.power < ini_Gpowermin)
				{	
					Gmode_testResult.str[i]="TX_Power_UP_G 1";
					//printf("******* TX_Power_UP_G 1 *******\r\n");
					DataToSend="sys wlan ctrl Tx_Power_Up_G 1";
					WriteCOMPort(DataToSend);
					Sleep(1000);
					continue;
				}
				if(Gmode_testResult.power > ini_Gpowermax)
				{
					Gmode_testResult.str[i]="Tx_Power_Down_G 1";
					//printf("*******Tx_Power_Down_G 1 *******\r\n");
					DataToSend="sys wlan ctrl Tx_Power_Down_G 1";
					WriteCOMPort(DataToSend);
					Sleep(2000);
					continue;
				}
				if((Gmode_testResult.power > ini_Gpowermin) && (Gmode_testResult.power < ini_Gpowermax) && (Gmode_testResult.evm< ini_Gevm) &&	(Gmode_testResult.evm> -40))
				{
					//All the test data in the spec store the register setting
					DataToSend="sys wlan ctrl Confirm_Register_Setting_G";
					WriteCOMPort(DataToSend);

					//show test result to the program interface
					//printf("The Power is %.2f\r\n",Gmode_testResult.power);
					//printf("The evm is %.2f\r\n",Gmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Gmode_testResult.freqerr);
					Sleep(3000);
					break;
				}
			}	
			else
			{
				if(Gmode_testResult.power  < ini_Gpowermin)
				{
					//GFlag++;
					tempPower = ini_Gpowermin - Gmode_testResult.power ;
					if(tempPower < 0.5)
					{
						Gmode_testResult.str[i]="Tx_Power_Up_G 1";
						//printf("*******Tx_Power_Up_G 1 *******\r\n");
						DataToSend="sys wlan ctrl Tx_Power_Up_G 1";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					//determine if are CR module, do the following procedure
					if(ModuleType == MODULE_CR)
					{
						if((tempPower > 0.5) && (tempPower < 2))
						{
							Gmode_testResult.str[i]="Tx_Power_Up_G 2";
							//printf("*******Tx_Power_Up_G 2 *******\r\n");
							DataToSend="sys wlan ctrl Tx_Power_Up_G 2";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
						if(tempPower >= 2)
						{
							Gmode_testResult.str[i]="Tx_Power_Up_G 3";
							//printf("*******Tx_Power_Up_G 3 *******\r\n");
							DataToSend="sys wlan ctrl Tx_Power_Up_G 3";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
					}
					else
					{
						if((tempPower > 0.5) && (tempPower < 1.5))
						{

							Gmode_testResult.str[i]="Tx_Power_Up_G 2";
							//printf("*******Tx_Power_Up_G 2 *******\r\n");
							DataToSend="sys wlan ctrl Tx_Power_Up_G 2";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
						if(tempPower >= 1.5)
						{
							Gmode_testResult.str[i]="Tx_Power_Up_G 3";
							//printf("*******Tx_Power_Up_G 3 *******\r\n");
							DataToSend="sys wlan ctrl Tx_Power_Up_G 3";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
					}
				}
				if(Gmode_testResult.power  > ini_Gpowermax)
				{
					tempPower = Gmode_testResult.power - ini_Gpowermax;
					if(tempPower < 0.5)
					{
						Gmode_testResult.str[i]="Tx_Power_Down_G 1";
						//printf("*******Tx_Power_Down_G 1 *******\r\n");
						DataToSend="sys wlan ctrl Tx_Power_Down_G 1";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if((tempPower >= 0.5) && (tempPower < 1.5))
					{
						Gmode_testResult.str[i]="Tx_Power_Down_G 2";
						//printf("*******Tx_Power_Down_G 2 *******\r\n");
						DataToSend="sys wlan ctrl Tx_Power_Down_G 2";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if(tempPower >= 1.5)
					{
						Gmode_testResult.str[i]="Tx_Power_Down_G 3";
						//printf("*******Tx_Power_Down_G 3 *******\r\n");
						DataToSend="sys wlan ctrl Tx_Power_Down_G 3";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
				}
				if(Gmode_testResult.evm > ini_Gevm)
				{
					tempPower = Gmode_testResult.power - ini_Gpowermin;
					if((tempPower < 0) )// ||(GFlag >= 2))
					{
						//show test result to the program interface
						char * testResult ="Catch Power Failed. The power is not at desired level after adjusting 5 times";						
						//printf("The Power is %.2f\r\n",Gmode_testResult.power);
						//printf("The evm is %.2f\r\n",Gmode_testResult.evm);
						//printf("The freqerr is %.2f\r\n",Gmode_testResult.freqerr);
						WritePwrLogs(testResult);
						Sleep(3000);
						return TX_FAIL;
					}
					else
					{
						Gmode_testResult.str[i]="Tx_Power_Down_G 1";
						//printf("*******Tx_Power_Down_G 1 *******\r\n");
						DataToSend="sys wlan ctrl Tx_Power_Down_G 1";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
				}
				if((Gmode_testResult.power > ini_Gpowermin) && (Gmode_testResult.power < ini_Gpowermax) && (Gmode_testResult.evm< ini_Gevm) &&(Gmode_testResult.evm> -40))
				{
						//All the test data in the spec store the register setting
						DataToSend="sys wlan ctrl Confirm_Register_Setting_G\r";
						WriteCOMPort(DataToSend);

						//show test result to the program interface
						//printf("The Power is %.2f\r\n",Gmode_testResult.power);
						//printf("The evm is %.2f\r\n",Gmode_testResult.evm);
						//printf("The frequency error is %.2f\r\n",Gmode_testResult.freqerr);
						//printf("Catch Power Success\r\n");
						//WritePwrLogs(testResult);
						Sleep(3000);
						break;
				}
			}
		}
	}
	if( (ini_Gevm <Gmode_testResult.evm) || (Gmode_testResult.evm< (-40)) || (Gmode_testResult.power<ini_Gpowermin) || (ini_Gpowermax<Gmode_testResult.power))
	{
		return TX_FAIL;
	}
	char * testResult ="Catch  TX Power for G Mode Success";
	WritePwrLogs(testResult);
	//Only for collect test log
	
	for(int g = 0; g <= 4; g++)
	{
		ReadPort=true;
		temp[0]='\0';
		sprintf(sendData,"sys wlan test startTx %d 8 200 %d %d 1",channel,antenna,g);
		//sprintf(sendData,"sys wlan test startTx 11 8 60 10 %d 1",g);
		WriteCOMPort(sendData);
		Sleep(1000);
		int gt;
		for (gt = 0; gt <10;gt++)
		{
			CheckSUBString="connected with";	
			char *CheckString3=strstr(temp,CheckSUBString);
			if(CheckString3!=NULL)
			{
				ReadPort=false;
				break;	
			}
			else
			{
				Sleep(100);
				temp[0]='\0';
			}
		}
		if(gt >= 10)
		{
			ReadPort=false;
			//return TX_FAIL;
		}
		Sleep(3000);
		mode = GMODE;
		testtime = 0;
		while(1)
		{
			//CatchTX: Catch TX power
			if(!TX_CaptureSignals())
			{
				if(testtime >= 20)
					return TX_FAIL;
				else
				{
					
					testtime++;
					Sleep(1000);
					continue;
				}
			}
			else
			{
				Gmode_testResult.pwrArr[g]=Gmode_testResult.power;
				Gmode_testResult.evmArr[g]=Gmode_testResult.evm;
				Gmode_testResult.freqArr[g]=Gmode_testResult.freqerr;
				PowerLevelLogs(g);
				//WritePwrLogs(testResult);
				break;
			}
		}
		
	}
	Sleep(1000);

	/// Measuring Power and EVM for different power levels

	
	///////////////////////////////////************ B MODE ************///////////////////////////////////
	dataParseSize=1300;
	DataToSend ="\r";
	WriteCOMPort(DataToSend);
	Sleep(300);
	int n=0;
	i = 0;
	Sleep(1000);

	//DataToSend="sys wlan test startTx 11 3 15 10 9 1";
	sprintf(sendData,"sys wlan test startTx %d 3 200 %d 9 1",channel,antenna);
	WriteCOMPort(sendData);
	ReadPort=true;
	Sleep(1000);
	for (n=0;n<20;n++)
	{
		CheckSUBString="connected with";	
		char *CheckString3=strstr(temp,CheckSUBString);
		if(CheckString3!=NULL)
		{
			break;	
		}
		Sleep(100);
	}
	ReadPort=false;
	if(n >= 20)
		return TX_FAIL;
	Sleep(3000);
	
	while(1)
	{	
		mode = BMODE;
		testtime = 0;
		while(1)
		{
			//CatchTX: Catch TX power
			if(!TX_CaptureSignals())
			{
				if(testtime > 20)
				{
					//show test result to the program interface
					char * testResult ="Test Failed because it couldn't connect to IQVIew" ;					
					//printf("The Power is %.2f\r\n",Bmode_testResult.power);
					//printf("The evm is %.2f\r\n",Bmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Bmode_testResult.freqerr);
					//printf("Test Failed because it couldn't connect to IQVIew \r\n");
					WritePwrLogs(testResult);
					return TX_FAIL;
				}
				else
				{
					testtime++;
					Sleep(600);
					continue;
				}
			}
			else if((Bmode_testResult.evm < -40) || (Bmode_testResult.power < 8))
			{
				if(testtime > 20)
				{
					char * testResult ="Catch Power Fail: EVM or Power not in desired Range" ;						
					//printf("The Power is %.2f\r\n",Bmode_testResult.power);
					//printf("The evm is %.2f\r\n",Bmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Bmode_testResult.freqerr);
					//printf("Catch Power Fail: EVM or Power not in desired Range\r\n");
					WritePwrLogs(testResult);
					return TX_FAIL;
				}
				else
				{
					testtime++;
					Sleep(600);
					continue;
				}
			}
			else
				break;
		}
		Bmode_testResult.pwrArr[i]=Bmode_testResult.power;
		Bmode_testResult.evmArr[i]=Bmode_testResult.evm;
		Bmode_testResult.freqArr[i]=Bmode_testResult.freqerr;
		//store the test log data
		//strdata.BP[i] = tx_Bpower;
		//adjust only 5 times
		if(i > 4)
		{
			//show test result to the program interface
			char * testResult ="Catch Power Fail: EVM or Power not in desired Range" ;
			//printf("The Power is %.2f\r\n",Bmode_testResult.power);
			//printf("The evm is %.2f\r\n",Bmode_testResult.evm);
			//printf("The frequency error is %.2f\r\n",Bmode_testResult.freqerr);
			//printf("Catch Power :Fail EVM or Power not in desired Range\r\n");
			WritePwrLogs(testResult);
			return TX_FAIL;
		}
		i++;
		//Determine whether to do smart step tuning
		if(1)
		{
			if(i > 1)
			{
				if(Bmode_testResult.power < ini_Bpowermin)
				{
					Bmode_testResult.str[i]="Tx_Power_Up_B 1";
					//printf("********** Tx_Power_Up_B 1 **********");
					DataToSend ="sys wlan ctrl Tx_Power_Up_B 1";
					WriteCOMPort(DataToSend);
					Sleep(1000);
					continue;
				}
				if(Bmode_testResult.power > ini_Bpowermax)
				{
					Bmode_testResult.str[i]="Tx_Power_Down_B 1";
					//printf("********** Tx_Power_Down_B 1 **********");
					DataToSend ="sys wlan ctrl Tx_Power_Down_B 1";
					WriteCOMPort(DataToSend);
					Sleep(1000);
					continue;
				}
				if((Bmode_testResult.power > ini_Bpowermin) && (Bmode_testResult.power < ini_Bpowermax) && (Bmode_testResult.evm < ini_Bevm) && (Bmode_testResult.evm > -40))
				{
					//All the test data in the spec store the register setting
					DataToSend ="sys wlan ctrl Confirm_Register_Setting_B";
					WriteCOMPort(DataToSend);

					//show test result to the program interface
					//printf("The Power is %.2f\r\n",Bmode_testResult.power);
					//printf("The evm is %.2f\r\n",Bmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Bmode_testResult.freqerr);
					Sleep(3000);
					break;
				}
			}
			else
			{
				if(Bmode_testResult.power < ini_Bpowermin)
				{
					tempPower = ini_Bpowermin - Bmode_testResult.power;
					if(tempPower < 0.5)
					{
						Bmode_testResult.str[i]="Tx_Power_Up_B 2";	
						//printf("********** Tx_Power_Up_B 2 **********");
						DataToSend ="sys wlan ctrl Tx_Power_Up_B 2";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if((tempPower >= 0.5) && (tempPower < 1))
					{
						Bmode_testResult.str[i]="Tx_Power_Up_B 3";					
						//printf("********** Tx_Power_Up_B 3 **********");
						DataToSend ="Sys wlan ctrl Tx_Power_Up_B 3";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if(ModuleType == MODULE_CR)
					{
						if((tempPower >= 1) && (tempPower < 2))
						{
							Bmode_testResult.str[i]="Tx_Power_Up_B 4";
							//printf("********** Tx_Power_Up_B 4 **********");
							DataToSend ="sys wlan ctrl Tx_Power_Up_B 4";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
						if(tempPower >= 2)
						{
							Bmode_testResult.str[i]="Tx_Power_Up_B 5";
							//printf("********** Tx_Power_Up_B 5 **********");
							DataToSend ="sys wlan ctrl Tx_Power_Up_B 5";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
					}
					else
					{
						if(tempPower >= 1)
						{
							Bmode_testResult.str[i]="Tx_Power_Up_B 4";
							//printf("********** Tx_Power_Up_B 4 **********");
							DataToSend ="sys wlan ctrl Tx_Power_Up_B 4";
							WriteCOMPort(DataToSend);
							Sleep(1000);
							continue;
						}
					}
				}
				if(Bmode_testResult.power > ini_Bpowermax)
				{
					tempPower = Bmode_testResult.power - ini_Bpowermax;
					if(tempPower < 0.5)
					{
						Bmode_testResult.str[i]="Tx_Power_Down_B 1";
						//printf("********** Tx_Power_Down_B 1 **********");
						DataToSend ="sys wlan ctrl Tx_Power_Down_B 1";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if((tempPower >= 0.5) && (tempPower < 1.5))
					{
						Bmode_testResult.str[i]="Tx_Power_Down_B 2";
						//printf("********** Tx_Power_Down_B 2 **********");
						DataToSend ="sys wlan ctrl Tx_Power_Down_B 2";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if((tempPower >= 1.5) && (tempPower < 2))
					{
						Bmode_testResult.str[i]="Tx_Power_Down_B 3";
						//printf("********** Tx_Power_Down_B 3 **********");
						DataToSend ="sys wlan ctrl Tx_Power_Down_B 3";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
					if(tempPower >= 2)
					{
						Bmode_testResult.str[i]="Tx_Power_Down_B 4";
						//printf("********** Tx_Power_Down_B 4 **********");
						DataToSend ="sys wlan ctrl Tx_Power_Down_B 4";
						WriteCOMPort(DataToSend);
						Sleep(1000);
						continue;
					}
				}
				if((Bmode_testResult.power > ini_Bpowermin) && (Bmode_testResult.power < ini_Bpowermax) && (Bmode_testResult.evm < ini_Bevm) &&(Bmode_testResult.evm > -40))
				{
					DataToSend ="sys wlan ctrl Confirm_Register_Setting_B";
					WriteCOMPort(DataToSend);
					
					//show test result to the program interface
					//printf("The Power is %.2f\r\n",Bmode_testResult.power);
					//printf("The evm is %.2f\r\n",Bmode_testResult.evm);
					//printf("The frequency error is %.2f\r\n",Bmode_testResult.freqerr);
					Sleep(3000);
					break;
				}
			}
		}
	}
	if( (ini_Bevm <Bmode_testResult.evm) || (Bmode_testResult.evm< (-40))|| (Bmode_testResult.power < ini_Bpowermin) || (ini_Bpowermax < Bmode_testResult.power))
	{
		return TX_FAIL;
	}

	testResult ="Catch  TX Power for B Mode Success";
	WritePwrLogs(testResult);
	//Only for collect test log
	double Bpower = 0.0;
	double Bevm = 0.0;
	double Bfreqerr = 0.0;

	for(int b = 8; b <= 12; b++)
	{
		ReadPort=true;
		temp[0]='\0';
		sprintf(sendData,"sys wlan test startTx %d 3 200 %d %d 1",channel,antenna,b);
		WriteCOMPort(sendData);
		Sleep(1000);
		int bt;
		for (bt = 0; bt < 20; bt++)
		{
			CheckSUBString="connected with";	
			char *CheckString3=strstr(temp,CheckSUBString);
			if(CheckString3!=NULL)
			{
				ReadPort=false;
				break;	
			}
			else
			{
				Sleep(100);
				temp[0]='\0';
			}
		}
		if(bt >= 20)
			return TX_FAIL;
		Sleep(3000);
		mode = BMODE;
		testtime = 0;
		while(1)
		{
			//CatchTX: Catch TX power
			if(!TX_CaptureSignals())
			{
				if(testtime >= 20)
					return TX_FAIL;
				else
				{
					testtime++;
					Sleep(1000);
					continue;
				}
			}
			else
			{
				Bmode_testResult.pwrArr[b]=Bmode_testResult.power;
				Bmode_testResult.evmArr[b]=Bmode_testResult.evm;
				Bmode_testResult.freqArr[b]=Bmode_testResult.freqerr;
				PowerLevelLogs(b);
				break;
			}
		}
	}
	//else
	return TX_SUCCESS;
}



int main(void)
{
	char ch;
	int returnErr=0;	

	//Open the COM port
	//printf("COM Port Name :%s\r\n",&COMPortName);
	

	/*returnErr=InitializeAndConnectIQView();
	if(returnErr!=0)
	{
		printf("\r\nTerminating the program as Initialising IQAPI Libraries or Connection to IQView Failed.Press enter and restart the test \r\n \r\n");
		getch();
		exit(0);
	}
	else
	{
		//printf("Established connection with IQView\r\n");
	}
	
	returnErr=setIQViewRX();
	if(returnErr==0)
	{
		//printf("Set TX success \r\n");
	}*/
	//Call the 1 db Tuning test
	do
	{
		printf("Enter the COM Port name as COM1, COM2 :");
		scanf("%s",COMPortName);
		printf("Enter the module type as CR or CX :");
		scanf("%s",Module);
		if(strcmp(Module,"CR")==0||strcmp(Module,"cr")==0)
		{
			ModuleType=MODULE_CR;
		}
		else if(strcmp(Module,"CX")==0||strcmp(Module,"cx")==0 )
		{
			ModuleType=MODULE_CX;
		}
		else
		{
			printf("Wrong type of Module entered.Press enter and restart the test\r\n");
			getch();
			exit(0);
		}
		printf("Enter the MAC ID  :");
		scanf("%s",MacId);
		printf("Enter the channel to be used for testing. :");
		scanf("%d",&channel);
		printf("Enter the antenna :");
		scanf("%d",&antenna);
		returnErr=InitializeDUT(COMPortName);
		//printf("port opening return code %d \r\n",returnErr);
		if(returnErr!=0)
		{
			printf("Terminating the program as opening COM port Failed.Press enter and restart the test \r\n");
			getch();
			exit(0);
		}		
		HANDLE PortReadThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&readComPort, NULL, 0,NULL);	
		Sleep(1000);
		sprintf(filepath,".//PwrLogs%s.txt",MAC);
		printf("%s",filepath);
		if(PortReadThread)
		{
			//printf("Reading port Thread Success \r\n");
		}
		RXTest1dB();
		//TXTest1dB();
		TerminateThread(PortReadThread,0);
		printf("\r\n \r\nTest Over.To Continue with other modules, Disconnect and place the next module boot to BSL and press 'Y' or 'y' \r\n");
		
		ch=getch();
		DisconnectDUT();
	}while((ch=='Y')||(ch=='y'));
	
	DisconnectDUT();
	DiconnectIQVIEW();
	return 0;
}

  


