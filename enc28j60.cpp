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
#include "enc28j60.h"


#ifndef Arduino_h
void delay(unsigned long);
#endif

/******************************************************************************
 * APPLICATION LAYER VARIABLES
 *****************************************************************************/
static WORD_VAL         NextPacketLocation;
static WORD_VAL         CurrentPacketLocation;
static unsigned char    Enc28j60Bank;
static unsigned char    *m_macadd;//[6];
//--- made by SKA ---static unsigned char    *m_ipadd;//[4];


/******************************************************************************
 * HARDWARE LAYER METHOD DECLARATIONS
 *****************************************************************************/
static void         enc28j60SetBank(uint8_t address);
static void         enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data);
static void         enc28j60Write(uint8_t address, uint8_t data);
static uint8_t      enc28j60ReadOp(uint8_t op, uint8_t address);
static uint8_t      enc28j60Read(uint8_t address);
static void         enc28j60ReadBuffer(uint16_t len, uint8_t* data);
static void         enc28j60PhyWrite(uint8_t address, uint16_t data);
static void         enc28j60WriteBuffer(uint16_t len, uint8_t* data);



/******************************************************************************
 * APPLICATION LAYER
 *****************************************************************************/

/******************************************************************************
 * Function:        void MACInit(void)
 *
 * PreCondition:    none
 *
 * Input:           none
 *
 * Output:          None
 *
 * Side Effects:    none
 *
 * Overview:        Init. ENC28J60 hardware
 *
 * Note:            None
 *****************************************************************************/
void MACInit(void)
{
	
    // 1.
    
    NextPacketLocation.Val = 0;
    CurrentPacketLocation.Val = 0;
    
    // RESET the entire ENC28J60, clearing all registers
    MACSendSystemReset();
    
    // Start up in Bank 0 and configure the receive buffer boundary pointers
    // and the buffer write protect pointer (receive buffer read pointer)
    NextPacketLocation.Val = RXSTART_INIT;
    CurrentPacketLocation.Val = RXSTART_INIT;

    enc28j60Write(ERXSTL, low(RXSTART_INIT));
    enc28j60Write(ERXSTH, high(RXSTART_INIT));
    enc28j60Write(ERXRDPTL, low(RXSTART_INIT));    // Write low byte first
    enc28j60Write(ERXRDPTH, high(RXSTART_INIT));   // Write high byte last
    enc28j60Write(ERXNDL, low(RXSTOP_INIT));
    enc28j60Write(ERXNDH, high(RXSTOP_INIT));
    enc28j60Write(ETXSTL, low(TXSTART_INIT));
    enc28j60Write(ETXSTH, high(TXSTART_INIT));

    // Enter Bank 1 and configure Receive Filters
    // For broadcast packets we allow only ARP packtets
    // All other packets should be unicast only for our mac (MAADR)
    //
    // The pattern to match on is therefore
    // Type     ETH.DST
    // ARP      BROADCAST
    // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
    // in binary these poitions are:11 0000 0011 1111
    // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
    enc28j60Write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
    enc28j60Write(EPMM0, 0x3F);
    enc28j60Write(EPMM1, 0x30);
    enc28j60Write(EPMCSL, 0xF9);
    enc28j60Write(EPMCSH, 0xF7);    
    // promiscuous mode
    //enc28j60Write(ERXFCON, ERXFCON_CRCEN);
    
    // Disable the CLKOUT output to reduce EMI generation
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECOCON, 0x00);   // Output off (0V)
    //enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECOCON, 0x01); // 25.000MHz
    //enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECOCON, 0x03); // 8.3333MHz (*4 with PLL is 33.3333MHz)
    
    // Enable the receive portion of the MAC
    // full duplex
    //enc28j60Write(MACON1, (MACON1_MARXEN | MACON1_TXPAUS | MACON1_RXPAUS));
    // half duplex
    enc28j60Write(MACON1, MACON1_MARXEN);
    
    // bring MAC out of reset
    enc28j60Write(MACON2, 0x00);
    
    // Pad packets to 60 bytes, add CRC, and check Type/Length field.
    // full duplex
    //enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3,
    //                MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN | MACON3_FULDPX);
    // half duplex
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3,
                   MACON3_PADCFG0 | MACON3_TXCRCEN | MACON3_FRMLNEN);
    
    // Allow infinite deferals if the medium is continuously busy
    // (do not time out a transmission if the half duplex medium is
    // completely saturated with other people's data)
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON4, MACON4_DEFER);
    // Clear because we are in full duplex mode
    //enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, MACON4, MACON4_DEFER);
    
    // Set the maximum packet size which the controller will accept
    unsigned int limit = 1518;  // 6+6+2+1500+4
                                // 1518 is the IEEE 802.3 specified limit
    enc28j60Write(MAMXFLL, low(limit));  
    enc28j60Write(MAMXFLH, high(limit));
    
    // set inter-frame gap (back-to-back)
    // half duplex
    enc28j60Write(MABBIPG, 0x12);
    // full duplex
    //enc28j60Write(MABBIPG, 0x15);
    
    // set inter-frame gap (non-back-to-back)
    enc28j60Write(MAIPGL, 0x12);
    // half duplex
    enc28j60Write(MAIPGH, 0x00);
    // full duplex
    //enc28j60Write(MAIPGH, 0x0C);

    // Late collisions occur beyond 63+8 bytes (8 bytes for preamble/start of frame delimiter)
    // 55 is all that is needed for IEEE 802.3, but ENC28J60 B5 errata for improper link pulse
    // collisions will occur less often with a larger number.
    enc28j60Write(MACLCON2, 63);
    
}


/******************************************************************************
 * Function:        void MACOpen(void)
 *
 * PreCondition:    none
 *
 * Input:           none
 *
 * Output:          None
 *
 * Side Effects:    none
 *
 * Overview:        Open ENC28J60 hardware
 *
 * Note:            None
 *****************************************************************************/
void MACOpen(void)
{
    // 4.
    
    // Set the MAC and PHY into the proper duplex state
    //enc28j60PhyWrite(PHCON1, PHCON1_PDPXMD);
    enc28j60PhyWrite(PHCON1, 0x0000);
    
    // Disable half duplex loopback in PHY.  Bank bits changed to Bank 2 as a
    // side effect.
    // no loopback of transmitted frames
    enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

    // switch to bank 0
    enc28j60SetBank(ECON1);
}

/******************************************************************************
 * Function:        void MACEnableRecv(void)
 *
 * PreCondition:    none
 *
 * Input:           none
 *
 * Output:          None
 *
 * Side Effects:    none
 *
 * Overview:        Enable ENC28J60 hardware receiving status
 *
 * Note:            None
 *****************************************************************************/
void MACEnableRecv(void)
{
    // 5.
    
    // enable interrupts
    //enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
    // enable packet reception
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
}

/******************************************************************************
 * Function:        void MACInitMacAddr(unsigned char *_macadd)
 *
 * PreCondition:    none
 *
 * Input:           none
 *
 * Output:          None
 *
 * Side Effects:    none
 *
 * Overview:        Init ENC28J60 hardware MAC address
 *
 * Note:            None
 *****************************************************************************/
void MACInitMacAddr(unsigned char *macaddr)
{
    // 2.
	m_macadd = macaddr; //*(&macaddr[0]);
	//m_macadd[3] = 192;
    
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    enc28j60Write(MAADR5, m_macadd[0]);
    enc28j60Write(MAADR4, m_macadd[1]);
    enc28j60Write(MAADR3, m_macadd[2]);
    enc28j60Write(MAADR2, m_macadd[3]);
    enc28j60Write(MAADR1, m_macadd[4]);
    enc28j60Write(MAADR0, m_macadd[5]);
}

unsigned char *MACGetMacAddr(void)
{
	return (unsigned char *)m_macadd;
}


/******************************************************************************
 * Function:        unsigned char MACHardwareRevision(void)
 *
 * PreCondition:    Must call MACOpen() first
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Returns hardware revision
 *
 * Note:            None
 *****************************************************************************/
unsigned char MACHardwareRevision(void)
{
    return enc28j60Read(EREVID);
}

/******************************************************************************
 * Function:        void MACSendSystemReset(void)
 *
 * PreCondition:    SPI bus must be initialized (done in MACInit()).
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        MACSendSystemReset sends the System Reset SPI command to
 *                  the Ethernet controller.  It resets all register contents
 *                  (except for ECOCON) and returns the device to the power
 *                  on default state.
 *
 * Note:            None
 *****************************************************************************/
void MACSendSystemReset(void)
{
    // perform system reset
    enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
    delay(50);
        
}//end MACSendSystemReset






/******************************************************************************
 * Function:        unsigned char MACGetPacketCount(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Check EPKTCNT to know how many packets arrived
 *
 * Note:            None
 *****************************************************************************/
unsigned char MACGetPacketCount(void)
{
    return enc28j60Read(EPKTCNT);
}

/******************************************************************************
 * Function:        void MACReadRXBuffer(unsigned char* _buf, unsigned int _size)
 *
 * PreCondition:    none
 *
 * Input:           *_buf: buffer to write data read
 *                  _size: amount data to read
 *
 * Output:          None
 *
 * Side Effects:    Last packet is discarded if MACDiscardRx() hasn't already
 *                  been called.
 *
 * Overview:        None
 *
 * Note:            None
 *****************************************************************************/
void MACReadRXBuffer(unsigned char* _buf, unsigned int _size)
{
    uint16_t rxstat;
    uint16_t len;
    
    // Set the read pointer to the start of the received packet
    enc28j60Write(ERDPTL, CurrentPacketLocation.Byte.LB);
    enc28j60Write(ERDPTH, CurrentPacketLocation.Byte.HB);
    
    // read the next packet pointer
    NextPacketLocation.Byte.LB  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    NextPacketLocation.Byte.HB  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    
    // read the packet length (see datasheet page 43)
    len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
    len-=4; //remove the CRC count
    
    // read the receive status (see datasheet page 43)
    rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
    rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
    
    // limit retrieve length
    if (len > _size) len = _size;
    
    // copy the packet from the receive buffer
    enc28j60ReadBuffer(len, _buf);
}

/******************************************************************************
 * Function:        void MACDiscardRx(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Marks the last received packet (obtained using
 *                  MACGetHeader())as being processed and frees the buffer
 *                  memory associated with it
 *
 * Note:            Is is safe to call this function multiple times between
 *                  MACGetHeader() calls.  Extra packets won't be thrown away
 *                  until MACGetHeader() makes it available.
 *****************************************************************************/
void MACDiscardRx(void)
{
    // Update Current pointer
    CurrentPacketLocation.Val = NextPacketLocation.Val;
    
    enc28j60Write(ERXRDPTL, NextPacketLocation.Byte.LB);
    enc28j60Write(ERXRDPTH, NextPacketLocation.Byte.HB);

    // decrement the packet counter indicate we are done with this packet
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);
}

/******************************************************************************
 * Function:        void MACWriteTXBuffer(unsigned char* _buf, unsigned int _size)
 *
 * PreCondition:    none
 *
 * Input:           *_buf: buffer to write data 
 *                  _size: amount data to write
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Writes bytes to TX Buffer space
 *
 * Note:            None
 *****************************************************************************/
void MACWriteTXBuffer(unsigned char* _buf, unsigned int _size)
{
    MACWriteTXBufferOffset(_buf, _size, 0);
}

/******************************************************************************
 * Function:        void MACWriteTXBufferOffset(unsigned char* _buf, unsigned int _size, unsigned int offset_len)
 *
 * PreCondition:    none
 *
 * Input:           *_buf: buffer to write data 
 *                  _size: amount data to write
 *                  offset:memory pointer to write data
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Writes bytes to TX Buffer space and skips offset
 *
 * Note:            None
 *****************************************************************************/
void MACWriteTXBufferOffset(unsigned char* _buf, unsigned int _size, unsigned int offset_len)
{
    // calc offset
    /*unsigned int offsetStart  = TXSTART_INIT + offset_len;
    unsigned int offsetEnd    = offsetStart + _size; 
    
    // Set the write pointer to start of transmit buffer area
    enc28j60Write(EWRPTL, low(offsetStart));
    enc28j60Write(EWRPTH, high(offsetStart));
    
    // Set the TXND pointer to correspond to the packet size given
    enc28j60Write(ETXNDL, low(offsetEnd));
    enc28j60Write(ETXNDH, high(offsetEnd));
    
    // write per-packet control byte (0x00 means use macon3 settings)
    enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
    
    // copy the packet into the transmit buffer
    enc28j60WriteBuffer(_size, _buf);*/

    MACWriteTXBufferOffset2(_buf, _size, offset_len, 0);
}

/******************************************************************************
 * Function:        void MACWriteTXBufferOffset2(uint8_t* _buf, uint16_t _size, uint16_t offset_len, uint16_t offset_val)
 *
 * PreCondition:    none
 *
 * Input:           *_buf: buffer to write data 
 *                  _size: amount data to write
 *                  offset_len:memory pointer to write data
 *                  offset_val:incremental offset
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Writes bytes to TX Buffer space and skips offset
 *
 * Note:            None
 *****************************************************************************/
void MACWriteTXBufferOffset2(uint8_t* _buf, uint16_t _size, uint16_t offset_len, uint16_t offset_val)
{
    // calc offset
    unsigned int offsetStart  = TXSTART_INIT + offset_len;
    unsigned int offsetEnd    = offsetStart + _size; 
    
    // Set the write pointer to start of transmit buffer area
    enc28j60Write(EWRPTL, low(offsetStart));
    enc28j60Write(EWRPTH, high(offsetStart));
    
    // Set the TXND pointer to correspond to the packet size given
    enc28j60Write(ETXNDL, low(offsetEnd));
    enc28j60Write(ETXNDH, high(offsetEnd));
    
    // write per-packet control byte (0x00 means use macon3 settings)
    if (!offset_val)
    {
        enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);
    }

    // copy the packet into the transmit buffer
    enc28j60WriteBuffer(_size, _buf);
}

void MACWriteTXEndPt( unsigned int _size)
{
    // calc end offset
    unsigned int offsetEnd    = TXSTART_INIT + _size; 
    
    // Set the TXND pointer to correspond to the packet size given
    enc28j60Write(ETXNDL, low(offsetEnd));
    enc28j60Write(ETXNDH, high(offsetEnd));
}

/******************************************************************************
 * Function:        void MACSendTx(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Send Tx buffer
 *
 * Note:            None
 *****************************************************************************/
void MACSendTx(void)
{
    // clear TXIF and config transmit interrupt if the case
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF);
    
    // send the contents of the transmit buffer onto the network
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
    
    
}


/******************************************************************************
 * Function:        BOOL IsMACSendTx(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Returns true if TX send flag was cleared
 *
 * Side Effects:    None
 *
 * Overview:        Check for Send Tx flag
 *
 * Note:            None
 *****************************************************************************/
//--- made by SKA ---BOOL IsMACSendTx(void)
bool IsMACSendTx(void)
{
    return !((enc28j60Read(ECON1) & ECON1_TXRTS) == ECON1_TXRTS);
}








/******************************************************************************
 * SOCKET APPLICATION LAYER
 *****************************************************************************/

/******************************************************************************
 * Function:        void SOCKETReadBuffer(unsigned char* _buf, unsigned int _size, unsigned int _start)
 *
 * PreCondition:    none
 *
 * Input:           *_buf:  buffer to write data read
 *                  _size:  amount data to read
 *                  _start: socket's buffer start address
 *
 * Output:          None
 *
 * Side Effects:    interfere in ERDPT pointer
 *
 * Overview:        Reads bytes from Socket RX/TX Buffer space
 *
 * Note:            None
 *****************************************************************************/
void SOCKETReadBuffer(unsigned char* _buf, unsigned int _size, unsigned int _start)
{
    // Set the read pointer to the start of the packet kept in the socket buffer
    enc28j60Write(ERDPTL, low(_start));
    enc28j60Write(ERDPTH, high(_start));
    
    //delay(1);
    
    // copy the packet from the socket buffer
    enc28j60ReadBuffer(_size, _buf);
}

/******************************************************************************
 * Function:        void SOCKETWriteBuffer(unsigned char* _buf, unsigned int _size, unsigned int _start)
 *
 * PreCondition:    none
 *
 * Input:           *_buf:  buffer to write data 
 *                  _size:  amount data to write
 *                  _start: socket's buffer start address
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Writes bytes to Socket RX/TX Buffer space
 *
 * Note:            None
 *****************************************************************************/
void SOCKETWriteBuffer(unsigned char* _buf, unsigned int _size, unsigned int _start)
{
    // Set the write pointer to start of socket buffer area
    enc28j60Write(EWRPTL, low(_start));
    enc28j60Write(EWRPTH, high(_start));
    
    // copy the packet into the socket buffer
    enc28j60WriteBuffer(_size, _buf);
    
}

/******************************************************************************
 * Function:        unsigned int SOCKETGetRxPointer() 
 *
 * PreCondition:    none
 *
 * Input:           None
 *
 * Output:          RX pointer location
 *
 * Side Effects:    None
 *
 * Overview:        Retrieves RX pointer location
 *
 * Note:            None
 *****************************************************************************/
unsigned int SOCKETGetRxPointer() 
{
    unsigned int addrTmp;
    addrTmp = enc28j60Read(ERDPTL);
    addrTmp |= enc28j60Read(ERDPTH) << 8;
    return addrTmp;
}

/******************************************************************************
 * Function:        void SOCKETSetRxPointer(unsigned int addr)
 *
 * PreCondition:    none
 *
 * Input:           addr = Address to set RX pointer to.
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Sets RX pointer location
 *
 * Note:            None
 *****************************************************************************/
void SOCKETSetRxPointer(unsigned int addr)
{
    enc28j60Write(ERDPTL, low(addr));
    enc28j60Write(ERDPTH, high(addr));
}

/******************************************************************************
 * Function:        unsigned int SOCKETGetTxPointer() 
 *
 * PreCondition:    none
 *
 * Input:           None
 *
 * Output:          TX pointer location
 *
 * Side Effects:    None
 *
 * Overview:        Retrieves TX pointer location
 *
 * Note:            None
 *****************************************************************************/
unsigned int SOCKETGetTxPointer() 
{
    unsigned int addrTmp;
    addrTmp = enc28j60Read(EWRPTL);
    addrTmp |= enc28j60Read(EWRPTH) << 8;
    return addrTmp;
}

/******************************************************************************
 * Function:        void SOCKETSetTxPointer(unsigned int addr)
 *
 * PreCondition:    none
 *
 * Input:           addr = Address to set TX pointer to.
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Sets TX pointer location
 *
 * Note:            None
 *****************************************************************************/
void SOCKETSetTxPointer(unsigned int addr)
{
    enc28j60Write(EWRPTL, low(addr));
    enc28j60Write(EWRPTH, high(addr));
}









/******************************************************************************
 * DMA COPY LAYER
 *****************************************************************************/

/******************************************************************************
 * Function:        void DMACopy(FLOW flow, unsigned int destAddr, unsigned int len)
 *
 * PreCondition:    SPI bus must be initialized (done in MACInit()).
 *
 * Input:           destAddr:   Destination address in the Ethernet memory to
 *                              copy to.  If (PTR_BASE)-1 is specified, the 
 *								current EWRPT value will be used instead.
 *                  sourceAddr: Source address to read from.  If (PTR_BASE)-1 is
 *                              specified, the current ERDPT value will be used
 *                              instead.
 *                  len:        Number of bytes to copy
 *
 * Output:          None
 *
 * Side Effects:    Moves read and write pointers. DMA conflict with transmitting and receiving
 *
 * Overview:        Bytes are asynchrnously transfered within the buffer.  Call
 *                  MACIsMemCopyDone() to see when the transfer is complete.
 *
 * Note:            If a prior transfer is already in progress prior to
 *                  calling this function, this function will block until it
 *                  can start this transfer.
 *
 *                  If (PTR_BASE)-1 is used for the sourceAddr or destAddr
 *                  parameters, then that pointer will get updated with the
 *                  next address after the read or write.
 *****************************************************************************/
void DMACopy(FLOW flow, unsigned int destAddr, unsigned int len)
{
    // finally using tx socket space, after 2 years developing this enc28's EEPROM approach
    // for documentation sake, I have made following organization
    // to enc28's 8k eeprom
    // 0H ~ 800H (2K) - RX FIFO (CYCLIC)
    // 802H ~ 1000H (2K-1byte) - TX FIFO (FLAT)
    // 1002H ~ 1401H - Socket RX Bank 0 (FLAT)
    // 1402H ~ 1801H - Socket RX Bank 1 (not used yet) (FLAT)
    // 1803H ~ 1C02H - Socket TX Bank 0 (FLAT)
    // 1C03H ~ 1FFEH - Socket TX Bank 1 (not used yet) (FLAT) -- Total 8K EEPROM used ONLY by this library!
    // Errata 7b has lots of issues about this addresses
    // I have made them by the book!
    // By Renato Aloi (May 2015)
    // While trying to remmember all rules to implement last and most important option: Write at TX Socket Buffer!
    unsigned int sourceAddr = RXSTART_INIT; // RX Copy From FIFO To Bank
    
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_CSUMEN);
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_DMAST);
    
    // Defining source address by read or write pointer
    if (flow == RX)
    {
        if ((CurrentPacketLocation.Val + RXD_STATUS_VECTOR_SIZE) <= RXSTOP_INIT)
        {
            sourceAddr = CurrentPacketLocation.Val + RXD_STATUS_VECTOR_SIZE;
        }
        else
        {
            unsigned int rest = (CurrentPacketLocation.Val + RXD_STATUS_VECTOR_SIZE) - RXSTOP_INIT;
            sourceAddr = rest - 1;
        }
    }
    else if (flow == TX)
    {
	sourceAddr = SOCKET_TX_START(0); // TX Copy From Bank To FIFO
    }
    
    // Source Init Address
    enc28j60Write(EDMASTL, low(sourceAddr));
    enc28j60Write(EDMASTH, high(sourceAddr));
    
    // Source Stop Address
    // Need compensate circular buffer WHEN RX COPY !!!
    unsigned int floatingEnd = (sourceAddr + len);
    if (flow == RX)
    {	
	if (floatingEnd >= RXSTOP_INIT)
	{
	    // Adjusting END for DMA copy
	    // otherwise it will never reach end pointer
	    // and never get it done
	    unsigned int tempEnd = (floatingEnd - RXSTOP_INIT);
	    if (tempEnd > 0) tempEnd--;
	    
	    floatingEnd = tempEnd;
	}
    }
    
    // Source Stop Address
    enc28j60Write(EDMANDL, low(floatingEnd));
    enc28j60Write(EDMANDH, high(floatingEnd));
    
    // Destination Init Address
    enc28j60Write(EDMADSTL, low(destAddr));
    enc28j60Write(EDMADSTH, high(destAddr));
    
    // Clear flag
    enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_DMAIF);
    
    // Execute!
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_DMAST);
}


/******************************************************************************
 * Function:        BOOL IsDMACopyDone(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Returns true if DMA flag was cleared
 *
 * Side Effects:    None
 *
 * Overview:        Check for DMA flag
 *
 * Note:            None
 *****************************************************************************/
//--- made by SKA ---BOOL IsDMACopyDone(void)
bool IsDMACopyDone(void)
{
    return !((enc28j60Read(ECON1) & ECON1_DMAST) == ECON1_DMAST);
}






/******************************************************************************
 * HARDWARE LAYER
 *****************************************************************************/



/******************************************************************************
 * Function:        static void enc28j60SetBank(uint8_t address)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs set bank operation
 *
 * Note:            None
 *****************************************************************************/
static void enc28j60SetBank(uint8_t address)
{
        // set the bank (if needed)
        if((address & BANK_MASK) != Enc28j60Bank)
        {
                // set the bank
                enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
                enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
                Enc28j60Bank = (address & BANK_MASK);
        }
}

/******************************************************************************
 * Function:        static void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs write of an operational register normally MAC operations
 *
 * Note:            None
 *****************************************************************************/
static void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
        CSACTIVE;
        // issue write command
/*--- made by SKA ---        SPDR = op | (address & ADDR_MASK);
        waitspi();*/
	SPI.transfer(op | (address & ADDR_MASK));
        // write data
/*--- made by SKA ---        SPDR = data;
        waitspi();*/
	SPI.transfer(data);
        CSPASSIVE;
}

/******************************************************************************
 * Function:        static void enc28j60Write(uint8_t address, uint8_t data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs write of a control register
 *
 * Note:            None
 *****************************************************************************/
static void enc28j60Write(uint8_t address, uint8_t data)
{
        // set the bank
        enc28j60SetBank(address);
        // do the write
        enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

/******************************************************************************
 * Function:        static void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None	
 *
 * Overview:        Performs write buffer memory
 *
 * Note:            None
 *****************************************************************************/
static void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
    CSACTIVE;
    // issue write command
/*--- made by SKA ---    SPDR = ENC28J60_WRITE_BUF_MEM;
    waitspi();*/
    SPI.transfer(ENC28J60_WRITE_BUF_MEM);
    while(len)
    {
        len--;
        // read data
/*--- made by SKA ---        SPDR = *data;
        waitspi();*/
	SPI.transfer((byte)* data);
        if (len) data++;
    }
    // release CS
    CSPASSIVE;
}

/******************************************************************************
 * Function:        static uint8_t enc28j60ReadOp(uint8_t address)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs read of a control register
 *
 * Note:            None
 *****************************************************************************/
static uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
        CSACTIVE;
        // issue read command
/*--- made by SKA ---        SPDR = op | (address & ADDR_MASK);
        waitspi();*/
	SPI.transfer(op | (address & ADDR_MASK));
        // read data
/*--- made by SKA ---        SPDR = 0x00;
        waitspi();*/
	SPI.transfer(0x00);
        // do dummy read if needed (for mac and mii, see datasheet page 29)
        if(address & 0x80)
        {
/*--- made by SKA ---            SPDR = 0x00;
            waitspi();*/
	    SPI.transfer(0x00);
        }
        // release CS
        CSPASSIVE;
        return(SPDR);
}

/******************************************************************************
 * Function:        static uint8_t enc28j60Read(uint8_t address)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs read of a control register
 *
 * Note:            None
 *****************************************************************************/
static uint8_t enc28j60Read(uint8_t address)
{
        // set the bank
        enc28j60SetBank(address);
        // do the read
        return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

/******************************************************************************
 * Function:        static void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs read buffer memory
 *
 * Note:            None
 *****************************************************************************/
static void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
        CSACTIVE;
        // issue read command
/*--- made by SKA ---        SPDR = ENC28J60_READ_BUF_MEM;
        waitspi();*/
	SPI.transfer(ENC28J60_READ_BUF_MEM);
        while(len)
        {
                len--;
                // read data
/*--- made by SKA ---                SPDR = 0x00;
                waitspi();
                *data = SPDR;*/
                *data = SPI.transfer(0x00);
                data++;
        }
        //*data='\0';
        CSPASSIVE;
}

/******************************************************************************
 * Function:        static void enc28j60PhyWrite(uint8_t address, uint16_t data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        Performs write for PHY registers
 *
 * Note:            None
 *****************************************************************************/
static void enc28j60PhyWrite(uint8_t address, uint16_t data)
{
        // set the PHY register address
        enc28j60Write(MIREGADR, address);
        // write the PHY data
        enc28j60Write(MIWRL, low(data));
        enc28j60Write(MIWRH, high(data));
        // wait until the PHY write completes
        while(enc28j60Read(MISTAT) & MISTAT_BUSY)
        {
                //delayMicroseconds(15);
                delay(1);
        }
}





