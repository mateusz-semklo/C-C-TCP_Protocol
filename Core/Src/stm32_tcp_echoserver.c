/**
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of and a contribution to the lwIP TCP/IP stack.
 *
 * Credits go to Adam Dunkels (and the current maintainers) of this software.
 *
 * Christiaan Simons rewrote this file to get a more stable echo application.
 *
 **/

 /* This file was modified by ST */

#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "stm32_tcp_echoserver.h"
#include "cJSON.h"



#if LWIP_TCP

static struct tcp_pcb *tcp_echoserver_pcb;


/* States of echo protocol */
enum tcp_echoserver_states{
  ES_NONE = 0,
  ES_ACCEPTED,
  ES_RECEIVED,
  ES_CLOSING
};

/* Structure for maintaing connection infos, that is passed as argumetns for LWIP callback */





/* Static functions declaration */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_echoserver_error(void *arg, err_t err);
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es);
static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es);

/*
 * @brief: Init echo server
 * @param: none
 * @ret: none
 */
void tcp_echoserver_init(void)
{
  /* Create new connection control block */
  tcp_echoserver_pcb = tcp_new();

  if (tcp_echoserver_pcb != NULL)
  {
    if (tcp_bind(tcp_echoserver_pcb, IP_ADDR_ANY, SERVER_TCP_PORT) == ERR_OK)
    {
      tcp_echoserver_pcb = tcp_listen(tcp_echoserver_pcb);
      tcp_accept(tcp_echoserver_pcb, tcp_echoserver_accept);
    }
    else /* tcp_bind return with ERR_USE, port is already used */
    {
      /* Set selected element free, clear all settings for it */
      memp_free(MEMP_TCP_PCB, tcp_echoserver_pcb);
    }
  }
}

/*
 * @brief  This function is the implementation of tcp_accept LwIP callback
 * @param  arg: not used
 * @param  newpcb: pointer to new made connection
 * @param  err: not used
 * @retval err_t: error status
 */
static err_t tcp_echoserver_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  err_t ret_err;
  struct tcp_echoserver_struct *es;

  /* Unused arguments to prevent warnings */
  (void)arg;
  (void)err;

  /* Set priority for new connection */
  tcp_setprio(newpcb, TCP_PRIO_MIN);

  /* allocate structure with info about tcp connection */
  es = (struct tcp_echoserver_struct *)mem_malloc(sizeof(struct tcp_echoserver_struct));

  if (es != NULL)
  {
    es->state = ES_ACCEPTED;
    es->pcb = newpcb;
    es->retries = 0;
    es->p = NULL;

    /* Pass structure data do new connection */
    tcp_arg(newpcb, es);

    /* prepare to receive data */
    tcp_recv(newpcb, tcp_echoserver_recv);

    /* if error ocure then tha t callback will be */
    tcp_err(newpcb, tcp_echoserver_error);

    /* waits for connection */
    tcp_poll(newpcb, tcp_echoserver_poll, 0);

    ret_err = ERR_OK;
  }
  else
  {
    /* Close connection */
    tcp_echoserver_connection_close(newpcb, es);
    /* return error */
    ret_err = ERR_MEM;
  }
  return ret_err;
}
/*
 * @brief  Reveive data callback
 * @param  arg:
 * @param  tpcb: pointer to connection data
 * @param  p: pointer to buffer that conteins data
 * @retval err_t: error status
 */
static err_t tcp_echoserver_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  struct tcp_echoserver_struct *es;
  err_t ret_err;

  LWIP_ASSERT("arg != NULL",arg != NULL);

  es = (struct tcp_echoserver_struct *)arg;

  /* If buffer is empty */
  if (p == NULL)
  {
    /* Close connection */
    es->state = ES_CLOSING;
    if(es->p == NULL)
    {
       /* Close connection */
       tcp_echoserver_connection_close(tpcb, es);
    }
    else
    {
      /* Callback function used when data was received */
      tcp_sent(tpcb, tcp_echoserver_sent);

      /* Send data back to server */
      tcp_echoserver_send(tpcb, es);
    }
    ret_err = ERR_OK;
  }
  else if(err != ERR_OK)
  {
	/* Error occure, clear buffer  */
    if (p != NULL)
    {
      es->p = NULL;
      pbuf_free(p);
    }
    ret_err = err;
  }
  else if(es->state == ES_ACCEPTED)
  {
	/* connection accept, first data received, chunk in p->payload */
    es->state = ES_RECEIVED;

    /* write data to structuce*/
    es->p = p;

    tcp_sent(tpcb, tcp_echoserver_sent);

    /* Send data */
    tcp_echoserver_send(tpcb, es);

    ret_err = ERR_OK;
  }
  else if (es->state == ES_RECEIVED)
  {
	/* All data received */
    if(es->p == NULL)
    {
      es->p = p;

      tcp_echoserver_send(tpcb, es);
    }
    else
    {
      struct pbuf *ptr;

      /* chain two to the end of what we recv'ed previously */
      ptr = es->p;
      pbuf_chain(ptr,p);
    }
    ret_err = ERR_OK;
  }
  else if(es->state == ES_CLOSING)
  {
    //odd case, remote side closing twice, free all trash data
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  else
  {
    //unkown es->state, trash data
    tcp_recved(tpcb, p->tot_len);
    es->p = NULL;
    pbuf_free(p);
    ret_err = ERR_OK;
  }
  return ret_err;
}

/*
 * @brief  Error callback
 * @param  arg: pass structure data
 * @param  err: not used
 * @retval none
 */
static void tcp_echoserver_error(void *arg, err_t err)
{
  struct tcp_echoserver_struct *es;

  (void)err;

  es = (struct tcp_echoserver_struct *)arg;
  if (es != NULL)
  {
    /* free structure */
    mem_free(es);
  }
}

/*
 * @brief  poll aplication with callback function
 * @param  arg: pass structure data
 * @param  tcp_pcb: pointer to data structure
 * @retval err_t: error status
 */
static err_t tcp_echoserver_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct tcp_echoserver_struct *es;

  es = (struct tcp_echoserver_struct *)arg;
  if (es != NULL)
  {
    if (es->p != NULL)
    {
      tcp_sent(tpcb, tcp_echoserver_sent);
      /* There is data in pbuf, chain try to send it */
      tcp_echoserver_send(tpcb, es);
    }
    else
    {
       /* No data in chain */
      if(es->state == ES_CLOSING)
      {
        /* Close connection */
        tcp_echoserver_connection_close(tpcb, es);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
	/* Aborts the connection by sending a RST (reset) segment to the remote host */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}

/*
 * @brief  Function is called when data has been acknowledged by remote host
 * @param  arg: pass structure data
 * @param  tcp_pcb: pointer to data structure
 * @param  len: data length
 * @retval err_t: error status
 */
static err_t tcp_echoserver_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct tcp_echoserver_struct *es;

  (void)len;

  es = (struct tcp_echoserver_struct *)arg;
  es->retries = 0;

  if(es->p != NULL)
  {
    /* data still in buffer */
    tcp_sent(tpcb, tcp_echoserver_sent);
    tcp_echoserver_send(tpcb, es);
  }
  else
  {
    /* No data to send close connection */
    if(es->state == ES_CLOSING)
    {
      tcp_echoserver_connection_close(tpcb, es);
    }
  }
  return ERR_OK;
}

/*
 * @brief  Function that sends data back to server
 * @param  arg: pass structure data
 * @param  tcp_pcb: pointer to data structure
 * @param  len: data length
 * @retval err_t: error status
 */

#define USART_COPY 1
static void tcp_echoserver_send(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
  struct pbuf *ptr;

#if USART_COPY == 1
  char dane[256] = {0};
  char buffer[256] = {0};
#endif

  err_t wr_err = ERR_OK;

  for(int i=0; i<size;i++)
	  jstring[i]=0;

  /* tcp_sndbuf - returns number of bytes in space that is avaliable in output queue */
  while ((wr_err == ERR_OK) && (es->p != NULL) && (es->p->len <= tcp_sndbuf(tpcb)))
  {
    /* get pointer on pbuf from es structure */
    ptr = es->p;

#if USART_COPY == 1
    sprintf(dane, "%s",(char *)ptr->payload);
    for(uint8_t i = 0; i<ptr->len; i++)
    {
    	buffer[i] = dane[i];
    	//dane1[i]=dane[i];
    }

    /* Close connection */
    if(buffer[0] == 'E' && buffer[1] == 'N' && buffer[2] == 'D') {
    	tcp_echoserver_connection_close(tpcb, es);
    }
    else{
      //  Usart_Uart_SendString(USART1, buffer, LF_CR);
    }
#endif

    // char *rendered;
    // char *rendered2;
     cJSON * root;
     cJSON * settings;

     root = cJSON_Parse((char *)ptr->payload);
     settings = cJSON_GetObjectItemCaseSensitive(root, "settings");

     rendered = cJSON_Print(root);

    sprintf(settings_allow, "%s",(char *)(cJSON_GetStringValue(settings)));
    sprintf(jstring, "%s",(char *)rendered);
    //sprintf(jstring, "%s",(char *)ptr->payload);

    if(settings_allow[0]=='t')
    	HAL_UART_Transmit_IT(&huart2, jstring, sizeof(jstring));


        char  speed_jstring[10];
        char  angle[10];

       cJSON *head;
       head = cJSON_CreateObject();
       sprintf(speed_jstring,"%4.1f",speed);
       sprintf(angle,"%d",capture_tim3_ccr1);


       cJSON_AddItemToObject(head, "speed", cJSON_CreateString(speed_jstring));
       cJSON_AddItemToObject(head, "encoder", cJSON_CreateString(angle));
       rendered2 = cJSON_Print(head);
       sprintf(jstring2, "%s",(char *)rendered2);

       cJSON_Delete(head);

       wr_err = tcp_write(tpcb, jstring2, strlen(rendered2), 1);




    cJSON_Delete(root);

    //wr_err = tcp_write(tpcb, buffereth, strlen(buffereth), 1);
    //wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
     //wr_err = tcp_write(tpcb, jstring, strlen(rendered), 1);
   // wr_err = tcp_write(tpcb, jstring2, strlen(rendered2), 1);

    /* Clear data */
    memset(dane, 0x00, sizeof(dane));

    if (wr_err == ERR_OK)
    {
      u16_t plen;
      u8_t freed;

      plen = ptr->len;

      /* continue with next pbuf in chain (if any) */
      es->p = ptr->next;

      if(es->p != NULL)
      {
        /* increment reference count for es->p */
        pbuf_ref(es->p);
      }

      /* chop first pbuf from chain */
      do
      {
        /* try hard to free pbuf */
        freed = pbuf_free(ptr);
      }
      while(freed == 0);
     /* we can read more data now */
     tcp_recved(tpcb, plen);
   }
   else if(wr_err == ERR_MEM)
   {
     /* we are low on memory, try later / harder, defer to poll */
     es->p = ptr;
   }
   else { }
  }
}

/*
 * @brief  Close connection
 * @param  *tpcb: connection parameters
 * @param  *es: struct with data
 * @retval none
 */

static void tcp_echoserver_connection_close(struct tcp_pcb *tpcb, struct tcp_echoserver_struct *es)
{
  //remove all callbacks
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_err(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);

  /* free structure */
  if (es != NULL)
  {
    mem_free(es);
  }

  /* Connection close */
  tcp_close(tpcb);
}
#endif /* LWIP_TCP */
