#ifndef STM32_TCP_ECHOSERVER_H_
#define STM32_TCP_ECHOSERVER_H_

#include "main.h"
#include "cJSON.h"

#define sizze 256

volatile  char dane1[sizze];

volatile char *rendered;
volatile cJSON * root;

struct tcp_echoserver_struct{
  u8_t state;             /* current connection state */
  u8_t retries;			  /* number of connection retries */
  struct tcp_pcb *pcb;    /* pointer on the current tcp_pcb */
  struct pbuf *p;         /* pointer on the received/to be transmitted pbuf */
};


//--------------------------------------------------------------
/* STM32 Mac adress */
#define myMAC_ADDR0   0x00
#define myMAC_ADDR1   0x80
#define myMAC_ADDR2   0xE1
#define myMAC_ADDR3   0x00
#define myMAC_ADDR4   0x00
#define myMAC_ADDR5   0x00

//--------------------------------------------------------------
/* Static Ip Adress */
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   0
#define IP_ADDR3   6

//--------------------------------------------------------------
/* Net mask: 255.255.255.0 */
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

//--------------------------------------------------------------
/* Gateway: 169.254.136.1 */
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   0
#define GW_ADDR3   1

//--------------------------------------------------------------
/* Ip host address */
#define HOST_IP_0   192
#define HOST_IP_1   168
#define HOST_IP_2   0
#define HOST_IP_3   2

//--------------------------------------------------------------
/* UDP port number */
#define  SERVER_TCP_PORT      3333
#define  CLIENT_TCP_PORT      3333

//--------------------------------------------------------------
/* Max size of buffer */
#define  UDP_RECEIVE_MSG_SIZE   150

void tcp_echoserver_init(void);

#endif /* STM32_TCP_ECHOSERVER_H_ */
