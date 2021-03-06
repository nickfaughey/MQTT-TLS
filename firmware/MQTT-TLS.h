/*
MQTT library for Particle Core, Photon, Arduino
This software is released under the MIT License.

Copyright (c) 2014 Hirotaka Niisato
Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Much of the code was inspired by Arduino Nicholas pubsubclient
sample code bearing this copyright.
//---------------------------------------------------------------------------
// Copyright (c) 2008-2012 Nicholas O'Leary
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------
*/

#ifndef MQTT_TLS_h
#define MQTT_TLS_h

#include "spark_wiring_string.h"
#include "spark_wiring_tcpclient.h"
#include "spark_wiring_usbserial.h"


#include "check_config.h"

#include "net.h"
#include "debug.h"
#include "ssl.h"
#include "entropy.h"
#include "ctr_drbg.h"
#include "error.h"
#include "timing.h"
#include "ssl_internal.h"
#include "timer_hal.h"

// for debugging.
#define DEBUG_TLS       1
#if defined(MBEDTLS_DEBUG_C)
#define DEBUG_TLS_CORE_LEVEL 1
#define debug_tls( fmt, ... ) \
    Serial.printf(fmt, ##__VA_ARGS__)
#else /* !DEBUG_TLS */
  #define debug_tls( fmt, ... ) ((void)0)
#endif /* DEBUG_TLS */

// MQTT_MAX_PACKET_SIZE : Maximum packet size
// this size is total of [MQTT Header(Max:5byte) + Topic Name Length + Topic Name + Message ID(QoS1|2) + Payload]
#define MQTT_MAX_PACKET_SIZE 255

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_KEEPALIVE 15

#define MQTTPROTOCOLVERSION 4
#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

class MQTT {
/** types */
public:
typedef enum{
    QOS0 = 0,
    QOS1 = 1,
    QOS2 = 2,
}EMQTT_QOS;

private:
    TCPClient tcpClient;

    uint8_t *buffer;
    uint16_t nextMsgId;
    unsigned long lastOutActivity;
    unsigned long lastInActivity;
    bool pingOutstanding;
    void (*callback)(char*,uint8_t*,unsigned int);
    void (*qoscallback)(unsigned int);
    uint16_t readPacket(uint8_t*);
    uint8_t readByte();
    bool write(uint8_t header, uint8_t* buf, uint16_t length);
    uint16_t writeString(const char* string, uint8_t* buf, uint16_t pos);
    String domain;
    uint8_t *ip;
    uint16_t port;
    uint16_t maxpacketsize;

    void initialize(char* domain, uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);
    uint16_t netWrite(unsigned char *buff, int length);
    bool available();


    /* TLS */
    mbedtls_entropy_context entropy;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;
    mbedtls_timing_delay_context timer;

    bool tlsConnected;
    bool tls;

    static int send_Tls(void *ctx, const unsigned char *buf, size_t len);
    static int recv_Tls(void *ctx, unsigned char *buf, size_t len);
    static int rng_Tls(void* handle, uint8_t* data, const size_t len_);
    static void debug_Tls( void *ctx, int level,
                          const char *file, int line,
                          const char *str );
    int handShakeTls();


public:
    MQTT();

    MQTT(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int));
    MQTT(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);
    MQTT(uint8_t *, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int));
    MQTT(uint8_t *, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int), int maxpacketsize);
    ~MQTT();

    bool connect(const char *);
    bool connect(const char *, const char *, const char *);
    bool connect(const char *, const char *, EMQTT_QOS, uint8_t, const char *);
    bool connect(const char *, const char *, const char *, const char *, EMQTT_QOS, uint8_t, const char*);
    void disconnect();

    bool publish(const char *, const char *);
    bool publish(const char *, const char *, EMQTT_QOS, uint16_t *messageid = NULL);
    bool publish(const char *, const char *, EMQTT_QOS, bool, uint16_t *messageid = NULL);
    bool publish(const char *, const uint8_t *, unsigned int);
    bool publish(const char *, const uint8_t *, unsigned int, EMQTT_QOS, uint16_t *messageid = NULL);
    bool publish(const char *, const uint8_t *, unsigned int, EMQTT_QOS, bool, uint16_t *messageid = NULL);
    bool publish(const char *, const uint8_t *, unsigned int, bool);
    bool publish(const char *, const uint8_t *, unsigned int, bool, EMQTT_QOS, uint16_t *messageid = NULL);
    bool publish(const char *, const uint8_t *, unsigned int, bool, EMQTT_QOS, bool, uint16_t *messageid);
    void addQosCallback(void (*qoscallback)(unsigned int));
    bool publishRelease(uint16_t messageid);

    bool subscribe(const char *);
    bool subscribe(const char *, EMQTT_QOS);
    bool unsubscribe(const char *);
    bool loop();
    bool isConnected();

    /* TLS */
    bool verify();
    int enableTls(const char *rootCaPem, const size_t rootCaPemSize);
};

#endif
