EtherEncLib
===========

**Ethernet ENC28J60 Library for Arduino**

- New branch for UDP, DHCP, DNS, WOL implementations!
- New REV 3.1! (Already commited to master branch!)
- New code by Renato Aloi, based on Howard Schlundler AN833's TCPStack work, from Microchip plataform. This new version is taking total advantage over ENC28J60's internal 8K Dual RAM!
- NO MORE string buffers consuming Arduino RAM!

June 2015 - Branch newMods
- Implemented UDP Stack for Homeseer integration (already working at Normal Mode)

May 2015
REV 3.1 
- SKA made new code for POST treatment
- New example with SDCard made by SKA
- ENC28J60' 8K Dual RAM final implementation by Renato Aloi
- New example for HelloWorld with low RAM usage 
- New example based on SKA work for WebServer

REV 3 - New version! Now using Enc28CoreLib! For now, see new version at enc28coremerge branch!
https://github.com/renatoaloi/EtherEncLib/tree/enc28coremerge
New Commit with HTTP POST implemented! I will update comments and docs later! 

REV 2 - EtherEncLib is a new implementation for ENC28J60 Stand-Alone Ethernet Controller with SPI Interface. 
Original implementation for this code was made by Guido Socher based on enc28j60.c file from the 
AVRLib library by Pascal Stang; and finally modified by Renato Aloi.

**Documentation @ Wiki**

- [Schematics](https://github.com/renatoaloi/EtherEncLib/wiki/Schematics)
- [Changelog](https://github.com/renatoaloi/EtherEncLib/wiki/Revisions)
- [Story](https://github.com/renatoaloi/EtherEncLib/wiki)
- [Memory Organization](https://github.com/renatoaloi/EtherEncLib/wiki#enc28j60-memory-usage)

Thanks all for contribuing, special thanks to SKA for his POST treatment and SPI workout!

by Renato Aloi

May 2015

[seriallink.com.br](http://www.seriallink.com.br)
