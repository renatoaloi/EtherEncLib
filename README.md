EtherEncLib
===========

Ethernet ENC28J60 Library for Arduino

REV 3 - New version! Now using Enc28CoreLib! For now, see new version at enc28coremerge branch!
https://github.com/renatoaloi/EtherEncLib/tree/enc28coremerge
New Commit with HTTP POST implemented! I will update comments and docs later! 

EtherEncLib is a new implementation for ENC28J60 Stand-Alone Ethernet Controller with SPI Interface. 
Original implementation for this code was made by Guido Socher based on enc28j60.c file from the 
AVRLib library by Pascal Stang; and finally modified by Renato Aloi.

Unlike older implementations, EtherEncLib library utilizes a diferent approach, 
sending more than one packet per request. This changes everything!

The problem I'v got using older implementations is that my HTML is a larger sum of text!
It got Javascript, CSS, tons of tags and commands. It didn't fit in that tiny 512 bytes 
buffer the library had. And this buffer is sent once per request.

EtherEncLib sends a small buffer plenty of times per request. Keep in mind we need to deliver
an elephant through a lock door hole, kepping RAM consumption at low rates. So I did tear apart
my large HTML into small chunks through TCP-IP packets.

See the real example HtmlButtonAllDigitalPortsAvailable.ino, where all Arduino UNO available ports
are mapped to HTML buttons.

Renato Aloi, August 2013
renato.aloi@gmail.com
http://seriallink.com.br
