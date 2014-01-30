int InitializeDUT(char * PortName_u8);
int DisconnectDUT();
int InitializeAndConnectIQView();
int DiconnectIQVIEW();
int RX_TransmitSignal(double rfpwr,int sigtype);
int TX_CaptureSignals();
int setIQViewTX();
int setIQViewRX();
int WritePwrLogs(char * testresult);
int RXTest1dB();

