EtherEncLib
===========

Ethernet ENC28J60 Library for Arduino

EtherEncLib is a new implementation for ENC28J60 Stand-Alone Ethernet Controller with SPI Interface. 
Original implementation for this code was made by Guido Socher based on enc28j60.c file from the 
AVRLib library by Pascal Stang; and finally modified by Renato Aloi.

Unlike older implementations, EtherEncLib library utilizes a diferent approach, 
sending more than one packet per request. This changes everything!

The problem I'v got using older implementations is that my HTML is a larger sum of text!
It got Javascript, CSS, tons of tags and commands. It Didn't fit in in that tiny 512 byte 
buffer the library had. And this buffer is sent once per request.

EtherEncLib sends a small buffer plenty of times per request. Keep in mind we need to deliver
an elephant through a lock door hole, kepping RAM consumption at low rates. So I did tear apart
my large HTML into small chunks through TCP-IP packets.

See the real example HtmlButtonAllDigitalPortsAvailable.ino, where all Arduino UNO available ports
are mapped to HTML buttons.

Renato Aloi, August 2013
renato.aloi@gmail.com
seriallink.com.br
