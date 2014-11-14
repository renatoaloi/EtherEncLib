#include "enc28typedef.h"
#include <stdint.h>
#include <avr/io.h>

#define low(a)          (a&0xFF)
#define high(a)         ((a>>8)&0xFF)

typedef enum _BOOL { FALSE = 0, TRUE } BOOL;
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
void        MACWriteTXBufferOffset2(uint8_t* _buf, uint16_t _size, uint16_t offset_len, uint16_t offset_val);
void 		MACSendTx(void);
BOOL 		IsMACSendTx(void);

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
void DMACopyTo(FLOW flow, unsigned int destAddr, unsigned int len);
void DMACopyFrom(FLOW flow, unsigned int sourceAddr, unsigned int len);
BOOL IsDMACopyDone(void);

//







