/*
 * master.h
 *
 *  Created on: Aug 20, 2018
 *      Author: root
 */

#ifndef SRC_MASTER_H_
#define SRC_MASTER_H_

#include "common_args.h"

HB_VOID connect_to_master_server_cb(struct bufferevent *pConnectToMasterServerBev, HB_S16 what, HB_HANDLE hArg);
HB_VOID func_connect_to_master_server_event(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg);
HB_VOID recv_msg_from_master_server_cb(struct bufferevent *pbevConnectToMasterServer, HB_HANDLE hArg);

#endif /* SRC_MASTER_H_ */
