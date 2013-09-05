
/*
  EtherEncLib.h - Library for Ethernet ENC29J60 Module.
  Created by Renato Aloi, August 27, 2013.
  Released into the public domain.
*/

#include <Arduino.h>

#ifndef ETHERENCLIB_H
#define ETHERENCLIB_H

#ifndef DEBUG
#define DEBUG              0
#endif

#define PORTLEN            2
#define IPLEN              4
#define MACLEN             6
#define SEQNUMLEN          4
#define BUFFER_READ_LEN    130 
#define BUFFER_WRITE_LEN   130 
#define BUFFER_PARAMS_LEN  50 
#define ETH_ARP_LEN        42
#define CHECKSUMLEN        8
#define TCPHEADEROPTLEN    4
#define CHECKSUMTCPID      2

#define MAX_PRINT_LEN      150
#define MAX_CHUNK_LEN      15


#define TEMPO_GET_POST     1000
#define TEMPO_PRINT        1000
#define TEMPO_FIN_ACK      1000

class EtherEncLib
{
  
  private:
    unsigned int  _port;
    unsigned char _ip[IPLEN];
    unsigned char _mac[MACLEN];
    unsigned char _bufferRd[BUFFER_READ_LEN];
    unsigned char _bufferWr[BUFFER_WRITE_LEN];
    char          _bufferPr[BUFFER_PARAMS_LEN];
    
    unsigned char _sessionLock;
    unsigned int  _sessionPortSrc;
    
    unsigned char recebeuRequestFlag;
    unsigned char tratamentoGetPostFlag;
    unsigned char tratamentoPrintFlag;
    unsigned char tratamentoCloseFlag;
    unsigned char tratamentoCloseAckFlag;
    
    unsigned int  tratamentoGetPostCounter;
    
    // Func declarations
    void          initBuffer();
    void          initBufferRead();
    void          initBufferWrite();
    void          initBufferWriteData();
    void          initBufferParams();
    void          initIP();
    void          initMAC();
    
    void          loadIP(unsigned char *address);
    void          loadMAC(unsigned char *address);
    void          loadBuffer();
    void          loadBufferParams();
    
    unsigned char isBufferRead();
    unsigned char isMyIp();
    unsigned char isMyIpHeader();
    unsigned char isMyMac();
    
    void          makeEthHeader();
    void          makeEthHeaderWithoutLoadBuffer();
    void          makeIpHeader();
    void          makeTcpPortsInverter();
    void          makeTcpHeaderSyn();
    void          makeTcpHeaderPush(unsigned int _payload);
    
    void          formatArpResponse();
    void          formatIcmpResponse();
    void          formatTcpSeqAck();
    void          formatTcpSeqAckPayload(unsigned int payload);
    void          formatTcpSynResponse();
    void          formatAckPushResponse(unsigned char _lenData);
    void          formatAckPushPrint(unsigned char _lenData);
    void          formatAckPushClose();
    void          formatAckPushCloseAck();
    void          formatAckPushCloseAck2();
    
    void          lerPacoteEth();
    unsigned char isBusy();
    void          sendHttpHeader();
    void          sendPrint(char *rd, unsigned int len);
    void          initSessao();
    unsigned char isReturnOk(unsigned long t);
    void          sendClose();
    void          sendCloseAck();
    void          sendCloseAck2();
    unsigned int  getPrintStringLen(char *_rd);
    void          print(char *rd, unsigned int len);
    void          resetSession();
    
  public:
    EtherEncLib(unsigned int port);
   
    void          begin     (unsigned char *ip, unsigned char *mac);
    unsigned char available ();
    void          print     (char *rd);
    void          print     (unsigned int val);
    void          close     ();
    char          *getParams ();
};

#endif