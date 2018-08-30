/*
 * cmd_ctrl.c
 *
 *  Created on: Aug 9, 2018
 *      Author: root
 */

#include "heartbeat.h"

HB_VOID func_connect_to_slave_server_event(evutil_socket_t fd, short events, void *hArg);
HB_VOID recv_msg_from_slave_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg);

static HB_VOID send_heartbate_err_cb(struct bufferevent *pConnectToKeepAliveServerBev, HB_S16 what, HB_HANDLE hArg)
{
	HB_S32 err = EVUTIL_SOCKET_ERROR();
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

	if (what & BEV_EVENT_TIMEOUT)//超时
	{
		TRACE_ERR("\n###########  send_heartbate_err_cb(%d) : send heartbeat  timeout(%s) !\n", err, evutil_socket_error_to_string(err));
	}
	else if (what & BEV_EVENT_ERROR)  //错误
	{
		TRACE_ERR("\n###########  send_heartbate_err_cb(%d) : send heartbeat failed(%s) !\n", err, evutil_socket_error_to_string(err));
	}
	else if (what & BEV_EVENT_EOF) //对端主动关闭
	{
		TRACE_YELLOW("\n###########  send_heartbate_err_cb(%d) : connection closed by peer(%s)!\n", err, evutil_socket_error_to_string(err));
	}
	else
	{
		TRACE_ERR("\n###########  send_heartbate_err_cb(%d) : (%s) !\n", err, evutil_socket_error_to_string(err));
	}

	event_del(&pEventArgs->stHeartBeatTimerEvent);
	if (NULL != pEventArgs->pConnectToKeepAliveServerBev)
	{
		bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
		pEventArgs->pConnectToKeepAliveServerBev = NULL;
	}

	struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
	event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_slave_server_event, pEventArgs);
	event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
	printf("send_heartbate_err_cb() return !\n");
}

HB_VOID send_heartbeat(evutil_socket_t fd, short events, HB_HANDLE hArg)
{
//	printf("curtain time : %ld.\n", time(NULL));
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;
	struct bufferevent *pConnectToKeepAliveServerBev = pEventArgs->pConnectToKeepAliveServerBev;
	HB_CHAR cSendBuf[512] = {0};
	CMD_HEADER_OBJ stCmdHeader;
	memset(&stCmdHeader, 0, sizeof(CMD_HEADER_OBJ));

	stCmdHeader.cmd_format = E_CMD_JSON;
	stCmdHeader.cmd_func = E_BOX_KEEP_ALIVE;
	strncpy(stCmdHeader.header, "BoxKeepAlive@yDt", 16);
	snprintf(cSendBuf+sizeof(CMD_HEADER_OBJ), sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), "{\"HeartBeat\":\"keep_alive\",\"DevId\":\"%s\"}", pEventArgs->cDevId);
	stCmdHeader.cmd_length = strlen(cSendBuf+sizeof(CMD_HEADER_OBJ));
	memcpy(cSendBuf, &stCmdHeader, sizeof(CMD_HEADER_OBJ));

	bufferevent_setcb(pConnectToKeepAliveServerBev, recv_msg_from_slave_server_cb, NULL, send_heartbate_err_cb, pEventArgs);
	bufferevent_enable(pConnectToKeepAliveServerBev, EV_READ);
	bufferevent_write(pConnectToKeepAliveServerBev, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	struct timeval tvRecvTimeOut = {5, 0}; //设置读超时
	bufferevent_set_timeouts(pConnectToKeepAliveServerBev, &tvRecvTimeOut, NULL);
}
