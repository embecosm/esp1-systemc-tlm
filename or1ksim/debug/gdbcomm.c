/* gdbcomm.c -- Communication routines for gdb
         Copyright (C) 2001 by Marko Mlinar, markom@opencores.org
         Code copied from toplevel.c

         This file is part of OpenRISC 1000 Architectural Simulator.
         
         This program is free software; you can redistribute it and/or modify
         it under the terms of the GNU General Public License as published by
         the Free Software Foundation; either version 2 of the License, or
         (at your option) any later version.
         
         This program is distributed in the hope that it will be useful,
         but WITHOUT ANY WARRANTY; without even the implied warranty of
         MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
         GNU General Public License for more details.

         You should have received a copy of the GNU General Public License
         along with this program; if not, write to the Free Software
         Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "gdb.h"
#include "gdbcomm.h"
#include "vapi.h"
#include "sim-config.h"
#include "debug_unit.h"

static int gdb_read(void* buf,int len);
static int gdb_write(const void* buf,int len);

static unsigned int serverIP = 0;
static unsigned int serverPort = 0;
static unsigned int server_fd = 0;
static unsigned int gdb_fd = 0;

static int tcp_level = 0;

/* Added by CZ 24/05/01 */
int GetServerSocket(const char* name,const char* proto,int port)
{
  struct servent *service;
  struct protoent *protocol;
  struct sockaddr_in sa;
  struct hostent *hp;  
  int sockfd;
  char myname[256];
  int flags;
  char sTemp[256];
  socklen_t len;

  /* First, get the protocol number of TCP */
  if(!(protocol = getprotobyname(proto)))
    {
      sprintf(sTemp,"Unable to load protocol \"%s\"",proto);
      perror(sTemp);
      return 0;
    }
  tcp_level = protocol->p_proto; /* Save for later */

  /* If we weren't passed a non standard port, get the port
     from the services directory. */
  if(!port)
    {
      if((service = getservbyname(name,protocol->p_name)))
        port = ntohs(service->s_port);
    }
 
  /* Create the socket using the TCP protocol */
  if((sockfd = socket(PF_INET,SOCK_STREAM,protocol->p_proto)) < 0)
    {
      perror("Unable to create socket");
      return 0;
    }
 
  flags = 1;
  if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&flags,sizeof(int)) < 0)
    {
      sprintf(sTemp,"Can not set SO_REUSEADDR option on socket %d",sockfd);
      perror(sTemp);
      close(sockfd);
      return 0;
    }

  /* The server should also be non blocking. Get the current flags. */
  if(fcntl(sockfd,F_GETFL,&flags) < 0)
    {
      sprintf(sTemp,"Unable to get flags for socket %d",sockfd);
      perror(sTemp);
      close(sockfd);
      return 0;
    }

  /* Set the nonblocking flag */
  if(fcntl(sockfd,F_SETFL, flags | O_NONBLOCK) < 0)
    {
      sprintf(sTemp,"Unable to set flags for socket %d to value 0x%08x",
              sockfd,flags | O_NONBLOCK);
      perror(sTemp);
      close(sockfd);
      return 0;
    }

  /* Find out what our address is */
  memset(&sa,0,sizeof(struct sockaddr_in));
  gethostname(myname,sizeof(myname));
  if(!(hp = gethostbyname(myname)))
    {
      perror("Unable to read hostname");
      close(sockfd);
      return 0;
    }
 
  /* Bind our socket to the appropriate address */
  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons(port);
  if(bind(sockfd,(struct sockaddr*)&sa,sizeof(struct sockaddr_in)) < 0)
    {
      sprintf(sTemp,"Unable to bind socket %d to port %d",sockfd,port);
      perror(sTemp);
      close(sockfd);
      return 0;
    }
  serverIP = sa.sin_addr.s_addr;
  len = sizeof(struct sockaddr_in);
  if(getsockname(sockfd,(struct sockaddr*)&sa,&len) < 0)
    {
      sprintf(sTemp,"Unable to get socket information for socket %d",sockfd);
      perror(sTemp);
      close(sockfd);
      return 0;
    }
  serverPort = ntohs(sa.sin_port);

  /* Set the backlog to 1 connections */
  if(listen(sockfd,1) < 0)
    {
      sprintf(sTemp,"Unable to set backlog on socket %d to %d",sockfd,1);
      perror(sTemp);
      close(sockfd);
      return 0;
    }

  return sockfd;
}

void BlockJTAG(void)
{
  struct pollfd fds[2];
  int n = 0;

  fds[n].fd = server_fd;
  fds[n].events = POLLIN;
  fds[n++].revents = 0;
  if(gdb_fd)
    {
      fds[n].fd = gdb_fd;
      fds[n].events = POLLIN;
      fds[n++].revents = 0;
    }
  poll(fds,n,-1);
}

void HandleServerSocket(Boolean block)
{
  struct pollfd fds[3];
  int n = 0;
  int timeout = block ? -1 : 0;
  Boolean data_on_stdin = false;
  int o_serv_fd = server_fd;
  
  if(!o_serv_fd && !gdb_fd)
    return;

  if(o_serv_fd)
    {
      fds[n].fd = o_serv_fd;
      fds[n].events = POLLIN;
      fds[n++].revents = 0;
    }
  if(gdb_fd)
    {
      fds[n].fd = gdb_fd;
      fds[n].events = POLLIN;
      fds[n++].revents = 0;
    }
  if(block)
    {
      fds[n].fd = 0;
      fds[n].events = POLLIN;
      fds[n++].revents = 0;
    }

  while(!data_on_stdin)
    {
      switch(poll(fds,n,timeout))
        {
        case -1:
          if(errno == EINTR)
            continue;
          perror("poll");
          server_fd = 0;
          break;
        case 0: /* Nothing interesting going on */
          data_on_stdin = true; /* Can only get here if nonblocking */
          break;
        default:
          /* Make sure to handle the gdb port first! */
          if((fds[0].revents && gdb_fd && !o_serv_fd) ||
             (fds[1].revents && server_fd && gdb_fd))
            {
              int revents = o_serv_fd ? fds[1].revents : fds[0].revents;

              if(revents & POLLIN)
                GDBRequest();
              else /* Error Occurred */
                {
                  fprintf(stderr,"Received flags 0x%08x on gdb socket. Shutting down.\n",revents);
                  close(gdb_fd);
                  gdb_fd = 0;
                }
            }
          if(fds[0].revents && o_serv_fd)
            {
              if(fds[0].revents & POLLIN)
                JTAGRequest();
              else /* Error Occurred */
                {
                  fprintf(stderr,"Received flags 0x%08x on server. Shutting down.\n",fds[0].revents);
                  close(o_serv_fd);
                  server_fd = 0;
                  serverPort = 0;
                  serverIP = 0;
                }
            }
          if(fds[2].revents || (fds[1].revents && !gdb_fd))
            data_on_stdin = true;
          break;
        } /* End of switch statement */
    } /* End of while statement */
}

void JTAGRequest(void)
{
  struct sockaddr_in sa;
  struct sockaddr* addr = (struct sockaddr*)&sa;
  socklen_t len = sizeof(struct sockaddr_in);
  int fd = accept(server_fd,addr,&len);
  int on_off = 0; /* Turn off Nagel's algorithm on the socket */
  int flags;
  char sTemp[256];

  if(fd < 0)
    {
      /* This is valid, because a connection could have started,
         and then terminated due to a protocol error or user
         initiation before the accept could take place. */
      if(errno != EWOULDBLOCK && errno != EAGAIN)
        {
          perror("accept");
          close(server_fd);
          server_fd = 0;
          serverPort = 0;
          serverIP = 0;
        }
      return;
    }

  if(gdb_fd)
    {
      close(fd);
      return;
    }

  if((flags = fcntl(fd,F_GETFL)) < 0)	/* JPB */
    {
      sprintf(sTemp,"Unable to get flags for gdb socket %d",fd);
      perror(sTemp);
      close(fd);
      return;
    }
  
  if(fcntl(fd,F_SETFL, flags | O_NONBLOCK) < 0)
    {
      sprintf(sTemp,"Unable to set flags for gdb socket %d to value 0x%08x",
              fd,flags | O_NONBLOCK);
      perror(sTemp);
      close(fd);
      return;
    }

  if(setsockopt(fd,tcp_level,TCP_NODELAY,&on_off,sizeof(int)) < 0)
    {
      sprintf(sTemp,"Unable to disable Nagel's algorithm for socket %d.\nsetsockopt",fd);
      perror(sTemp);
      close(fd);
      return;
    }

  gdb_fd = fd;
}

void GDBRequest(void)
{
  JTAGProxyWriteMessage msg_write;
  JTAGProxyReadMessage msg_read;
  JTAGProxyChainMessage msg_chain;
  JTAGProxyWriteResponse resp_write;
  JTAGProxyReadResponse resp_read;
  JTAGProxyChainResponse resp_chain;
  JTAGProxyBlockWriteMessage *msg_bwrite;
  JTAGProxyBlockReadMessage msg_bread;
  JTAGProxyBlockWriteResponse resp_bwrite;
  JTAGProxyBlockReadResponse *resp_bread;
  char *buf;
  int err = 0;
  uint32_t command,length;
  int len,i;

  /* First, we must read the incomming command */
  if(gdb_read(&command,sizeof(uint32_t)) < 0)
    {
      if(gdb_fd)
        {
          perror("gdb socket - 1");
          close(gdb_fd);
          gdb_fd = 0;
        }
      return;
    }
  if(gdb_read(&length,sizeof(uint32_t)) < 0)
    {
      if(gdb_fd)
        {
          perror("gdb socket - 2");
          close(gdb_fd);
          gdb_fd = 0;
        }
      return;
    }
  length = ntohl(length);

  /* Now, verify the protocol and implement the command */
  switch(ntohl(command))
    {
    case JTAG_COMMAND_WRITE:
      if(length != sizeof(msg_write) - 8)
        {
          ProtocolClean(length,JTAG_PROXY_PROTOCOL_ERROR);
          return;
        }
      buf = (char*)&msg_write;
      if(gdb_read(&buf[8],length) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 3");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      msg_write.address = ntohl(msg_write.address);
      msg_write.data_H = ntohl(msg_write.data_H);
      msg_write.data_L = ntohl(msg_write.data_L);
      err = DebugSetRegister(msg_write.address,msg_write.data_L);
      resp_write.status = htonl(err);
      if(gdb_write(&resp_write,sizeof(resp_write)) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 4");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      break;
    case JTAG_COMMAND_READ:
      if(length != sizeof(msg_read) - 8)
        {
          ProtocolClean(length,JTAG_PROXY_PROTOCOL_ERROR);
          return;
        }
      buf = (char*)&msg_read;
      if(gdb_read(&buf[8],length) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 5");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      msg_read.address = ntohl(msg_read.address);
      err = DebugGetRegister(msg_read.address,&resp_read.data_L);
      resp_read.status = htonl(err);
      resp_read.data_H = 0;
      resp_read.data_L = htonl(resp_read.data_L);
      if(gdb_write(&resp_read,sizeof(resp_read)) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 6");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      break;
    case JTAG_COMMAND_BLOCK_WRITE:
      if(length < sizeof(JTAGProxyBlockWriteMessage)-8)
        {
          ProtocolClean(length,JTAG_PROXY_PROTOCOL_ERROR);
          return;
        }
      if(!(buf = (char*)malloc(8+length)))
        {
          ProtocolClean(length,JTAG_PROXY_OUT_OF_MEMORY);
          return;
        }
      msg_bwrite = (JTAGProxyBlockWriteMessage*)buf;
      if(gdb_read(&buf[8],length) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 5");
              close(gdb_fd);
              gdb_fd = 0;
            }
          free(buf);
          return;
        }
      msg_bwrite->address = ntohl(msg_bwrite->address);
      msg_bwrite->nRegisters = ntohl(msg_bwrite->nRegisters);
      for(i=0;i<msg_bwrite->nRegisters;i++)
        {
          int t_err = 0;

          msg_bwrite->data[i] = ntohl(msg_bwrite->data[i]);
          t_err = DebugSetRegister(msg_bwrite->address + 4 * i,msg_bwrite->data[i]);
          err = err ? err : t_err;
        }
      resp_bwrite.status = htonl(err);
      free(buf);
      buf = NULL;
      msg_bwrite = NULL;
      if(gdb_write(&resp_bwrite,sizeof(resp_bwrite)) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 4");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      break;
    case JTAG_COMMAND_BLOCK_READ:
      if(length != sizeof(msg_bread) - 8)
        {
          ProtocolClean(length,JTAG_PROXY_PROTOCOL_ERROR);
          return;
        }
      buf = (char*)&msg_bread;
      if(gdb_read(&buf[8],length) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 5");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      msg_bread.address = ntohl(msg_bread.address);
      msg_bread.nRegisters = ntohl(msg_bread.nRegisters);
      len = sizeof(JTAGProxyBlockReadResponse) + 4*(msg_bread.nRegisters-1);
      if(!(buf = (char*)malloc(len)))
        {
          ProtocolClean(0,JTAG_PROXY_OUT_OF_MEMORY);
          return;
        }
      resp_bread = (JTAGProxyBlockReadResponse*)buf;
      for(i=0;i<msg_bread.nRegisters;i++)
        {
          int t_err;

          t_err = DebugGetRegister(msg_bread.address + 4 * i,&resp_bread->data[i]);
          resp_bread->data[i] = htonl(resp_bread->data[i]);
          err = err ? err : t_err;
        }
      resp_bread->status = htonl(err);
      resp_bread->nRegisters = htonl(msg_bread.nRegisters);
      if(gdb_write(resp_bread,len) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 6");
              close(gdb_fd);
              gdb_fd = 0;
            }
          free(buf);
          return;
        }
      free(buf);
      buf = NULL;
      resp_bread = NULL;
      break;
    case JTAG_COMMAND_CHAIN:
      if(length != sizeof(msg_chain) - 8)
        {
          ProtocolClean(length,JTAG_PROXY_PROTOCOL_ERROR);
          return;
        }
      buf = (char*)&msg_chain;
      if(gdb_read(&buf[8],sizeof(msg_chain)-8) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 7");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      msg_chain.chain = htonl(msg_chain.chain);
      err = DebugSetChain(msg_chain.chain);
      resp_chain.status = htonl(err);
      if(gdb_write(&resp_chain,sizeof(resp_chain)) < 0)
        {
          if(gdb_fd)
            {
              perror("gdb socket - 8");
              close(gdb_fd);
              gdb_fd = 0;
            }
          return;
        }
      break;
    default:
      ProtocolClean(length,JTAG_PROXY_COMMAND_NOT_IMPLEMENTED);
      break;
    }
}

void ProtocolClean(int length,int32_t err)
{
  char buf[4096];

  err = htonl(err);
  if((gdb_read(buf,length) < 0) ||
      ((gdb_write(&err,sizeof(err)) < 0) && gdb_fd))
    {
      perror("gdb socket - 9");
      close(gdb_fd);
      gdb_fd = 0;
    }
}

static int gdb_write(const void* buf,int len)
{
  int n, log_n = 0;
  const char* w_buf = (const char*)buf;
  const uint32_t* log_buf = (const uint32_t*)buf;
  struct pollfd block;

  while(len) {
    if((n = write(gdb_fd,w_buf,len)) < 0) {
      switch(errno) {
      case EWOULDBLOCK: /* or EAGAIN */
        /* We've been called on a descriptor marked
           for nonblocking I/O. We better simulate
           blocking behavior. */
        block.fd = gdb_fd;
        block.events = POLLOUT;
        block.revents = 0;
        poll(&block,1,-1);
        continue;
      case EINTR:
        continue;
      case EPIPE:
        close(gdb_fd);
        gdb_fd = 0;
        return -1;
      default:
        return -1;
      }
    }
    else {
      len -= n;
      w_buf += n;
      if ( config.debug.vapi_id )
        for ( log_n += n; log_n >= 4; log_n -= 4, ++ log_buf )
          vapi_write_log_file( VAPI_COMMAND_SEND, config.debug.vapi_id, ntohl(*log_buf) );
    }
  }
  return 0;
}

static int gdb_read(void* buf,int len)
{
  int n, log_n = 0;
  char* r_buf = (char*)buf;
  uint32_t* log_buf = (uint32_t*)buf;
  struct pollfd block;

  while(len) {
    if((n = read(gdb_fd,r_buf,len)) < 0) {
      switch(errno) {
      case EWOULDBLOCK: /* or EAGAIN */
        /* We've been called on a descriptor marked
           for nonblocking I/O. We better simulate
           blocking behavior. */
        block.fd = gdb_fd;
        block.events = POLLIN;
        block.revents = 0;
        poll(&block,1,-1);
        continue;
      case EINTR:
        continue;
      default:
        return -1;
      }
    }
    else if(n == 0) {
      close(gdb_fd);
      gdb_fd = 0;
      return -1;
    }
    else {
      len -= n;
      r_buf += n;
      if ( config.debug.vapi_id )
        for ( log_n += n; log_n >= 4; log_n -= 4, ++ log_buf )
          vapi_write_log_file( VAPI_COMMAND_REQUEST, config.debug.vapi_id, ntohl(*log_buf) );
    }
  }
  return 0;
}

void gdbcomm_init (void)
{
  serverPort = config.debug.server_port;
  if((server_fd = GetServerSocket("or1ksim","tcp",serverPort)))
    PRINTF("JTAG Proxy server started on port %d\n",serverPort);
  else
    PRINTF("Cannot start JTAG proxy server on port %d\n", serverPort);
}
