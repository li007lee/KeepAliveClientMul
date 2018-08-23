/*
 * cmd_ctrl.h
 *
 *  Created on: Aug 9, 2018
 *      Author: root
 */

#ifndef SRC_HEARTBEAT_H_
#define SRC_HEARTBEAT_H_

#include "common_args.h"

HB_VOID send_heartbeat(evutil_socket_t fd, short events, HB_HANDLE hArg);

#endif /* SRC_HEARTBEAT_H_ */
