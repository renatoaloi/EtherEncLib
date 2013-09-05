/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Renato Aloi - 2013 August
 * Copyright: GPL V2
 *
 * Based on the net.h file from the AVRlib library by Pascal Stang.
 * For AVRlib See http://www.procyonengineering.com/
 * Used with explicit permission of Pascal Stang.
 *
 * Chip type           : ATMEGA88 with ENC28J60
 *********************************************/

#include "enc28j60.h"

#ifndef SOCKET_H
#define SOCKET_H

void          socketInit                (unsigned char *macaddr);
void          socketPacketSend          (unsigned int  len,     unsigned char *packet);
unsigned int  socketPacketReceive       (unsigned int  maxlen,  unsigned char *packet);
unsigned char socketGetHardwareRevision ();
void          changeHardwareClockout    ();

#endif