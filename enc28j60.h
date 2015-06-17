/*
    EtherEncLib - Ethernet ENC28J60 Library for Arduino
    Copyright (C) 2015  Renato Aloi
    renato.aloi@gmail.com -- http://www.seriallink.com.br

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "enc28typedef.h"
#include <stdint.h>
#include <avr/io.h>

//--- made by SKA ---
#include <SPI.h>

#define low(a)          (a&0xFF)
#define high(a)         ((a>>8)&0xFF)

//typedef enum _BOOL { FALSE = 0, TRUE } BOOL;
typedef enum _FLOW { RX = 0, TX } FLOW;

typedef union
{
    unsigned int Val;
    struct
    {
        unsigned char LB;
        unsigned char HB;
    } Byte;
    struct
    {
         unsigned char b0:1;
         unsigned char b1:1;
         unsigned char b2:1;
         unsigned char b3:1;
         unsigned char b4:1;
         unsigned char b5:1;
         unsigned char b6:1;
         unsigned char b7:1;
         unsigned char b8:1;
         unsigned char b9:1;
         unsigned char b10:1;
         unsigned char b11:1;
         unsigned char b12:1;
         unsigned char b13:1;
         unsigned char b14:1;
         unsigned char b15:1;
    } Bits;
} WORD_VAL, WORD_BITS;


/******************************************************************************
 * APPLICATION LAYER METHOD DECLARATIONS
 *****************************************************************************/
void 		MACInit(void);
void 		MACOpen(void);
void 		MACEnableRecv(void);
void 		MACInitMacAddr(unsigned char *_macadd);
unsigned char *MACGetMacAddr(void);
unsigned char 	MACHardwareRevision(void);
void 		MACSendSystemReset(void);

unsigned char 	MACGetPacketCount(void);
void 		MACReadRXBuffer(unsigned char* _buf, unsigned int _size);
void 		MACDiscardRx(void);
void 		MACWriteTXBuffer(unsigned char* _buf, unsigned int _size);
void 		MACWriteTXBufferOffset(unsigned char* _buf, unsigned int _size, unsigned int offset_len);
void        	MACWriteTXBufferOffset2(uint8_t* _buf, uint16_t _size, uint16_t offset_len, uint16_t offset_val);
void 		MACWriteTXEndPt(unsigned int _size);
void 		MACSendTx(void);
//--- made by SKA ---
bool 		IsMACSendTx(void);

/******************************************************************************
 * SOCKET APPLICATION LAYER METHOD DECLARATIONS
 *****************************************************************************/
void 		SOCKETReadBuffer(unsigned char* _buf, unsigned int _size, unsigned int _start);
void 		SOCKETWriteBuffer(unsigned char* _buf, unsigned int _size, unsigned int _start);
unsigned int 	SOCKETGetRxPointer();
void 		SOCKETSetRxPointer(unsigned int addr);
unsigned int 	SOCKETGetTxPointer();
void 		SOCKETSetTxPointer(unsigned int addr);


/******************************************************************************
 * DMA COPY LAYER METHOD DECLARATIONS
 *****************************************************************************/
void DMACopy(FLOW flow, unsigned int destAddr, unsigned int len);
//--- made by SKA ---
bool IsDMACopyDone(void);

