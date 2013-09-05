
/*
  EtherEncLib.cpp - Library for Ethernet ENC29J60 Module.
  Created by Renato Aloi, August 27, 2013.
  Released into the public domain.
  
  Description ::
  HTTP 1.0/1.1 GET Listener for web browsing integration.
  
  What DO you get ::
  - ARP and ICMP treatment
  - HTTP GET action over TCP-IP 
  - Very responsive engine, and very fast!
  - HTTPd engine with "One Way" implementation
    - One way flow = SYS..ACK..PUSH..FIN
  - Possible to send various PUSH statments trhough print() function
    - SYS..ACK..PUSH1..PUSH2..PUSH3..PUSHn..FIN
  - Avoid the limit imposed by others ENC28J60 libraries
    - Other libs sends all data in one TCP PUSH packet, need large buffer
    - EtherEncLib.h sends data in little chunks, NO need large buffer
  - available(), print() and close() functions makes easy
      coding and "thinking outside the box"
  - getParams() extracts the command line parameters ex:
      http://192.168.1.25/?param1=0.231&param2=0.34
      will result: ?param1=0.231&param2=0.34
  - Timered loops to avoid get stucked waiting a client response
  
  
  What do you NOT get ::
  - This is a very limited implementation of TCP-IP protocol
    - Do not expect implementations like FTP, SMTP, etc
    - Only one aspect of TCP/HTTP service was implemented
  - On HTTP protocol, only GET form method was implemented
    - POST method is NOT implemented
    - POST method is ready to go, having already commented spots to code 
  - Faulty handle for diferent Source ports
    - The system will only handle one source port incomming connection
    - Other request arriving from other ports will be handled only at end of current request
    - Only FIN+ACK arriving from other ports will be handled
  - UDP NOT implemented
  
*/

#include <Arduino.h>
#include "EtherEncLib.h"

#include "net.h"

extern "C"
{
    #include "socket.h"
    #include "checksum.h"
}

// --------------------------------------------------------------------------------------------------------------------------------------------
//
// Public Methods
//
// --------------------------------------------------------------------------------------------------------------------------------------------


// Constructor
EtherEncLib::EtherEncLib(unsigned int port)
{
    // Loading initial values
    _sessionLock             = 0;
    _sessionPortSrc          = 0;
    _port                    = port;
    
    recebeuRequestFlag       = 0;
    tratamentoGetPostFlag    = 0;
    tratamentoPrintFlag      = 0;
    tratamentoCloseFlag      = 0;
    tratamentoCloseAckFlag   = 0;
    
    tratamentoGetPostCounter = 0;
    
    // I/O buffers init
    // and request params
    initBuffer();
    
    // Ip array init
    initIP();
    
    // Mac array init
    initMAC();
}

// Begin method
// Configures the lib and load initial params
void EtherEncLib::begin(unsigned char *ip, unsigned char *mac)
{
    // Loading IP and MAC addresses
    loadIP(ip);
    loadMAC(mac);
    
    // Socket and Eth layer init
    socketInit(_mac);
    
    // Changing hardware clock
    changeHardwareClockout();
    
    if (DEBUG) { Serial.print("Hardware Rev.: "); Serial.println(socketGetHardwareRevision()); Serial.println("Software Rev.: 2"); }
}

// Available method
// Returns true when initial handshake is done
unsigned char EtherEncLib::available()
{
    // Checking for SYN packets for us
    recebeuRequestFlag = 1;
    if (!isBusy())
    {
        // Session init
        initSessao();
        
        // Return callback, ruled by time
        // receive and handle HTTP GET/POST
        tratamentoGetPostFlag = 1;
        if (isReturnOk(TEMPO_GET_POST))
        {
            return 1;
        }
        
        // If gets here, means
        // return NOT ok, clear session
        resetSession();
    }
    
    
    return 0;
}

// Print method
// Sends data to client in small chunks
void EtherEncLib::print(char *rd)
{
    // String length calc.
    unsigned int l = getPrintStringLen(rd);
    
    // Checking if total len is greather
    // than max chunk length
    if (l > MAX_CHUNK_LEN)
    {
        // In this case, slice it up
        // Calculating total chunks and additional
        // "left overs" chunk
        unsigned char divInt  = l / MAX_CHUNK_LEN;
        unsigned char divRest = l % MAX_CHUNK_LEN;
        
        if (DEBUG) { Serial.print("Rodando: "); Serial.print(divInt, DEC); Serial.print(" - "); Serial.print(divRest, DEC); Serial.print(" :: de: "); Serial.println(l, DEC); }
        
        // Slicing role message in chunks
        for (int i = 0; i < divInt; i++)
        {
            print(&rd[i * MAX_CHUNK_LEN], MAX_CHUNK_LEN);
        }
        
        // One last chunk to send "left overs"
        print(&rd[divInt * MAX_CHUNK_LEN], divRest);
    }
    else
    {
        // If total length is smaller than
        // max chunk size, no need slicing
        print(rd, l);
    }
}

// Print method
// Int values version of print method
void EtherEncLib::print(unsigned int val)
{
    char sI[] = { 0, 0, 0, 0, 0 };
    itoa(val, sI, 10);
    print(sI);
}

// Close method
// Self-explaining
void EtherEncLib::close()
{
    // Sending FIN+ACK
    sendClose();
    
    // Callback for ACK returning from FIN+ACK
    tratamentoCloseFlag = 1;
    if (isReturnOk(TEMPO_FIN_ACK))
    {
        // Sends ACK for FIN+ACK Part 1
        sendCloseAck2();
        
        // Callback for ACK returning from FIN+ACK
        tratamentoCloseAckFlag = 1;
        if (isReturnOk(TEMPO_FIN_ACK))
        {
            // Sends ACK for FIN+ACK Part 2
            sendCloseAck2();
        }
    }
    
    // reset session
    resetSession();
}

// GetParams
// Returns char array containing string of URL parameters, including character "?"
// like: ?param1=512&param2=xyz
char *EtherEncLib::getParams()
{
    return &_bufferPr[0];
}


// --------------------------------------------------------------------------------------------------------------------------------------------
//
// Priv. Funcs
//
// --------------------------------------------------------------------------------------------------------------------------------------------



//
// Inits
//
void EtherEncLib::initIP()
{
    for (int i = 0; i < IPLEN; i++)
    {
        _ip[i] = 0;
    }
}

void EtherEncLib::initMAC()
{
    for (int i = 0; i < MACLEN; i++)
    {
        _mac[i] = 0;
    }
}

void EtherEncLib::initBuffer()
{
    initBufferRead();
    initBufferWrite();
    initBufferParams();
}

void EtherEncLib::initBufferRead()
{
    for (int i = 0; i < BUFFER_READ_LEN; i++)
    {
        _bufferRd[i] = 0;
    }
}

void EtherEncLib::initBufferWrite()
{
    for (int i = 0; i < BUFFER_WRITE_LEN; i++)
    {
        _bufferWr[i] = 0;
    }
}

void EtherEncLib::initBufferParams()
{
    for (int i = 0; i < BUFFER_PARAMS_LEN; i++)
    {
        _bufferPr[i] = 0;
    }
}

void EtherEncLib::initBufferWriteData()
{
    for (int i = TCP_DATA_P; i < BUFFER_WRITE_LEN; i++)
    {
        _bufferWr[i] = 0;
    }
}

void EtherEncLib::initSessao()
{
    // Performs a session lock
    // and gets the source port #
    _sessionLock    = 1;
    _sessionPortSrc = getTcpSrcPortH();
    _sessionPortSrc = (_sessionPortSrc << 8) | getTcpSrcPortL();
    
    // Reset Get-Post counter
    tratamentoGetPostCounter = 0;
    
    if (DEBUG) { Serial.print("_sessionPortSrc = "); Serial.println(_sessionPortSrc, DEC); }
    
    // Handling requests from source ports outside the session
    // TODO: TIMER !!!!!!!
}



//
// Loads
//
void EtherEncLib::loadIP(unsigned char *address)
{
    for (int i = 0; i < IPLEN; i++)
    {
        _ip[i] = address[i];
    }
}

void EtherEncLib::loadMAC(unsigned char *address)
{
    for (int i = 0; i < MACLEN; i++)
    {
        _mac[i] = address[i];
    }
}

void EtherEncLib::loadBuffer()
{
    // Buffers should be same size,
    // but it is possible read buffer
    // greater than write buffer
    // (otherwise is not recomended!)
    for (int i = 0; i < BUFFER_WRITE_LEN; i++)
    {
        if (i < BUFFER_READ_LEN)
        {
            _bufferWr[i] = _bufferRd[i];
        }
    }
}

void EtherEncLib::loadBufferParams()
{
    int  i = 0;
    char c = getTcpRequestData(i);
    if (c == '?')
    {
        while(c != ' ')
        {
            if (i < BUFFER_PARAMS_LEN - 1)
            {
                _bufferPr[i] = c;
            }
            else if (i < BUFFER_PARAMS_LEN)
            {
                _bufferPr[i] = '\0';
            }
            i++;
            c = getTcpRequestData(i);
        }
    }
}





//
// Is'
//
unsigned char EtherEncLib::isBufferRead()
{
    unsigned char ret = 0;
    for (int i = 0; i < BUFFER_READ_LEN; i++)
    {
        if (_bufferRd[i])
        {
            ret = 1;
            break;
        }
    }
    return ret;
}

unsigned char EtherEncLib::isMyIp()
{
    for (int i = 0; i < IPLEN; i++)
    {
        if (!isMyIpByte(i)) return 0;
    }
    return 1;
}

unsigned char EtherEncLib::isMyIpHeader()
{
    for (int i = 0; i < IPLEN; i++)
    {
        if (!isMyIpHeaderByte(i)) return 0;
    }
    return 1;
}

unsigned char EtherEncLib::isMyMac()
{
    for (int i = 0; i < MACLEN; i++)
    {
        //if (!isMyMacByte(i)) return 0;
    }
    return 1;
}



//
// Makes
//
void EtherEncLib::makeEthHeader()
{
    // Load read's buffer content into write's buffer
    loadBuffer();
    
    // Swap MAC addresses between Source and Destination
    makeEthHeaderWithoutLoadBuffer();
}

void EtherEncLib::makeEthHeaderWithoutLoadBuffer()
{
    for (int i = 0; i < MACLEN; i++)
    {
        fillDstMac(i);
        fillSrcMac(i);
    }
}

void EtherEncLib::makeIpHeader()
{
    for (int i = 0; i < IPLEN; i++)
    {
        fillDstIp(i);
        fillSrcIp(i);
    }
    fillChecksum(_bufferWr);
}

void EtherEncLib::makeTcpPortsInverter()
{
    for (int i = 0; i < PORTLEN; i++)
    {
        // Copy source port into port destination address
        fillTcpDstPort();
        // Clear source port
        clearTcpSrcPort();
    }
    // Config source port
    fillTcpSrcPort();
}

void EtherEncLib::makeTcpHeaderSyn() 
{
    // Swapping source and destination ports
    makeTcpPortsInverter();
    
    // Resolving ACK and SEQ
    formatTcpSeqAck();
    
    // Cleaning Chechsum
    clearTcpChecksumH();
    clearTcpChecksumL();
    
    // Adding MMS Option
    fillTcpMmsOptHH();
    fillTcpMmsOptHL();
    fillTcpMmsOptLH();
    fillTcpMmsOptLL();
    
    // Configuring Tcp Header Len
    // with options
    fillTcpHeaderLenWithOpt();
}

void EtherEncLib::makeTcpHeaderPush(unsigned int _payload) 
{
    // Swapping source and destination ports
    makeTcpPortsInverter();
    
    // Resolving ACK and SEQ considering payload
    formatTcpSeqAckPayload(_payload);
    
    // Cleaning Chechsum
    clearTcpChecksumH();
    clearTcpChecksumL();
    
    // Configuring Tcp Header Len
    fillTcpHeaderLen();
}



//
// Formats
//
void EtherEncLib::formatTcpSeqAck()
{
    formatTcpSeqAckPayload(1);
}

void EtherEncLib::formatTcpSeqAckPayload(unsigned int payload)
{
    // Check if ACK # from read's buffer is zero
    if (isAckZero())
    {
        // If yes
    
        // Fills SEQ # from write's buffer
        // with a initial value
        fillTcpSeqInitHH();
        fillTcpSeqInitHL();
        fillTcpSeqInitLH();
        fillTcpSeqInitLL();
    }
    else
    {
        // If no
        
        // Fills SEQ # from write's buffer
        // with value of ACK # from read's buffer
        fillTcpSeqAckHH();
        fillTcpSeqAckHL();
        fillTcpSeqAckLH();
        fillTcpSeqAckLL();
        
    }
    
    // Incrementa SEQ Num do buffer de read
    unsigned long ackPlus1 = 0;
    for (int i = 0; i < SEQNUMLEN; i++)
    {
        //if (DEBUG) { Serial.print("AckPlus1["); Serial.print(i); Serial.print("]: "); Serial.println(((getTcpReadSeq(i)) & 0xFF), DEC); }
        
        // Alimenta unsigned long ackPlus1 com os 4 bytes
        // do SEQ Num no buffer Read
        if (!i) ackPlus1 = ((getTcpReadSeq(i)) & 0xFF);
        else ackPlus1 = (ackPlus1 << 8) | ((getTcpReadSeq(i)) & 0xFF);
    }
    // Incrementa ackPlus1
    // Request SEQ + 1 = Response ACK (quando NAO HA dados)
    // Request SEQ + payload = Response ACK (quando HA dados)
    ackPlus1 += payload;
    
    //if (DEBUG) { Serial.print("AckPlus1: "); Serial.println(ackPlus1); }
    
    // Alimenta o ACK do buffer de Write
    // Adiciona o ackPlus1 no Ack do buffer Write
    for (int i = 0; i < SEQNUMLEN; i++)
    {
        fillTcpWriteAck(SEQNUMLEN - i - 1, ackPlus1 >> (i * 8));
    }
    
    
}

void EtherEncLib::formatArpResponse()
{
    // Builds Eth header
    makeEthHeader();
    
    // Fill OPTCODE with response value
    fillArpOptCodeH();
    fillArpOptCodeL();
    
    // Fill ARP Mac address
    for (int i = 0; i < MACLEN; i++)
    {
        fillArpDstMac(i);
        fillArpSrcMac(i);
    }
    
    // Fill ARP Ip address
    for (int i = 0; i < IPLEN; i++)
    {
        fillArpDstIp(i);
        fillArpSrcIp(i);
    }
}

void EtherEncLib::formatIcmpResponse()
{
    // Building Header Eth
    makeEthHeader();
    
    // Building Header Ip
    makeIpHeader();
    
    // Fill return type field
    // Changes from request=8 to reply=0
    fillIcmpTypeReply();
    
    
    // Handling checksum
    if (isChecksumIcmp())
    {
        checksumIcmpIncrement();
    }
    checksumIcmpIncBy8();  

}

void EtherEncLib::formatTcpSynResponse()
{
    // Building Header Eth
    makeEthHeader();
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 24 bytes (20tcp+4tcp options)
    fillIpTotalLenWithOptH();
    fillIpTotalLenWithOptL();
        
    // Building Header Ip
    makeIpHeader();
    
    // Fill FLAG TCP with SYN+ACK
    fillTcpFlagSynAck();
    
    // Config Tcp Header
    makeTcpHeaderSyn();
 
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + 4 (one option: mss)
    unsigned int ck = checksum(&_bufferWr[IP_SRC_P], CHECKSUMLEN + TCP_HEADER_LEN_PLAIN_V + TCPHEADEROPTLEN, CHECKSUMTCPID);
    
    //if (DEBUG) { Serial.print("Checksum: "); Serial.println(ck, DEC); }
    
    // Fill new checksum
    // into write's buffer
    fillTcpChecksumH(ck >> 0x08);
    fillTcpChecksumL(ck  & 0xFF);
}

void EtherEncLib::formatAckPushResponse(unsigned char _lenData)
{
    // Getting payload from push sent
    unsigned int payload = getTcpPayload();
    
    if (DEBUG) { Serial.print("Payload H Bit: "); Serial.println(getIpTotalLenH(), HEX); }
    if (DEBUG) { Serial.print("Payload L Bit: "); Serial.println(getIpTotalLenL(), HEX); }
    
    // Building Header Eth
    makeEthHeaderWithoutLoadBuffer();
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp + data len
    fillIpTotalLenH();
    fillIpTotalLenL(_lenData);
    
    // Building Header Ip
    makeIpHeader();
    
    // Fill FLAG TCP with PUSH+ACK
    fillTcpFlagPshAck();
    
    if (DEBUG) { Serial.print("payload: "); Serial.println(payload, DEC); }
    
    // Config Tcp Header
    makeTcpHeaderPush(payload);
 
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    unsigned int ck = checksum(&_bufferWr[IP_SRC_P], CHECKSUMLEN + TCP_HEADER_LEN_PLAIN_V + _lenData, CHECKSUMTCPID);
    
    //if (DEBUG) { Serial.print("Checksum: "); Serial.println(ck, DEC); }
    
    // Fill new checksum
    // into write's buffer
    fillTcpChecksumH(ck >> 0x08);
    fillTcpChecksumL(ck  & 0xFF);
}

void EtherEncLib::formatAckPushPrint(unsigned char _lenData)
{
    // Building Header Eth
    makeEthHeaderWithoutLoadBuffer();
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp 
    fillIpTotalLenH();
    fillIpTotalLenL(_lenData);
    
    // Building Header Ip
    makeIpHeader();
    
    // Fill FLAG TCP with PUSH+ACK
    fillTcpFlagPshAck();
    
    // Building Tcp Header
    makeTcpHeaderPush(0);
 
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    unsigned int ck = checksum(&_bufferWr[IP_SRC_P], CHECKSUMLEN + TCP_HEADER_LEN_PLAIN_V + _lenData, CHECKSUMTCPID);
    
    //if (DEBUG) { Serial.print("Checksum: "); Serial.println(ck, DEC); }
    
    // Fill new checksum
    // into write's buffer
    fillTcpChecksumH(ck >> 0x08);
    fillTcpChecksumL(ck  & 0xFF);
}

void EtherEncLib::formatAckPushClose()
{
    
    // Building Header Eth
    makeEthHeader();
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp 
    fillIpTotalLenH();
    fillIpTotalLenL(0);
    
    // Building Header Ip
    makeIpHeader();
    
    // Fill FLAG TCP with PUSH+ACK
    fillTcpFlagFinAck();
    
    // Building Tcp Header
    makeTcpHeaderPush(0);
 
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    unsigned int ck = checksum(&_bufferWr[IP_SRC_P], CHECKSUMLEN + TCP_HEADER_LEN_PLAIN_V + 0, CHECKSUMTCPID);
    
    //if (DEBUG) { Serial.print("Checksum: "); Serial.println(ck, DEC); }
    
    // Fill new checksum
    // into write's buffer
    fillTcpChecksumH(ck >> 0x08);
    fillTcpChecksumL(ck  & 0xFF);
    
}

void EtherEncLib::formatAckPushCloseAck()
{
    
    // Building Header Eth
    makeEthHeader();
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp 
    fillIpTotalLenH();
    fillIpTotalLenL(0);
    
    // Building Header Ip
    makeIpHeader();
    
    // Fill FLAG TCP with PUSH+ACK
    fillTcpFlagAck();
    
    // Building Tcp Header
    makeTcpHeaderPush(0);
 
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    unsigned int ck = checksum(&_bufferWr[IP_SRC_P], CHECKSUMLEN + TCP_HEADER_LEN_PLAIN_V + 0, CHECKSUMTCPID);
    
    //if (DEBUG) { Serial.print("Checksum: "); Serial.println(ck, DEC); }
    
    // Fill new checksum
    // into write's buffer
    fillTcpChecksumH(ck >> 0x08);
    fillTcpChecksumL(ck  & 0xFF);
    
}

void EtherEncLib::formatAckPushCloseAck2()
{
    
    // Building Header Eth
    makeEthHeader();
    
    // total length field in the IP header must be set:
    // 20 bytes IP + 20 bytes tcp 
    fillIpTotalLenH();
    fillIpTotalLenL(0);
    
    // Building Header Ip
    makeIpHeader();
    
    // Fill FLAG TCP with PUSH+ACK
    fillTcpFlagAck();
    
    // Building Tcp Header
    makeTcpHeaderPush(1);
 
    // calculate the checksum, len=8 (start from ip.src) + TCP_HEADER_LEN_PLAIN + data len
    unsigned int ck = checksum(&_bufferWr[IP_SRC_P], CHECKSUMLEN + TCP_HEADER_LEN_PLAIN_V + 0, CHECKSUMTCPID);
    
    //if (DEBUG) { Serial.print("Checksum: "); Serial.println(ck, DEC); }
    
    // Fill new checksum
    // into write's buffer
    fillTcpChecksumH(ck >> 0x08);
    fillTcpChecksumL(ck  & 0xFF);
    
}




//
// Eth layer
//
void EtherEncLib::lerPacoteEth()
{
    // Check for incomming data
    // Fill read's buffer
    if (!socketPacketReceive(BUFFER_READ_LEN, _bufferRd))
    {
        // If has not arrived nothing
        // clear buffer
        initBufferRead();
    }
    else
    {
        if (DEBUG) Serial.println("Package received!");
        
        // Checking package type
        if (isArpPacket())
        {
            if (DEBUG) Serial.println("ARP Package");
            
            // Checking if ARP packet has my IP address
            if (isMyIp())
            {
                if (DEBUG) Serial.println("My IP!");
                
                // Send response for ARP packet
                formatArpResponse();
                socketPacketSend(ETH_ARP_LEN, _bufferWr);
                
                if (DEBUG) Serial.println("ARP package handled!");
            }
            else
            {
                if (DEBUG) Serial.println("IP differs from mine!");
            }
        }
        // Checking if IP packet
        else if (isIpPacket())
        {
            if (DEBUG) Serial.println("IP Packet!");
            
            // Checking for IPV4 and 20 bytes header
            if (isIpHeaderLenAndV4())
            {
                if (DEBUG) Serial.println("IPV4 OK! Header Len OK!");
                
                // Checking if ARP packet has my IP address
                if (isMyIpHeader())
                {
                    if (DEBUG) Serial.println("My IP Header!");
                    
                    // Checking for ICMP packet
                    if (isIcmpPacket())
                    {
                        if (DEBUG) Serial.println("ICMP packet arrived!");
                        
                        // Anwser ICMP packet
                        formatIcmpResponse();
                        socketPacketSend(BUFFER_WRITE_LEN , _bufferWr);
                        
                        if (DEBUG) Serial.println("ICMP package handled!");
                    }
                }
                else
                {
                    if (DEBUG) Serial.println("IP Header differs from mine!");
                }
            }
            else
            {
                if (DEBUG) Serial.println("IPV4 or Header Len ERROR!");
            }
        }
        else
        {
            if (DEBUG) Serial.println("Unknow packet!");
        }
    }

}

unsigned char EtherEncLib::isBusy()
{
    // Reads data from Ethernet
    // and fills the read's buffer
    // Obs1: Treatment for ARP and ICMP automatically
    // Obs2: In IP Packet, check for IPV4, Header Len e IP address
    lerPacoteEth();
    
    // If bufferRd has data
    if (isBufferRead())
    {
        // DEBUG
        //if (DEBUG) { Serial.print("_bufferRd: "); for (int i = 0; i < BUFFER_READ_LEN; i++) { Serial.print(_bufferRd[i], HEX); Serial.print(" "); } Serial.println(); }
        
        // Checking for TCP packet and
        // if destination port is our port
        if (isTcpPacket() && isTcpMyPort())
        {
            if (DEBUG) { Serial.print("Pacote TCP chegou! "); Serial.print("Src Port: "); Serial.print(getTcpSrcPort(), DEC);  Serial.print(" || Dst Port: "); Serial.println(getTcpDstPort(), DEC); }
            
            // Check for TCP flags
            if (isTcpFlagRst())
            {
                if (DEBUG) Serial.println("FLAG TCP RST!");
                
            }
            else if (isTcpFlagRstAck())
            {
                if (DEBUG) Serial.println("FLAG TCP RST+ACK!");
                
            }
            else if (isTcpFlagFin())
            {
                if (DEBUG) Serial.println("FLAG TCP FIN!");
                
            }
            else if (isTcpFlagFinAck())
            {
                if (DEBUG) Serial.println("FLAG TCP FIN+ACK!");
                
                if (tratamentoCloseAckFlag) // && getTcpSrcPort() == _sessionPortSrc)
                {
                    if (DEBUG) Serial.println("FIN+ACK com tratamento!");
                    
                    // reset flag
                    tratamentoCloseAckFlag = 0;
                    
                    // reset session
                    resetSession();
                    
                    // bail out
                    return 0;
                }
                else
                {
                    if (DEBUG) Serial.println("FIN+ACK sem tratamento!");
                    
                    // Sends ACK for FIN+ACK Part 2
                    sendCloseAck2();
                    
                    // reset session
                    resetSession();
                }
                
                /*
                if (getTcpSrcPort() != _sessionPortSrc)
                {
                    if (DEBUG) { Serial.print("Fechando porta: "); Serial.println(getTcpSrcPort(), DEC); }
                    
                    // Callback for ACK returning from FIN+ACK
                    tratamentoCloseAckFlag = 1;
                    if (isReturnOk(TEMPO_FIN_ACK))
                    {
                        // Sends ACK for FIN+ACK Part 2
                        sendCloseAck2();
                    }
                }
                */
            }
            else if (isTcpFlagSyn())
            {
                if (DEBUG) Serial.println("FLAG TCP SYN!");
                
                if (recebeuRequestFlag && !_sessionLock)
                {
                    // reset flag
                    recebeuRequestFlag = 0;
                    
                    // Answering Syn
                    formatTcpSynResponse();
                    
                    // Send Syn anwser
                    socketPacketSend(IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + TCPHEADEROPTLEN + ETH_HEADER_LEN_V, _bufferWr);
                    
                    // at this point, request is confirmed
                    // return false to trigger POST and GET handling
                    return 0;
                }
                
            }
            else if (isTcpFlagSynAck())
            {
                if (DEBUG) Serial.println("FLAG TCP SYN+ACK!");
            }
            else if (isTcpFlagPsh())
            {
                if (DEBUG) Serial.println("FLAG TCP PUSH!");
            }
            else if (isTcpFlagPshAck())
            {
                if (DEBUG) Serial.println("FLAG TCP PUSH+ACK!");
                
                if(tratamentoGetPostFlag && getTcpSrcPort() == _sessionPortSrc)
                {
                    
                    // Verificando o metodo do form action
                    // Protocolos aceitos: GET & POST
                    // TODO: Implementar metodo POST
                    if (strncmp(getTcpRequestAction(), "GET", 3) == 0)
                    {
                        // Tratando protocolo GET
                        // Alimentando buffer dos parametros de URL
                        // Formato: http://url/?param1=x&param2=y&paramN=z
                        // Onde:    url     -> endereco IP ou apelido de Dns
                        //          /       -> indicador de "raiz"
                        //          ?       -> inicio dos parametros
                        //          paramN  -> nome do parametro
                        //          x, y, z -> valores dos parametros
                        //          &       -> separador dos parametros
                        //          =       -> separador de "chave/valor" do parametro
                        
                        if (DEBUG) Serial.println("Protocolo GET!");
                        
                        // Check for parameters
                        if ((char)getTcpRequestData(0) == '?')
                        {
                            // There are parameters!
                            if (DEBUG) { Serial.println("Parametros chegaram!"); }
                            
                            // Load parameters string
                            // Start by ? 
                            // Split by &
                            loadBufferParams();
                            
                            //if (DEBUG) { Serial.print("2 - GET Params: "); Serial.println((char*)_bufferPr); }
                        }
                        else
                        {
                            // No parameters
                            // Usually means initial load (first time load)
                            if (DEBUG) { Serial.println("Nenhum parametro!"); }
                        }
                        
                        // Sending first part of
                        // Http Header
                        sendHttpHeader();
                        
                        // Further Http header parts at ACK treatment...
                    }
                    else if (strncmp(getTcpRequestAction(), "POST", 4) == 0)
                    {
                        // POST not implemented yet
                        if (DEBUG) Serial.println("Protocolo Post nao implementado!");
                    }
                    else
                    {
                        // Not a Http valid protocol
                        if (DEBUG) Serial.println("Erro! Protocolo Http invalido!");
                    }
                }
            }
            else if (isTcpFlagAck())
            {
                if (DEBUG) Serial.println("FLAG TCP ACK!");
                
                if(tratamentoPrintFlag && getTcpSrcPort() == _sessionPortSrc)
                {
                    if (DEBUG) Serial.println("Print flag OK!");
                    
                    tratamentoPrintFlag = 0;
                    
                    return 0;
                }
                
                if(tratamentoCloseFlag && getTcpSrcPort() == _sessionPortSrc)
                {
                    if (DEBUG) Serial.println("Close flag OK!");
                    
                    tratamentoCloseFlag = 0;
                    
                    return 0;
                }
                
                if(tratamentoGetPostFlag && getTcpSrcPort() == _sessionPortSrc)
                {
                    if (DEBUG) Serial.println("GetPost flag OK!");
                    
                    if (tratamentoGetPostCounter > 0)
                    {
                        // Sending second and third parts of
                        // Http Header
                        sendHttpHeader();
                        
                        // Waiting counter gets greather
                        // than number of chunks to send
                        if (tratamentoGetPostCounter > 3)
                            return 0;
                    }
                }
            }
            else
            {
                if (DEBUG) Serial.println("FLAG TCP desconhecida!");
                
                if (DEBUG) { Serial.print("_bufferRd: "); for (int i = 0; i < BUFFER_READ_LEN; i++) { Serial.print(_bufferRd[i], HEX); Serial.print(" "); } Serial.println(); }
        
            }
        }
        
        
        // DEBUG
        //if (DEBUG) { Serial.print("_bufferWr: "); for (int i = 0; i < BUFFER_WRITE_LEN; i++) { Serial.print(_bufferWr[i], HEX); Serial.print(" "); } Serial.println(); }
        
    }
    return 1;
}

void EtherEncLib::sendHttpHeader()
{
    // Building anwser header
    // Sending 3 parts, handled by
    // tratamentoGetPostCounter
    if (DEBUG) { Serial.print("Enviando Http Header: Parte "); Serial.println(tratamentoGetPostCounter, DEC); }
    
    if (tratamentoGetPostCounter <= 2)
    {
        // Clear write's buffer
        initBufferWrite();
        
        // Loading read's buffer into
        // write's buffer
        loadBuffer();
        
        // Cleaning data area from read's buffer
        initBufferWriteData();
        
        // Data for response and length
        // Part 1
        unsigned char lenData = 17;
        char *resposta = "HTTP/1.0 200 OK\n\r";
        
        // Checking counter to decide wich part send now
        if (tratamentoGetPostCounter == 1)
        {
            // Part 2
            lenData = 25;
            resposta = "Content-Type: text/html\n\r";
        }
        else if (tratamentoGetPostCounter == 2)
        {
            // Part 3
            lenData = 20;
            resposta = "Pragma: no-cache\n\r\n\r";
        }
        
        // Loading initial response into write's buffer
        for (int i = 0; i < lenData; i++)
        {
            fillTcpData(i, resposta[i]);
        }
        
        // Formating packet
        if (tratamentoGetPostCounter > 0)
        {
            formatAckPushPrint(lenData);
        }
        else
        {
            // Answer ACK e increment SEQ Num
            // to reflect data reading
            formatAckPushResponse(lenData);
        }
        
        //if (DEBUG) { Serial.print("2 - _bufferWr: "); for (int i = 0; i < BUFFER_WRITE_LEN; i++) { Serial.print(_bufferWr[i], HEX); Serial.print(" "); } Serial.println(); }
        
        // Send ACK+PUSH answer
        socketPacketSend(IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + lenData + ETH_HEADER_LEN_V, _bufferWr);
    }
    
    // Increment counter
    tratamentoGetPostCounter++;
}

void EtherEncLib::sendPrint(char *rd, unsigned int len)
{
    // Clear write's buffer
    initBufferWrite();
    
    // Loading read's buffer into
    // write's buffer
    loadBuffer();
    
    // Cleaning data area from read's buffer
    initBufferWriteData();
    
    // Add data into write's buffer
    for (int i = 0; i < len; i++)
    {
        fillTcpData(i, rd[i]);
    }
    
    // Formating packet
    formatAckPushPrint(len);
    
    // DEBUG
    //if (DEBUG) { Serial.print("1 - _bufferWr: "); for (int i = 0; i < BUFFER_WRITE_LEN; i++) { Serial.print(_bufferWr[i], HEX); Serial.print(" "); } Serial.println(); }
    
    // Send packet
    socketPacketSend(IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + ETH_HEADER_LEN_V + len, _bufferWr);
}

unsigned char EtherEncLib::isReturnOk(unsigned long t)
{
    // Loop ruled by time
    unsigned long tempo = millis() + t;
    while(isBusy() && tempo > millis())
    {
        delay(1);
    }
    
    // At this point we are ready
    // to return true if time is not up
    if (tempo > millis())
        return 1;
    else
        return 0;
}

void EtherEncLib::sendClose()
{
    // Formating packet
    formatAckPushClose();
    
    // DEBUG
    //if (DEBUG) { Serial.print("1 - _bufferWr: "); for (int i = 0; i < BUFFER_WRITE_LEN; i++) { Serial.print(_bufferWr[i], HEX); Serial.print(" "); } Serial.println(); }
    
    // Sending packet
    socketPacketSend(IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + ETH_HEADER_LEN_V + 0, _bufferWr);
}

void EtherEncLib::sendCloseAck()
{
    // Formating packet
    formatAckPushCloseAck();
    // Sending packet
    socketPacketSend(IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + ETH_HEADER_LEN_V + 0, _bufferWr);
}

void EtherEncLib::sendCloseAck2()
{
    // Formating packet
    formatAckPushCloseAck2();
    // Sending packet
    socketPacketSend(IP_HEADER_LEN_V + TCP_HEADER_LEN_PLAIN_V + ETH_HEADER_LEN_V + 0, _bufferWr);
}

void EtherEncLib::print(char *rd, unsigned int len)
{
    // print HTML into socket
    sendPrint(rd, len);
    
    // Callback for ACK's PUSH
    tratamentoPrintFlag = 1;
    
    if (!isReturnOk(TEMPO_PRINT))
    {
        if (DEBUG) Serial.println("ERRO! Enviando pacote PUSH!");
    }
}

unsigned int EtherEncLib::getPrintStringLen(char *_rd)
{
    unsigned int _len = 0;
    for (int i = 0; i < MAX_PRINT_LEN; i++)
    {
        if (_rd[i]) _len++;
        else break;
    }
    if (_len > MAX_PRINT_LEN) _len = MAX_PRINT_LEN;
    return _len;
}

void EtherEncLib::resetSession()
{
    // Closing connection, releases the lock,
    // reset source port and reset flags
    _sessionLock             = 0;
    recebeuRequestFlag       = 0;
    tratamentoGetPostFlag    = 0;
    tratamentoPrintFlag      = 0;
    tratamentoCloseFlag      = 0;
    tratamentoCloseAckFlag   = 0;
    tratamentoGetPostCounter = 0;
    
    // Clear buffers
    initBuffer();
}

