/*
 * slave.h
 *
 *  Created on: Aug 20, 2018
 *      Author: root
 */

#ifndef SRC_SLAVE_H_
#define SRC_SLAVE_H_

#include "common_args.h"

HB_VOID connect_to_slave_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg);
HB_VOID func_connect_to_slave_server_event(evutil_socket_t fd, short events, void *hArg);
HB_VOID recv_msg_from_slave_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg);

#endif /* SRC_SLAVE_H_ */
