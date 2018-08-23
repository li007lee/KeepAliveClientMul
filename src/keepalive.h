/*
 * keepalive.h
 *
 *  Created on: Jul 16, 2018
 *      Author: root
 */

#ifndef SRC_KEEPALIVE_H_
#define SRC_KEEPALIVE_H_

#include "event2/event_struct.h"


typedef struct _tagTHREAD_ARGS
{
	struct event_base *pEventBase;
	pthread_t threadSelfId;
	HB_S32 iClientNums;
	HB_S32 iKeepAliveServerPort; //长连接服务器ip
}THREAD_ARGS_OBJ, *THREAD_ARGS_HANDLE;

HB_VOID *start_keep_alive_base(HB_HANDLE hArg);

#endif /* SRC_KEEPALIVE_H_ */
