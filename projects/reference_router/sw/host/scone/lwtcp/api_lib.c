/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: api_lib.c 2720 2007-08-12 05:47:54Z derickso $
 */

/*******************************************************************************
*
* @NETFPGA_LICENSE_HEADER_START@
*
* Licensed to NetFPGA C.I.C. (NetFPGA) under one or more contributor
* license agreements. See the NOTICE file distributed with this work for
* additional information regarding copyright ownership. NetFPGA licenses this
* file to you under the NetFPGA Hardware-Software License, Version 1.0 (the
* "License"); you may not use this file except in compliance with the
* License. You may obtain a copy of the License at:
*
* http://www.netfpga-cic.org
*
* Unless required by applicable law or agreed to in writing, Work distributed
* under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
* CONDITIONS OF ANY KIND, either express or implied. See the License for the
* specific language governing permissions and limitations under the License.
*
* @NETFPGA_LICENSE_HEADER_END@
*
*
******************************************************************************/


/* This is the part of the API that is linked with
   the application */

#include "lwip/debug.h"
#include "lwip/api.h"
#include "lwip/api_msg.h"
#include "lwip/memp.h"

#include "lwip/debug.h"

/*-----------------------------------------------------------------------------------*/
struct
netbuf *netbuf_new(void)
{
  struct netbuf *buf;

  buf = memp_mallocp(MEMP_NETBUF);
  if(buf != NULL) {
    buf->p = NULL;
    buf->ptr = NULL;
    return buf;
  } else {
    return NULL;
  }
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_delete(struct netbuf *buf)
{
  if(buf != NULL) {
    if(buf->p != NULL) {
      pbuf_free(buf->p);
      buf->p = buf->ptr = NULL;
    }
    memp_freep(MEMP_NETBUF, buf);
  }
}
/*-----------------------------------------------------------------------------------*/
void *
netbuf_alloc(struct netbuf *buf, uint16_t size)
{
  /* Deallocate any previously allocated memory. */
  if(buf->p != NULL) {
    pbuf_free(buf->p);
  }
  buf->p = pbuf_alloc(PBUF_TRANSPORT, size, PBUF_RAM);
  if(buf->p == NULL) {
     return NULL;
  }
  buf->ptr = buf->p;
  return buf->p->payload;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_free(struct netbuf *buf)
{
  if(buf->p != NULL) {
    pbuf_free(buf->p);
  }
  buf->p = buf->ptr = NULL;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_ref(struct netbuf *buf, void *dataptr, uint16_t size)
{
  if(buf->p != NULL) {
    pbuf_free(buf->p);
  }
  buf->p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_ROM);
  buf->p->payload = dataptr;
  buf->p->len = buf->p->tot_len = size;
  buf->ptr = buf->p;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_chain(struct netbuf *head, struct netbuf *tail)
{
  pbuf_chain(head->p, tail->p);
  head->ptr = head->p;
  memp_freep(MEMP_NETBUF, tail);
}
/*-----------------------------------------------------------------------------------*/
uint16_t
netbuf_len(struct netbuf *buf)
{
  return buf->p->tot_len;
}
/*-----------------------------------------------------------------------------------*/
err_t
netbuf_data(struct netbuf *buf, void **dataptr, uint16_t *len)
{
  if(buf->ptr == NULL) {
    return ERR_BUF;
  }
  *dataptr = buf->ptr->payload;
  *len = buf->ptr->len;
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
int8_t
netbuf_next(struct netbuf *buf)
{
  if(buf->ptr->next == NULL) {
    return -1;
  }
  buf->ptr = buf->ptr->next;
  if(buf->ptr->next == NULL) {
    return 1;
  }
  return 0;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_first(struct netbuf *buf)
{
  buf->ptr = buf->p;
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_copy_partial(struct netbuf *buf, void *dataptr, uint16_t len, uint16_t offset)
{
  struct pbuf *p;
  uint16_t i, left;

  left = 0;

  if(buf == NULL) {
    return;
  }

  /* This implementation is bad. It should use bcopy
     instead. */
  for(p = buf->p; left < len && p != NULL; p = p->next) {
    if(offset != 0 && offset >= p->len) {
      offset -= p->len;
    } else {
      for(i = offset; i < p->len; ++i) {
	((char *)dataptr)[left] = ((char *)p->payload)[i];
	if(++left >= len) {
	  return;
	}
      }
    }
  }
}
/*-----------------------------------------------------------------------------------*/
void
netbuf_copy(struct netbuf *buf, void *dataptr, uint16_t len)
{
  netbuf_copy_partial(buf, dataptr, len, 0);
}
/*-----------------------------------------------------------------------------------*/
struct ip_addr *
netbuf_fromaddr(struct netbuf *buf)
{
  return buf->fromaddr;
}
/*-----------------------------------------------------------------------------------*/
uint16_t
netbuf_fromport(struct netbuf *buf)
{
  return buf->fromport;
}
/*-----------------------------------------------------------------------------------*/
struct
netconn *netconn_new(enum netconn_type t)
{
  struct netconn *conn;

  conn = memp_mallocp(MEMP_NETCONN);
  if(conn == NULL) {
    return NULL;
  }
  conn->type = t;
  conn->pcb.tcp = NULL;

  if((conn->mbox = sys_mbox_new()) == SYS_MBOX_NULL) {
    memp_freep(MEMP_NETCONN, conn);
    return NULL;
  }
  conn->recvmbox = SYS_MBOX_NULL;
  conn->acceptmbox = SYS_MBOX_NULL;
  conn->sem = SYS_SEM_NULL;
  conn->state = NETCONN_NONE;
  return conn;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_delete(struct netconn *conn)
{
  struct api_msg *msg;
  void *mem;

  if(conn == NULL) {
    return ERR_OK;
  }

  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return ERR_MEM;
  }

  msg->type = API_MSG_DELCONN;
  msg->msg.conn = conn;
  api_msg_post(msg);
  sys_mbox_fetch(conn->mbox, NULL);
  memp_freep(MEMP_API_MSG, msg);

  /* Drain the recvmbox. */
  if(conn->recvmbox != SYS_MBOX_NULL) {
    while(sys_arch_mbox_fetch(conn->recvmbox, &mem, 1) != 0) {
      if(conn->type == NETCONN_TCP) {
	pbuf_free((struct pbuf *)mem);
      } else {
	netbuf_delete((struct netbuf *)mem);
      }
    }
    sys_mbox_free(conn->recvmbox);
    conn->recvmbox = SYS_MBOX_NULL;
  }


  /* Drain the acceptmbox. */
  if(conn->acceptmbox != SYS_MBOX_NULL) {
    while(sys_arch_mbox_fetch(conn->acceptmbox, &mem, 1) != 0) {
      netconn_delete((struct netconn *)mem);
    }

    sys_mbox_free(conn->acceptmbox);
    conn->acceptmbox = SYS_MBOX_NULL;
  }

  sys_mbox_free(conn->mbox);
  conn->mbox = SYS_MBOX_NULL;
  if(conn->sem != SYS_SEM_NULL) {
    sys_sem_free(conn->sem);
  }
  /*  conn->sem = SYS_SEM_NULL;*/
  memp_free(MEMP_NETCONN, conn);
  return ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
enum netconn_type
netconn_type(struct netconn *conn)
{
  return conn->type;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_peer(struct netconn *conn, struct ip_addr **addr,
	     uint16_t *port)
{
  switch(conn->type) {
  case NETCONN_UDPLITE:
  case NETCONN_UDPNOCHKSUM:
  case NETCONN_UDP:
    *addr = &(conn->pcb.udp->remote_ip);
    *port = conn->pcb.udp->remote_port;
    break;
  case NETCONN_TCP:
    *addr = &(conn->pcb.tcp->remote_ip);
    *port = conn->pcb.tcp->remote_port;
    break;
  }
  return (conn->err = ERR_OK);
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_addr(struct netconn *conn, struct ip_addr **addr,
	     uint16_t *port)
{
  switch(conn->type) {
  case NETCONN_UDPLITE:
  case NETCONN_UDPNOCHKSUM:
  case NETCONN_UDP:
    *addr = &(conn->pcb.udp->local_ip);
    *port = conn->pcb.udp->local_port;
    break;
  case NETCONN_TCP:
    *addr = &(conn->pcb.tcp->local_ip);
    *port = conn->pcb.tcp->local_port;
    break;
  }
  return (conn->err = ERR_OK);
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_bind(struct netconn *conn, struct ip_addr *addr,
	    uint16_t port)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->type != NETCONN_TCP &&
     conn->recvmbox == SYS_MBOX_NULL) {
    if((conn->recvmbox = sys_mbox_new()) == SYS_MBOX_NULL) {
      return ERR_MEM;
    }
  }

  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }
  msg->type = API_MSG_BIND;
  msg->msg.conn = conn;
  msg->msg.msg.bc.ipaddr = addr;
  msg->msg.msg.bc.port = port;
  api_msg_post(msg);
  sys_mbox_fetch(conn->mbox, NULL);
  memp_freep(MEMP_API_MSG, msg);
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_connect(struct netconn *conn, struct ip_addr *addr,
		   uint16_t port)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }


  if(conn->recvmbox == SYS_MBOX_NULL) {
    if((conn->recvmbox = sys_mbox_new()) == SYS_MBOX_NULL) {
      return ERR_MEM;
    }
  }

  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return ERR_MEM;
  }
  msg->type = API_MSG_CONNECT;
  msg->msg.conn = conn;
  msg->msg.msg.bc.ipaddr = addr;
  msg->msg.msg.bc.port = port;
  api_msg_post(msg);
  sys_mbox_fetch(conn->mbox, NULL);
  memp_freep(MEMP_API_MSG, msg);
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_listen(struct netconn *conn)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->acceptmbox == SYS_MBOX_NULL) {
    conn->acceptmbox = sys_mbox_new();
    if(conn->acceptmbox == SYS_MBOX_NULL) {
      return ERR_MEM;
    }
  }

  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }
  msg->type = API_MSG_LISTEN;
  msg->msg.conn = conn;
  api_msg_post(msg);
  sys_mbox_fetch(conn->mbox, NULL);
  memp_freep(MEMP_API_MSG, msg);
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/
struct netconn *
netconn_accept(struct netconn *conn)
{
  struct netconn *newconn;

  if(conn == NULL) {
    return NULL;
  }

  sys_mbox_fetch(conn->acceptmbox, (void **)&newconn);

  return newconn;
}
/*-----------------------------------------------------------------------------------*/
struct netbuf *
netconn_recv(struct netconn *conn)
{
  struct api_msg *msg;
  struct netbuf *buf;
  struct pbuf *p;

  if(conn == NULL) {
    return NULL;
  }

  if(conn->recvmbox == SYS_MBOX_NULL) {
    conn->err = ERR_CONN;
    return NULL;
  }

  if(conn->err != ERR_OK) {
    return NULL;
  }

  if(conn->type == NETCONN_TCP) {
    if(conn->pcb.tcp->state == LISTEN) {
      conn->err = ERR_CONN;
      return NULL;
    }


    buf = memp_mallocp(MEMP_NETBUF);

    if(buf == NULL) {
      conn->err = ERR_MEM;
      return NULL;
    }

    sys_mbox_fetch(conn->recvmbox, (void **)&p);

    /* If we are closed, we indicate that we no longer wish to recieve
       data by setting conn->recvmbox to SYS_MBOX_NULL. */
    if(p == NULL) {
      memp_freep(MEMP_NETBUF, buf);
      sys_mbox_free(conn->recvmbox);
      conn->recvmbox = SYS_MBOX_NULL;
      return NULL;
    }

    buf->p = p;
    buf->ptr = p;
    buf->fromport = 0;
    buf->fromaddr = NULL;

    /* Let the stack know that we have taken the data. */
    if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
      conn->err = ERR_MEM;
      return buf;
    }
    msg->type = API_MSG_RECV;
    msg->msg.conn = conn;
    if(buf != NULL) {
      msg->msg.msg.len = buf->p->tot_len;
    } else {
      msg->msg.msg.len = 1;
    }
    api_msg_post(msg);

    sys_mbox_fetch(conn->mbox, NULL);
    memp_freep(MEMP_API_MSG, msg);
  } else {
    sys_mbox_fetch(conn->recvmbox, (void **)&buf);
  }




  DEBUGF(API_LIB_DEBUG, ("netconn_recv: received %p (err %d)\n", buf, conn->err));


  return buf;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_send(struct netconn *conn, struct netbuf *buf)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->err != ERR_OK) {
    return conn->err;
  }

  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }

  DEBUGF(API_LIB_DEBUG, ("netconn_send: sending %d bytes\n", buf->p->tot_len));
  msg->type = API_MSG_SEND;
  msg->msg.conn = conn;
  msg->msg.msg.p = buf->p;
  api_msg_post(msg);

  sys_mbox_fetch(conn->mbox, NULL);
  memp_freep(MEMP_API_MSG, msg);
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_write(struct netconn *conn, void *dataptr, uint16_t size, uint8_t copy)
{
  struct api_msg *msg;
  uint16_t len;
  int big_trouble = 0;

  if(conn == NULL) {
    return ERR_VAL;
  }

  if(conn->err != ERR_OK) {
    return conn->err;
  }

  if(conn->sem == SYS_SEM_NULL) {
    conn->sem = sys_sem_new(0);
    if(conn->sem == SYS_SEM_NULL) {
      return ERR_MEM;
    }
  }

  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }
  msg->type = API_MSG_WRITE;
  msg->msg.conn = conn;

  conn->state = NETCONN_WRITE;
  while(conn->err == ERR_OK && size > 0) {
    msg->msg.msg.w.dataptr = dataptr;
    msg->msg.msg.w.copy = copy;

		if (conn->type == NETCONN_TCP) {
			if(tcp_sndbuf(conn->pcb.tcp) == 0) {
				sys_sem_wait(conn->sem);
				if(conn->err != ERR_OK) {
					big_trouble = 1;
					goto ret;
				}
			}
			if(size > tcp_sndbuf(conn->pcb.tcp)) {
				/* We cannot send more than one send buffer's worth of data at a
				   time. */
				len = tcp_sndbuf(conn->pcb.tcp);
			} else {
				len = size;
			}
		} else {
      len = size;
    }

    DEBUGF(API_LIB_DEBUG, ("netconn_write: writing %d bytes (%d)\n", len, copy));
    msg->msg.msg.w.len = len;
    api_msg_post(msg);
    sys_mbox_fetch(conn->mbox, NULL);
    if(conn->err == ERR_OK) {
      dataptr = (void *)((char *)dataptr + len);
      size -= len;
    } else if(conn->err == ERR_MEM) {
      conn->err = ERR_OK;
      sys_sem_wait(conn->sem);
    } else {
    	big_trouble = 1;
      goto ret;
    }
  }
 ret:
  memp_freep(MEMP_API_MSG, msg);
	// Only run the following if there is a major sending error
	if (big_trouble) {
	  conn->state = NETCONN_NONE;
	  if(conn->sem != SYS_SEM_NULL) {
	    sys_sem_free(conn->sem);
	    conn->sem = SYS_SEM_NULL;
	  }
	}

  return conn->err;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_close(struct netconn *conn)
{
  struct api_msg *msg;

  if(conn == NULL) {
    return ERR_VAL;
  }
  if((msg = memp_mallocp(MEMP_API_MSG)) == NULL) {
    return (conn->err = ERR_MEM);
  }

  conn->state = NETCONN_CLOSE;
 again:
  msg->type = API_MSG_CLOSE;
  msg->msg.conn = conn;
  api_msg_post(msg);
  sys_mbox_fetch(conn->mbox, NULL);
  if(conn->err == ERR_MEM &&
     conn->sem != SYS_SEM_NULL) {
    sys_sem_wait(conn->sem);
    goto again;
  }
  conn->state = NETCONN_NONE;
  memp_freep(MEMP_API_MSG, msg);
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/
err_t
netconn_err(struct netconn *conn)
{
  return conn->err;
}
/*-----------------------------------------------------------------------------------*/




