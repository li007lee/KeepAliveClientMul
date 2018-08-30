/*
 * keepalive.c
 *
 *  Created on: Jul 16, 2018
 *      Author: root
 */

#include "common_args.h"
#include "cJSON.h"
#include "keepalive.h"
#include "master.h"

//extern HB_CHAR cLocalIp[16];

HB_VOID connect_to_master_server_cb(struct bufferevent *pConnectToMasterServerBev, HB_S16 what, HB_HANDLE hArg);
HB_VOID recv_msg_from_master_server_cb(struct bufferevent *pbevConnectToMasterServer, HB_HANDLE hArg);

#if 0
HB_VOID recv_msg_from_keepalive_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg);
static HB_VOID connect_to_keepalive_server(evutil_socket_t fd, short events, void *arg);
HB_VOID connect_to_master_server(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg);

/************************************************************/
/*********************连接长连接服务器slave********************/
/************************************************************/

HB_VOID connect_to_keepalive_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

	if (what & BEV_EVENT_CONNECTED)
	{
		pEventArgs->pConnectToKeepAliveServerBev = pbevConnectToKeepAliveServer;
		pEventArgs->iConnectToKeepAliveServerErrTimes = 0;
		HB_CHAR cSendBuf[512] = {0};
		CMD_HEADER_OBJ stCmdHeader;
		memset(&stCmdHeader, 0, sizeof(CMD_HEADER_OBJ));
		stCmdHeader.cmd_format = E_CMD_JSON;
		stCmdHeader.cmd_func = E_BOX_REGIST_ALIVE_SERVER;
		strncpy(stCmdHeader.header, "BoxKeepAlive@yDt", 16);

		snprintf(cSendBuf+sizeof(CMD_HEADER_OBJ), sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), \
						"{\"DevType\":\"ydt_box\",\"DevModel\":\"TM-X01-A\",\"DevId\":\"%s\",\"DevVer\":\"3.0.0.1\"}",\
						pEventArgs->cDevId);
		stCmdHeader.cmd_length = strlen(cSendBuf+sizeof(CMD_HEADER_OBJ));
		memcpy(cSendBuf, &stCmdHeader, sizeof(CMD_HEADER_OBJ));

		//连接成功发送注册消息
		bufferevent_write(pbevConnectToKeepAliveServer, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	}
	else
	{
		bufferevent_free(pbevConnectToKeepAliveServer);
		pbevConnectToKeepAliveServer = NULL;

		pEventArgs->iConnectToKeepAliveServerErrTimes += 1;
		if(pEventArgs->iConnectToKeepAliveServerErrTimes >= 3)
		{
			pEventArgs->iConnectToKeepAliveServerErrTimes = 0;
			TRACE_ERR("Can't connect to keep alive server, get new keep alive server...\n");
			struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_master_server, pEventArgs);
			event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
			printf("Can't connect to keep alive server, get new keep alive server... End!\n");
		}
		else
		{
			printf("connect to keep alive server failed! reconnect !\n");
			struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_keepalive_server, pEventArgs);
			event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
			printf("connect to keep alive server failed! reconnect End!\n");
		}
	}
}

static HB_VOID connect_to_keepalive_server(evutil_socket_t fd, short events, void *hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

//	if (NULL != pEventArgs->pConnectToKeepAliveServerEvent)
//	{
//		printf("Del pConnectToKeepAliveServerEvent\n");
//		event_del(pEventArgs->pConnectToKeepAliveServerEvent);
//		event_free(pEventArgs->pConnectToKeepAliveServerEvent);
//		pEventArgs->pConnectToKeepAliveServerEvent = NULL;
//	}

	HB_S32 iConnectToKeepAliveServerFd = socket(AF_INET,SOCK_STREAM,0);//返回-1表示失败;
	struct sockaddr_in stConnectToKeepAliveServerAddr1;
	HB_S32 iSize = sizeof(struct sockaddr_in);

	bzero(&stConnectToKeepAliveServerAddr1,sizeof(stConnectToKeepAliveServerAddr1));
	stConnectToKeepAliveServerAddr1.sin_family = AF_INET;
	stConnectToKeepAliveServerAddr1.sin_port = htons(0); //服务器绑定的端口
	stConnectToKeepAliveServerAddr1.sin_addr.s_addr = inet_addr(cLocalIp);//服务器的IP地址
	bind(iConnectToKeepAliveServerFd,(struct sockaddr*)&stConnectToKeepAliveServerAddr1,iSize);//返回-1表示失败

	struct bufferevent *pbevConnectToKeepAliveServer = bufferevent_socket_new(pEventArgs->pEventBase, iConnectToKeepAliveServerFd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
	struct sockaddr_in stConnectToKeepAliveServerAddr;
	bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
	stConnectToKeepAliveServerAddr.sin_family = AF_INET;
	stConnectToKeepAliveServerAddr.sin_port = htons(SLAVE_SERVER_KEEP_ALAVE_PORT);
	inet_pton(AF_INET, pEventArgs->cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
	printf("connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, SLAVE_SERVER_KEEP_ALAVE_PORT);

	bufferevent_setcb(pbevConnectToKeepAliveServer, recv_msg_from_keepalive_server_cb, NULL, connect_to_keepalive_server_cb, pEventArgs);
	if (bufferevent_socket_connect(pbevConnectToKeepAliveServer, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in)) < 0)
	{
		bufferevent_free(pbevConnectToKeepAliveServer);
		pbevConnectToKeepAliveServer = NULL;
		struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_keepalive_server, pEventArgs);
		event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
		return ;
	}
	if (bufferevent_enable(pbevConnectToKeepAliveServer, EV_READ | EV_WRITE) < 0)
	{
		bufferevent_free(pbevConnectToKeepAliveServer);
		pbevConnectToKeepAliveServer = NULL;
		struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_keepalive_server, pEventArgs);
		event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
		return ;
	}
	struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
	bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, &tvConnectTimeOut);
}

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

//	if(NULL != pEventArgs->pHeartBeatTimerEvent)
//	{
//		event_del(pEventArgs->pHeartBeatTimerEvent);
//		event_free(pEventArgs->pHeartBeatTimerEvent);
//		pEventArgs->pHeartBeatTimerEvent = NULL;
//	}

	if (NULL != pEventArgs->pConnectToKeepAliveServerBev)
	{
		bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
		pEventArgs->pConnectToKeepAliveServerBev = NULL;
	}

	event_del(&pEventArgs->stHeartBeatTimerEvent);
	struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
	event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_keepalive_server, pEventArgs);
	event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
	printf("send_heartbate_err_cb() return !\n");
}

static HB_VOID send_heartbeat(evutil_socket_t fd, short events, HB_HANDLE hArg)
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

	bufferevent_setcb(pConnectToKeepAliveServerBev, recv_msg_from_keepalive_server_cb, NULL, send_heartbate_err_cb, pEventArgs);
	bufferevent_write(pConnectToKeepAliveServerBev, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	bufferevent_enable(pConnectToKeepAliveServerBev, EV_READ | EV_WRITE);
	struct timeval tvRecvTimeOut = {5, 0}; //设置读超时
	bufferevent_set_timeouts(pConnectToKeepAliveServerBev, &tvRecvTimeOut, NULL);
}

//用于接收注册成功或失败的结果信息
HB_VOID recv_msg_from_keepalive_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg)
{
	CMD_HEADER_OBJ stCmdHeader;
	HB_CHAR cRecvBuf[1024] = { 0 };
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;
	HB_S32 iErrCode = 0;

	for (;;)
	{
		struct evbuffer *src = bufferevent_get_input(pbevConnectToKeepAliveServer);//获取输入缓冲区
		HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。

		if(len < sizeof(CMD_HEADER_OBJ))
		{
//			printf("data small than head len! continue ...\n");
			return;
		}

		evbuffer_copyout(src, (void*)(&stCmdHeader), sizeof(CMD_HEADER_OBJ));
		if (strncmp(stCmdHeader.header, "BoxKeepAlive@yDt", 16) != 0)
		{
			//头消息错误，直接返回
			TRACE_ERR("st_MsgHead.header error recv:[%s]\n", stCmdHeader.header);
			return ;
		}

		if(evbuffer_get_length(src) < (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)))
		{
			printf("\n2222222222recv len=%d   msg_len=%d\n", (HB_S32)evbuffer_get_length(src), (HB_S32)(stCmdHeader.cmd_length));
			return;
		}
		bufferevent_read(pbevConnectToKeepAliveServer, (HB_VOID*)(cRecvBuf), (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)));

		switch(stCmdHeader.cmd_func)
		{
			case E_BOX_REGIST_ALIVE_SERVER_REPLY:
			{
				cJSON *pRoot;
				pRoot = cJSON_Parse(cRecvBuf+sizeof(CMD_HEADER_OBJ));
				cJSON *pResult = cJSON_GetObjectItem(pRoot, "Result");
				if (pResult->valueint != 0)
				{
					cJSON *pMsg = cJSON_GetObjectItem(pRoot, "Msg");
					TRACE_ERR("E_BOX_REGIST_ALIVE_SERVER_REPLY regist failed : [%s]\n", pMsg->valuestring);
					cJSON_Delete(pRoot);
					iErrCode = -1;
					break;
				}
				cJSON_Delete(pRoot);
				TRACE_LOG("Registe to Slave succeed! id=[%s]\n", pEventArgs->cDevId);
	//			注册成功，创建定时器，定时发送心跳包
				struct timeval tv = {HEARTBATE_TIME_VAL, 0};
				event_assign(&pEventArgs->stHeartBeatTimerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT | EV_PERSIST, send_heartbeat, pEventArgs);
				event_add(&pEventArgs->stHeartBeatTimerEvent, &tv);
				TRACE_LOG("Start send_heartbeat succeed! id=[%s] End\n", pEventArgs->cDevId);

			}
			break;
			case E_BOX_KEEP_ALIVE_REPLY:
			{
				cJSON *pRoot;
				pRoot = cJSON_Parse(cRecvBuf+sizeof(CMD_HEADER_OBJ));
				cJSON *pResult = cJSON_GetObjectItem(pRoot, "Result");
				if (pResult->valueint != 0)
				{
					cJSON *pMsg = cJSON_GetObjectItem(pRoot, "Msg");
					TRACE_ERR("E_BOX_KEEP_ALIVE_REPLY recv heartbeat failed : [%s]\n", pMsg->valuestring);
				}
				cJSON_Delete(pRoot);
//				printf("E_BOX_KEEP_ALIVE_REPLY recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
				bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, NULL);
			}
			break;
			default:
				TRACE_ERR("recv_msg_from_keepalive_server_cb default recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
				iErrCode = -3;
				break;
		}

		if (iErrCode < 0)
		{
			TRACE_ERR("recv_msg_from_keepalive_server_cb recv err cmd : [%d]\n", iErrCode);
			if (NULL != pEventArgs->pConnectToKeepAliveServerBev)
			{
				bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
				pEventArgs->pConnectToKeepAliveServerBev = NULL;
			}
			struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_keepalive_server, pEventArgs);
			event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
			TRACE_ERR("recv_msg_from_keepalive_server_cb recv err cmd : [%d] END\n", iErrCode);
		}
	}
}

/************************************************************/
/*********************连接长连接服务器slave********************/
/************************************************************/


/************************************************************/
/*********************连接调度服务器master*********************/
/************************************************************/
//用于接收注册成功或失败的结果信息
static HB_VOID recv_msg_from_master_server_cb(struct bufferevent *pbevConnectToMasterServer, HB_HANDLE hArg)
{
	CMD_HEADER_OBJ stCmdHeader;
	HB_CHAR cRecvBuf[1024] = { 0 };
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;
	HB_S32 iErrCode = 0;
#if 1
	struct evbuffer *src = bufferevent_get_input(pbevConnectToMasterServer);//获取输入缓冲区
	HB_S32 len = evbuffer_get_length(src);//获取输入缓冲区中数据的长度，也就是可以读取的长度。
	if(len < sizeof(CMD_HEADER_OBJ))
	{
		printf("data small than head len! continue ...\n");
		return;
	}

	evbuffer_copyout(src, (void*)(&stCmdHeader), sizeof(CMD_HEADER_OBJ));
	if (strncmp(stCmdHeader.header, "BoxKeepAlive@yDt", 16) != 0)
	{
		//头消息错误，直接返回
		TRACE_ERR("st_MsgHead.header error recv:[%s]\n", stCmdHeader.header);
		return ;
	}

	if(evbuffer_get_length(src) < (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)))
	{
		printf("\n2222222222recv len=%d   msg_len=%d\n", (HB_S32)evbuffer_get_length(src), (HB_S32)(stCmdHeader.cmd_length));
		return;
	}

	bufferevent_read(pbevConnectToMasterServer, (HB_VOID*)(cRecvBuf), (stCmdHeader.cmd_length + sizeof(CMD_HEADER_OBJ)));
	bufferevent_free(pbevConnectToMasterServer);
	pbevConnectToMasterServer = NULL;

	switch(stCmdHeader.cmd_func)
	{
		case E_MASTER_DEV_GIVE_ME_SLAVE_REPLY:
		{
			cJSON *pRoot;
			pRoot = cJSON_Parse(cRecvBuf+sizeof(CMD_HEADER_OBJ));
			cJSON *pSlaveIp = cJSON_GetObjectItem(pRoot, "SlaveIp");
			if (NULL != pSlaveIp)
			{
				memset(pEventArgs->cKeepAliveServerIp, 0, sizeof(pEventArgs->cKeepAliveServerIp));
				strncpy(pEventArgs->cKeepAliveServerIp, pSlaveIp->valuestring, sizeof(pEventArgs->cKeepAliveServerIp));
			}
			else
			{
				cJSON_Delete(pRoot);
				TRACE_ERR("从Master获取Slave失败!\n");
				iErrCode = -1;
				break;
			}

			cJSON_Delete(pRoot);
			TRACE_LOG("从Master获取Slave地址成功 recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
//				struct timeval tv = {0, 0};
//				event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, 0, connect_to_keepalive_server, pEventArgs);
//				event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);

			HB_S32 iConnectToKeepAliveServerFd = socket(AF_INET,SOCK_STREAM,0);//返回-1表示失败;
			struct sockaddr_in stConnectToKeepAliveServerAddr1;
			HB_S32 iSize = sizeof(struct sockaddr_in);

			bzero(&stConnectToKeepAliveServerAddr1,sizeof(stConnectToKeepAliveServerAddr1));
			stConnectToKeepAliveServerAddr1.sin_family = AF_INET;
			stConnectToKeepAliveServerAddr1.sin_port = htons(0); //服务器绑定的端口
			stConnectToKeepAliveServerAddr1.sin_addr.s_addr = inet_addr(cLocalIp);//服务器的IP地址
			bind(iConnectToKeepAliveServerFd,(struct sockaddr*)&stConnectToKeepAliveServerAddr1,iSize);//返回-1表示失败

			struct bufferevent *pConnectToKeepAliveServerBev = bufferevent_socket_new(pEventArgs->pEventBase, iConnectToKeepAliveServerFd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
			struct sockaddr_in stConnectToKeepAliveServerAddr;
			bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
			stConnectToKeepAliveServerAddr.sin_family = AF_INET;
//			stConnectToKeepAliveServerAddr.sin_port = htons(SLAVE_SERVER_KEEP_ALAVE_PORT);
			stConnectToKeepAliveServerAddr.sin_port = htons(pEventArgs->iKeepAliveServerPort);
			inet_pton(AF_INET, pEventArgs->cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
			printf("connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);

			bufferevent_setcb(pConnectToKeepAliveServerBev, recv_msg_from_keepalive_server_cb, NULL, connect_to_keepalive_server_cb, pEventArgs);
			if (bufferevent_socket_connect(pConnectToKeepAliveServerBev, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in)) < 0)
			{
				bufferevent_free(pConnectToKeepAliveServerBev);
				pConnectToKeepAliveServerBev = NULL;
				iErrCode = -3;
				break;
			}
			if (bufferevent_enable(pConnectToKeepAliveServerBev, EV_READ | EV_WRITE) < 0)
			{
				bufferevent_free(pConnectToKeepAliveServerBev);
				pConnectToKeepAliveServerBev = NULL;
				iErrCode = -4;
				break;
			}
			struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
			bufferevent_set_timeouts(pConnectToKeepAliveServerBev, NULL, &tvConnectTimeOut);
			printf("------------>connect to slave ServerIp[%s], ServerPort[%d] End!\n", pEventArgs->cKeepAliveServerIp, SLAVE_SERVER_KEEP_ALAVE_PORT);
		}
		break;
		default:
			TRACE_ERR("default recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
			iErrCode = -5;
			break;
	}

	if (iErrCode < 0)
	{
		TRACE_ERR("iErrCode iErrCode iErrCode iErrCode iErrCode ======= %d\n", iErrCode);
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_master_server, pEventArgs);
		event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
		TRACE_ERR("iErrCode iErrCode iErrCode iErrCode iErrCode ======= %d  End\n", iErrCode);
		return ;
	}
#endif
}

static HB_VOID connect_to_master_server_cb(struct bufferevent *pConnectToMasterServerBev, HB_S16 what, HB_HANDLE hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;
	if (what & BEV_EVENT_CONNECTED)
	{
		HB_CHAR cSendBuf[512] = {0};
		CMD_HEADER_OBJ stCmdHeader;
		memset(&stCmdHeader, 0, sizeof(CMD_HEADER_OBJ));
		stCmdHeader.cmd_format = E_CMD_JSON;
		stCmdHeader.cmd_func = E_MASTER_DEV_GIVE_ME_SLAVE;
		strncpy(stCmdHeader.header, "BoxKeepAlive@yDt", 16);
		snprintf(cSendBuf+sizeof(CMD_HEADER_OBJ), sizeof(cSendBuf)-sizeof(CMD_HEADER_OBJ), "{\"DevId\":\"%s\"}", pEventArgs->cDevId);

		stCmdHeader.cmd_length = strlen(cSendBuf+sizeof(CMD_HEADER_OBJ));
		memcpy(cSendBuf, &stCmdHeader, sizeof(CMD_HEADER_OBJ));

		printf("++++++++++++++++++++++++Send to Master : [%s]\n", cSendBuf+sizeof(CMD_HEADER_OBJ));
		//连接成功发送注册消息
		bufferevent_write(pConnectToMasterServerBev, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	}
	else
	{
		printf("连接Master失败! 重连 !\n");
		bufferevent_free(pConnectToMasterServerBev);
		pConnectToMasterServerBev = NULL;
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_master_server, pEventArgs);
		event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
		printf("连接Master失败! 重连 End!\n");
	}
}

HB_VOID connect_to_master_server(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

//	if(NULL != pEventArgs->pConnectToMasterServerEvent)
//	{
//		event_del(pEventArgs->pConnectToMasterServerEvent);
//		event_free(pEventArgs->pConnectToMasterServerEvent);
//		pEventArgs->pConnectToMasterServerEvent = NULL;
//	}

	struct bufferevent *pConnectToMasterServerBev = bufferevent_socket_new(pEventArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
	struct sockaddr_in stConnectToMasterServerAddr;
	bzero(&stConnectToMasterServerAddr, sizeof(stConnectToMasterServerAddr));
	stConnectToMasterServerAddr.sin_family = AF_INET;
	stConnectToMasterServerAddr.sin_port = htons(MASTER_SERVER_PORT);
	inet_pton(AF_INET, MASTER_SERVER_IP, (void *) &stConnectToMasterServerAddr.sin_addr);
	printf("############ Connect to master ServerIp[%s], ServerPort[%d]\n", MASTER_SERVER_IP, MASTER_SERVER_PORT);

	bufferevent_setcb(pConnectToMasterServerBev, recv_msg_from_master_server_cb, NULL, connect_to_master_server_cb, pEventArgs);
	if (bufferevent_socket_connect(pConnectToMasterServerBev, (struct sockaddr*) &stConnectToMasterServerAddr, sizeof(struct sockaddr_in)) < 0)
	{
		bufferevent_free(pConnectToMasterServerBev);
		pConnectToMasterServerBev = NULL;
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_master_server, pEventArgs);
		event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
		return ;
	}
	if (bufferevent_enable(pConnectToMasterServerBev, EV_READ | EV_WRITE) < 0)
	{
		bufferevent_free(pConnectToMasterServerBev);
		pConnectToMasterServerBev = NULL;
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, connect_to_master_server, pEventArgs);
		event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
		return ;
	}
	struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
	bufferevent_set_timeouts(pConnectToMasterServerBev, NULL, &tvConnectTimeOut);
}

/************************************************************/
/*********************连接调度服务器master*********************/
/************************************************************/
#endif

HB_VOID *keepalive_start(HB_HANDLE hArg)
{
	pthread_detach(pthread_self());
	THREAD_ARGS_HANDLE pThreadArgs = (THREAD_ARGS_HANDLE)hArg;
	//首先连接Master服务器，获取长连接服务器的地址和端口
	int i = 0;
	for (i = 0; i<pThreadArgs->iClientNums; ++i)
	{
//		printf("pThreadArgs->iClientNums = %d\n", i);
		EVENT_ARGS_HANDLE pEventArgs = calloc(1, sizeof(EVENT_ARGS_OBJ));
		if (pEventArgs == NULL)
		{
			i--;
			usleep(10000);
			continue;
		}
		pEventArgs->pConnectToKeepAliveServerBev = NULL;
		snprintf(pEventArgs->cDevId, sizeof(pEventArgs->cDevId), "%ld-%d", pThreadArgs->threadSelfId, i);
		pEventArgs->pEventBase = pThreadArgs->pEventBase;
		pEventArgs->iKeepAliveServerPort = pThreadArgs->iKeepAliveServerPort;

		struct bufferevent *pConnectToMasterServerBev = bufferevent_socket_new(pEventArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
		if (NULL == pConnectToMasterServerBev)
		{
			free(pEventArgs);
			pEventArgs = NULL;
			i--;
			usleep(10000);
			continue;
		}
		struct sockaddr_in stConnectToMasterServerAddr;
		bzero(&stConnectToMasterServerAddr, sizeof(stConnectToMasterServerAddr));
		stConnectToMasterServerAddr.sin_family = AF_INET;
		stConnectToMasterServerAddr.sin_port = htons(MASTER_SERVER_PORT);
		inet_pton(AF_INET, MASTER_SERVER_IP, (void *) &stConnectToMasterServerAddr.sin_addr);
		printf("############ Connect to master ServerIp[%s], ServerPort[%d] DevID[%s\n", MASTER_SERVER_IP, MASTER_SERVER_PORT, pEventArgs->cDevId);

//		bufferevent_setcb(pConnectToMasterServerBev, recv_msg_from_master_server_cb, NULL, connect_to_master_server_cb, pEventArgs);
		bufferevent_setcb(pConnectToMasterServerBev, NULL, NULL, connect_to_master_server_cb, pEventArgs);
		if (bufferevent_enable(pConnectToMasterServerBev, 0) < 0)
		{
			TRACE_ERR("bufferevent_enable() err!");
			free(pEventArgs);
			pEventArgs = NULL;
			bufferevent_free(pConnectToMasterServerBev);
			pConnectToMasterServerBev = NULL;
			i--;
			usleep(10000);
			continue;
		}
		if (bufferevent_socket_connect(pConnectToMasterServerBev, (struct sockaddr*) &stConnectToMasterServerAddr, sizeof(struct sockaddr_in)) < 0)
		{
			TRACE_ERR("bufferevent_socket_connect() err!");
			free(pEventArgs);
			pEventArgs = NULL;
			bufferevent_free(pConnectToMasterServerBev);
			pConnectToMasterServerBev = NULL;
			i--;
			usleep(10000);
			continue;
		}
//		usleep(100);

#if 0


		//		strncpy(pEventArgs->cKeepAliveServerIp, "192.168.118.29", sizeof(pEventArgs->cKeepAliveServerIp));
		strncpy(pEventArgs->cKeepAliveServerIp, "172.16.3.100", sizeof(pEventArgs->cKeepAliveServerIp));
		HB_S32 iConnectToKeepAliveServerFd = socket(AF_INET,SOCK_STREAM,0);//返回-1表示失败;
		struct sockaddr_in stConnectToKeepAliveServerAddr1;
		HB_S32 iSize = sizeof(struct sockaddr_in);

		bzero(&stConnectToKeepAliveServerAddr1,sizeof(stConnectToKeepAliveServerAddr1));
		stConnectToKeepAliveServerAddr1.sin_family = AF_INET;
		stConnectToKeepAliveServerAddr1.sin_port = htons(0); //服务器绑定的端口
		stConnectToKeepAliveServerAddr1.sin_addr.s_addr = inet_addr(cLocalIp);//服务器的IP地址
		bind(iConnectToKeepAliveServerFd,(struct sockaddr*)&stConnectToKeepAliveServerAddr1,iSize);//返回-1表示失败

		struct bufferevent *pbevConnectToKeepAliveServer = bufferevent_socket_new(pEventArgs->pEventBase, iConnectToKeepAliveServerFd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
		struct sockaddr_in stConnectToKeepAliveServerAddr;
		bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
		stConnectToKeepAliveServerAddr.sin_family = AF_INET;
		stConnectToKeepAliveServerAddr.sin_port = htons(pEventArgs->iKeepAliveServerPort);
		inet_pton(AF_INET, pEventArgs->cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
		printf("connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);

		bufferevent_setcb(pbevConnectToKeepAliveServer, recv_msg_from_keepalive_server_cb, NULL, connect_to_keepalive_server_cb, pEventArgs);
		if(bufferevent_socket_connect(pbevConnectToKeepAliveServer, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in)) < 0)
		{
			bufferevent_free(pbevConnectToKeepAliveServer);
			pbevConnectToKeepAliveServer = NULL;
			i--;
			free(pEventArgs);
			pEventArgs = NULL;
			continue;
		}
		if (bufferevent_enable(pbevConnectToKeepAliveServer, EV_READ | EV_WRITE) < 0)
		{
			bufferevent_free(pbevConnectToKeepAliveServer);
			pbevConnectToKeepAliveServer = NULL;
			i--;
			free(pEventArgs);
			pEventArgs = NULL;
			continue;
		}
		struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
		bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, &tvConnectTimeOut);
#endif
	}

	return NULL;
}



//获取start_num与end_num之间的随机数
HB_S32 random_number(HB_U32 threadSelfId, HB_S32 start_num, HB_S32 end_num)
{
	HB_S32 ret_num = 0;
	srand(threadSelfId);
	ret_num = rand() % (end_num - start_num) + start_num;
	return ret_num;
}


static void timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event_base *base = (struct event_base *)arg;
	printf("\n#######   Base:[%p] work thread base   total events = %d   \n", base, event_base_get_num_events(base, EVENT_BASE_COUNT_ADDED));
}

HB_VOID *start_keep_alive_base(HB_HANDLE hArg)
{
	pthread_t threadSelfId = pthread_self();
	pthread_detach(threadSelfId);

	int ret = random_number((HB_U32)threadSelfId, 1, 10);
	printf("sleep time : %d\n", ret);
	sleep(ret);

	HB_S32 iClientNums = *(HB_S32 *)hArg;
	struct event_base *pEventBase = event_base_new();
	if (!pEventBase)
	{
		perror("cmd_base event_base_new()");
		return NULL;
	}
	//用于线程安全
	if(evthread_make_base_notifiable(pEventBase) != 0)
	{
		TRACE_ERR("###### evthread_make_base_notifiable() err!");
		return NULL;
	}

	THREAD_ARGS_OBJ stThreadArgs;
	stThreadArgs.pEventBase = pEventBase;
	stThreadArgs.threadSelfId = threadSelfId;
	stThreadArgs.iClientNums = iClientNums;

	pthread_t threadKeepAliveId = -1;
	pthread_create(&threadKeepAliveId, NULL, keepalive_start, &stThreadArgs);


	struct event timeout;//定时器事件
	struct timeval tv = {10, 0};//定时器值
	event_assign(&timeout, pEventBase, -1, EV_TIMEOUT | EV_PERSIST, timeout_cb, pEventBase);
	event_add(&timeout, &tv);

	event_base_dispatch(pEventBase);
	event_base_free(pEventBase);

	return NULL;
}









#if 0

//HB_VOID keepalive_start(struct event_base *pEventBase, pthread_t threadSelfId, HB_S32 iClientNums)
//{
//	//首先连接Master服务器，获取长连接服务器的地址和端口
//	int i = 0;
//	for (i = 0; i<iClientNums; ++i)
//	{
//		EVENT_ARGS_HANDLE pEventArgs = calloc(1, sizeof(EVENT_ARGS_OBJ));
//		if (pEventArgs == NULL)
//		{
//			i--;
//			continue;
//		}
//		snprintf(pEventArgs->cDevId, sizeof(pEventArgs->cDevId), "%ld-%d", threadSelfId, i);
//		pEventArgs->pEventBase = pEventBase;
//
////		strncpy(pEventArgs->cKeepAliveServerIp, "192.168.118.29", sizeof(pEventArgs->cKeepAliveServerIp));
////		pEventArgs->iKeepAliveServerPort = SLAVE_SERVER_KEEP_ALAVE_PORT;
////		struct timeval tv = {0, 0};
////		pEventArgs->pConnectToKeepAliveServerEvent = event_new(pEventArgs->pEventBase, -1, 0, connect_to_keepalive_server, pEventArgs);
////		event_add(pEventArgs->pConnectToKeepAliveServerEvent, &tv);
//
////		struct timeval tv = {0, 0};
////		pEventArgs->pConnectToMasterServerEvent = event_new(pEventArgs->pEventBase, -1, 0, connect_to_master_server, pEventArgs);
////		event_add(pEventArgs->pConnectToMasterServerEvent, &tv);
//
//
//		struct bufferevent *pbevConnectToMasterServer = bufferevent_socket_new(pEventArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
//		struct sockaddr_in stConnectToMasterServerAddr;
//		bzero(&stConnectToMasterServerAddr, sizeof(stConnectToMasterServerAddr));
//		stConnectToMasterServerAddr.sin_family = AF_INET;
//		stConnectToMasterServerAddr.sin_port = htons(MASTER_SERVER_PORT);
//		inet_pton(AF_INET, MASTER_SERVER_IP, (void *) &stConnectToMasterServerAddr.sin_addr);
//		printf("############ Connect to master ServerIp[%s], ServerPort[%d]\n", MASTER_SERVER_IP, MASTER_SERVER_PORT);
//
//		bufferevent_setcb(pbevConnectToMasterServer, recv_msg_from_master_server_cb, NULL, connect_to_master_server_cb, pEventArgs);
//		bufferevent_socket_connect(pbevConnectToMasterServer, (struct sockaddr*) &stConnectToMasterServerAddr, sizeof(struct sockaddr_in));
//		bufferevent_enable(pbevConnectToMasterServer, EV_READ | EV_WRITE);
//		struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
//		bufferevent_set_timeouts(pbevConnectToMasterServer, NULL, &tvConnectTimeOut);
////		usleep(1000);
//	}
//}


HB_VOID *keepalive_start(HB_HANDLE hArg)
{
	pthread_detach(pthread_self());
	THREAD_ARGS_HANDLE pThreadArgs = (THREAD_ARGS_HANDLE)hArg;
	//首先连接Master服务器，获取长连接服务器的地址和端口
	int i = 0;
	for (i = 0; i<pThreadArgs->iClientNums; ++i)
	{
		EVENT_ARGS_HANDLE pEventArgs = calloc(1, sizeof(EVENT_ARGS_OBJ));
		if (pEventArgs == NULL)
		{
			i--;
			continue;
		}
		snprintf(pEventArgs->cDevId, sizeof(pEventArgs->cDevId), "%ld-%d", pThreadArgs->threadSelfId, i);
		pEventArgs->pEventBase = pThreadArgs->pEventBase;
		pEventArgs->iKeepAliveServerPort = pThreadArgs->iKeepAliveServerPort;


//		strncpy(pEventArgs->cKeepAliveServerIp, "192.168.118.29", sizeof(pEventArgs->cKeepAliveServerIp));
		strncpy(pEventArgs->cKeepAliveServerIp, "172.16.3.100", sizeof(pEventArgs->cKeepAliveServerIp));
//		pEventArgs->iKeepAliveServerPort = SLAVE_SERVER_KEEP_ALAVE_PORT;
//		struct timeval tv = {0, 0};
//		event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, 0, connect_to_keepalive_server, pEventArgs);
//		event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);

		HB_S32 iConnectToKeepAliveServerFd = socket(AF_INET,SOCK_STREAM,0);//返回-1表示失败;
		struct sockaddr_in stConnectToKeepAliveServerAddr1;
		HB_S32 iSize = sizeof(struct sockaddr_in);

		bzero(&stConnectToKeepAliveServerAddr1,sizeof(stConnectToKeepAliveServerAddr1));
		stConnectToKeepAliveServerAddr1.sin_family = AF_INET;
		stConnectToKeepAliveServerAddr1.sin_port = htons(0); //服务器绑定的端口
		stConnectToKeepAliveServerAddr1.sin_addr.s_addr = inet_addr(cSlaveIp);//服务器的IP地址
		bind(iConnectToKeepAliveServerFd,(struct sockaddr*)&stConnectToKeepAliveServerAddr1,iSize);//返回-1表示失败

		struct bufferevent *pbevConnectToKeepAliveServer = bufferevent_socket_new(pEventArgs->pEventBase, iConnectToKeepAliveServerFd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
		struct sockaddr_in stConnectToKeepAliveServerAddr;
		bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
		stConnectToKeepAliveServerAddr.sin_family = AF_INET;
		stConnectToKeepAliveServerAddr.sin_port = htons(pEventArgs->iKeepAliveServerPort);
		inet_pton(AF_INET, pEventArgs->cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
		printf("connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);

		bufferevent_setcb(pbevConnectToKeepAliveServer, recv_msg_from_keepalive_server_cb, NULL, connect_to_keepalive_server_cb, pEventArgs);
		if(bufferevent_socket_connect(pbevConnectToKeepAliveServer, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in)) < 0)
		{
			bufferevent_free(pbevConnectToKeepAliveServer);
			pbevConnectToKeepAliveServer = NULL;
			i--;
			free(pEventArgs);
			pEventArgs = NULL;
			continue;
		}
		if (bufferevent_enable(pbevConnectToKeepAliveServer, EV_READ | EV_WRITE) < 0)
		{
			bufferevent_free(pbevConnectToKeepAliveServer);
			pbevConnectToKeepAliveServer = NULL;
			i--;
			free(pEventArgs);
			pEventArgs = NULL;
			continue;
		}
		struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
		bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, &tvConnectTimeOut);

		//
		////		struct timeval tv = {0, 0};
		////		pEventArgs->pConnectToMasterServerEvent = event_new(pEventArgs->pEventBase, -1, 0, connect_to_master_server, pEventArgs);
		////		event_add(pEventArgs->pConnectToMasterServerEvent, &tv);

//		struct bufferevent *pbevConnectToMasterServer = bufferevent_socket_new(pEventArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
//		struct sockaddr_in stConnectToMasterServerAddr;
//		bzero(&stConnectToMasterServerAddr, sizeof(stConnectToMasterServerAddr));
//		stConnectToMasterServerAddr.sin_family = AF_INET;
//		stConnectToMasterServerAddr.sin_port = htons(MASTER_SERVER_PORT);
//		inet_pton(AF_INET, MASTER_SERVER_IP, (void *) &stConnectToMasterServerAddr.sin_addr);
//		printf("############ Connect to master ServerIp[%s], ServerPort[%d]\n", MASTER_SERVER_IP, MASTER_SERVER_PORT);
//
//		bufferevent_setcb(pbevConnectToMasterServer, recv_msg_from_master_server_cb, NULL, connect_to_master_server_cb, pEventArgs);
//		if (bufferevent_socket_connect(pbevConnectToMasterServer, (struct sockaddr*) &stConnectToMasterServerAddr, sizeof(struct sockaddr_in)) < 0)
//		{
//			bufferevent_free(pbevConnectToMasterServer);
//			pbevConnectToMasterServer = NULL;
//			i--;
//			continue;
//		}
//		if (bufferevent_enable(pbevConnectToMasterServer, EV_READ | EV_WRITE) < 0)
//		{
//			bufferevent_free(pbevConnectToMasterServer);
//			pbevConnectToMasterServer = NULL;
//			i--;
//			continue;
//		}
//		struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
//		bufferevent_set_timeouts(pbevConnectToMasterServer, NULL, &tvConnectTimeOut);
		usleep(100);
	}

	return NULL;
}


//获取start_num与end_num之间的随机数
HB_S32 random_number(HB_U32 threadSelfId, HB_S32 start_num, HB_S32 end_num)
{
	HB_S32 ret_num = 0;
	srand(threadSelfId);
	ret_num = rand() % (end_num - start_num) + start_num;
	return ret_num;
//	return 0;
}


static void timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event *timeout;
	timeout = (struct event *)arg;
	struct timeval tv;
	evutil_timerclear(&tv);

	HB_S32 total_events_num;
	struct event_base *base = event_get_base(timeout);

	total_events_num = event_base_get_num_events(base, EVENT_BASE_COUNT_ADDED);//获取base中活跃event的数量
//	total_events_num = event_base_get_num_events(base, EVENT_BASE_COUNT_ACTIVE);//获取base中活跃event的数量
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	event_add(timeout, &tv);
	printf("\n#######   Base:[%p] work thread base   total events = %d   \n", base, total_events_num);
}

HB_VOID *start_keep_alive_base(HB_HANDLE hArg)
{
	pthread_t threadSelfId = pthread_self();
	pthread_detach(threadSelfId);

	static HB_S32 iFlag = 1;

	int ret = random_number((HB_U32)threadSelfId, 1, 5);
	printf("sleep time : %d\n", ret);
//	sleep(ret);

	HB_S32 iClientNums = *(HB_S32 *)hArg;
	struct event_base *pEventBase = event_base_new();
	if (!pEventBase)
	{
		perror("cmd_base event_base_new()");
		return NULL;
	}
	//用于线程安全
	if(evthread_make_base_notifiable(pEventBase) != 0)
	{
		TRACE_ERR("###### evthread_make_base_notifiable() err!");
		return NULL;
	}

	THREAD_ARGS_OBJ stThreadArgs;
	stThreadArgs.pEventBase = pEventBase;
	stThreadArgs.threadSelfId = threadSelfId;
	stThreadArgs.iClientNums = iClientNums;
	if (iFlag)
	{
		stThreadArgs.iKeepAliveServerPort = 9003;
		iFlag--;
	}
	else
	{
		stThreadArgs.iKeepAliveServerPort = 9001;
	}

	pthread_t threadKeepAliveId = -1;
	pthread_create(&threadKeepAliveId, NULL, keepalive_start, &stThreadArgs);

	struct event timeout;//定时器事件
	struct timeval tv;//定时器值
	int flags = EV_TIMEOUT;
	event_assign(&timeout, pEventBase, -1, flags, timeout_cb, (void*)&timeout);
	evutil_timerclear(&tv);
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	event_add(&timeout, &tv);

	event_base_dispatch(pEventBase);
	event_base_free(pEventBase);

	return NULL;
}

#endif



