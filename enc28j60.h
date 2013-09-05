/*****************************************************************************
* vim:sw=8:ts=8:si:et
*
* Title         : Microchip ENC28J60 Ethernet Interface Driver
* Author        : Pascal Stang (c)2005
* Modified by Guido Socher
* Modified again by Renato Aloi - 2013 August
* Copyright: GPL V2
*
* This driver provides initialization and transmit/receive
* functions for the Microchip ENC28J60 10Mb Ethernet Controller and PHY.
* This chip is novel in that it is a full MAC+PHY interface all in a 28-pin
* chip, using an SPI interface to the host processor.
*
*
*****************************************************************************/
/*********************************************
 * Modified: nuelectronics.com -- Ethershield for Arduino
 *********************************************/
//@{


#ifndef ENC28J60_H
#define ENC28J60_H

#include <inttypes.h>
#include "enctypes.h"



// functions
extern uint8_t  enc28j60ReadOp        (uint8_t  op,      uint8_t address                  );
extern void     enc28j60WriteOp       (uint8_t  op,      uint8_t address,     uint8_t data);
extern void     enc28j60ReadBuffer    (uint16_t len,     uint8_t* data                    );
extern void     enc28j60WriteBuffer   (uint16_t len,     uint8_t* data                    );
extern void     enc28j60SetBank       (uint8_t  address                                   );
extern uint8_t  enc28j60Read          (uint8_t  address                                   );
extern void     enc28j60Write         (uint8_t  address, uint8_t data                     );
extern void     enc28j60PhyWrite      (uint8_t  address, uint16_t data                    );
extern void     enc28j60clkout        (uint8_t  clk                                       );
extern uint8_t  enc28j60getrev        (void                                               );

#endif
//@}
