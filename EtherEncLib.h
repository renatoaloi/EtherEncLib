
/*
  EtherEncLib.h - Library for Ethernet ENC29J60 Module.
  Created by Renato Aloi, August 27, 2013.
  Released into the public domain.
*/

#include <Arduino.h>
#include "TcpStack.h"

#ifndef ETHERENCLIB_H
#define ETHERENCLIB_H

// by Renato Aloi May 2015
// Trying to implement SocketTX avoiding RAM buffers
// New rule of 3 buffers
// 1. Buffer for incomming TCP Headers (58 bytes) -- TcpStack->m_tcpData[]
// 2. Buffer for outcomming TCP Headers (58 bytes) -- TcpStack->m_sendData[]
// 3. Buffer for Http parameters (50 bytes) -- EtherEncLib->m_httpData[]
// Total RAM from Buffers: 166 bytes + 20 bytes = 186 bytes (from 2x ip and 2x mac addresses: (2 * 4 bytes) + (2 * 6 bytes))

#define BUFFER_PARAMS_LEN  50 // buffering only parameters and tcp header to conserve RAM
			      // TCP Headers Buffers = 116 bytes + Params Buffer = 50
			      // Total = 166 bytes total RAM consumption
			      // by Renato Aloi (May 2015)
#define DEBUGLIB           0

class EtherEncLib
{
  
  private:
	unsigned int m_port;
	TcpStack m_stack;
	char  m_httpData[BUFFER_PARAMS_LEN];

	int freeRam () {
	  extern int __heap_start, *__brkval;
	  int v;
	  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	};
	unsigned char analize    (void); // by SKA
    
  public:
    EtherEncLib(unsigned int port) : m_port(port), m_stack(TcpStack()) {
		for (unsigned i = 0; i < BUFFER_PARAMS_LEN; i++)  m_httpData[i] = 0;
	};
   
    void          begin      (unsigned char *ip, unsigned char *mac);
    unsigned char available  (void);
    void          print      (char c);
    void          print      (char *rd);
    void          print      (unsigned int val);
    void          print      (int val);
    void	  print      (char *, unsigned char); // made by SKA
    void          close      (void);
    char	  read       (void);
    char          *getParams (void);
//--- made by SKA ---
    
    unsigned char isGet;
    unsigned char isPost;
    unsigned char isIndexHtml;

//--- made by SKA ---
};

#endif
