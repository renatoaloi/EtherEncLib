
/*
  EtherEncLib.h - Library for Ethernet ENC29J60 Module.
  Created by Renato Aloi, August 27, 2013.
  Released into the public domain.
*/

#include <Arduino.h>
#include "TcpStack.h"

#ifndef ETHERENCLIB_H
#define ETHERENCLIB_H

#define MAX_PRINT_LEN      150
#define MAX_CHUNK_LEN      15
#define BUFFER_PARAMS_LEN  50
#define DEBUGLIB           0

class EtherEncLib
{
  
  private:
	unsigned int m_port;
	TcpStack m_stack;
	char  m_httpData[BUFFER_PARAMS_LEN];
		
	unsigned int getPrintStringLen(char *_rd);
    
  public:
    EtherEncLib(unsigned int port) : m_port(port), m_stack(TcpStack()) {
		for (unsigned i = 0; i < BUFFER_PARAMS_LEN; i++)  m_httpData[i] = 0;
	};
   
    void          begin      (unsigned char *ip, unsigned char *mac);
    unsigned char available  (void);
    void          print      (char *rd);
    void          print      (unsigned int val);
    void          close      (void);
    char	  read       (void);
    char          *getParams (void);
};

#endif
