﻿//
//  Header.h
//  Mapport
//
//  Created by MoK on 15-3-4.
//  Copyright (c) 2015年 MoK. All rights reserved.
//

#ifndef _MY_INCLUDE_H
#define _MY_INCLUDE_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <resolv.h>
#include <syslog.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <net/ethernet.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netpacket/packet.h>

#define X86_32bit
//#define X86_64bit


#ifdef X86_32bit
///////////////////////////////////////////////////////////////////////////////////////////
//数据类型定义
///////////////////////////////////////////////////////////////////////////////////////////
typedef unsigned char 		HB_U8;
typedef unsigned short		HB_U16;
typedef unsigned int		HB_U32;
typedef unsigned long long	HB_U64;
typedef signed char		HB_S8;
typedef short			HB_S16;
typedef int				HB_S32;
typedef long long		HB_S64;
typedef char		HB_CHAR;
typedef float		HB_FLOAT;
typedef void		HB_VOID;
typedef void *		HB_HANDLE;
#endif

#ifdef X86_64bit
///////////////////////////////////////////////////////////////////////////////////////////
//数据类型定义
///////////////////////////////////////////////////////////////////////////////////////////
typedef char		HB_CHAR;
typedef unsigned char	HB_U8;
typedef short		HB_S16;
typedef unsigned short	HB_U16;
typedef int			HB_S32;
typedef unsigned int	HB_U32;
typedef unsigned long	HB_U64;
typedef signed char		HB_S8;
typedef long		HB_S64;
typedef float		HB_FLOAT;
typedef void		HB_VOID;
typedef void *		HB_HANDLE;
#endif

typedef enum _tagHB_BOOL
{
    HB_FALSE = 0,
    HB_TRUE  = 1
}HB_BOOL;


#define HB_NULL     0L
#define HB_SUCCESS  0
#define HB_FAILURE  -1

#define TRUE    1
#define FALSE   0

#define DEBUG
#ifdef DEBUG
#define COLOR_STR_NONE          "\033[0m"
#define COLOR_STR_RED              "\033[1;31m"
#define COLOR_STR_GREEN         "\033[1;32m"
#define COLOR_STR_YELLOW      "\033[1;33m"
#define COLOR_STR_BLUE		     "\033[0;32;34m"

#define TRACE_LOG(str, args...)  printf(COLOR_STR_GREEN  "\n########   FILE:%s  FUNCTION: %s" "\n" str "\n" COLOR_STR_NONE,__FILE__, __FUNCTION__,## args);
#define TRACE_ERR(str, args...)  printf(COLOR_STR_RED "\n%s:%d########" str COLOR_STR_NONE "\n",__FILE__, __LINE__,## args);
#define TRACE_DBG(str, args...)  printf(COLOR_STR_YELLOW  "\n########   FILE:%s  FUNCTION: %s" "\n" str "\n" COLOR_STR_NONE,__FILE__, __FUNCTION__, ## args);
#define TRACE_GREEN(str, args...)  printf(COLOR_STR_GREEN "%s:%d########" str  COLOR_STR_NONE "\n",__FILE__, __LINE__, ## args);
#define TRACE_YELLOW(str, args...)  printf(COLOR_STR_YELLOW "%s:%d########" str  COLOR_STR_NONE "\n",__FILE__, __LINE__, ## args);
#define TRACE_BLUE(str, args...)  printf(COLOR_STR_BLUE "%s:%d########" str  COLOR_STR_NONE "\n",__FILE__, __LINE__, ## args);


#else
#define TRACE_LOG(str, args...)   do{} while(0)
#define TRACE_ERR(str, args...)    do{} while(0)
#define TRACE_DBG(str, args...)   do{} while(0)
#endif /* ERR_DEBUG */

#endif
