/*
 * slave.c
 *
 *  Created on: Aug 20, 2018
 *      Author: root
 */

#include "slave.h"
#include "heartbeat.h"

extern HB_CHAR cLocalIp[16];
HB_VOID func_connect_to_master_server_event(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg);

//用于从Slave接收消息
HB_VOID recv_msg_from_slave_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_HANDLE hArg)
{
	CMD_HEADER_OBJ stCmdHeader;
	HB_CHAR cRecvBuf[1024] = { 0 };
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;
	HB_S32 iErrCode = 0;

	bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, NULL);

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
//				TRACE_LOG("Registe to Slave succeed! id=[%s]\n", pEventArgs->cDevId);
	//			注册成功，创建定时器，定时发送心跳包
				struct timeval tv = {HEARTBATE_TIME_VAL, 0};
				event_assign(&pEventArgs->stHeartBeatTimerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT | EV_PERSIST, send_heartbeat, pEventArgs);
//				event_add(&pEventArgs->stHeartBeatTimerEvent, &tv);
				TRACE_LOG("Start send_heartbeat.. id=[%s] End event_add ret = %d\n", pEventArgs->cDevId, event_add(&pEventArgs->stHeartBeatTimerEvent, &tv));

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
//				bufferevent_set_timeouts(pbevConnectToKeepAliveServer, NULL, NULL);
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
			event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_slave_server_event, pEventArgs);
			event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
			TRACE_ERR("33333333333333333333 recv_msg_from_keepalive_server_cb recv err cmd[%d] reconnect to Slave... ret = %d\n", \
							iErrCode, event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv));
		}
	}
}


HB_VOID connect_to_slave_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

	if (what & BEV_EVENT_CONNECTED)
	{
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

		bufferevent_setcb(pEventArgs->pConnectToKeepAliveServerBev, recv_msg_from_slave_server_cb, NULL, connect_to_slave_server_cb, pEventArgs);
		//连接成功发送注册消息
		bufferevent_write(pbevConnectToKeepAliveServer, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
		if (bufferevent_enable(pbevConnectToKeepAliveServer, EV_READ) < 0)
		{
			if (pEventArgs->pConnectToKeepAliveServerBev != NULL)
			{
				bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
				pEventArgs->pConnectToKeepAliveServerBev = NULL;
			}
			struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_slave_server_event, pEventArgs);
			printf("XXXXXXXXXXXXX connect to Slave bufferevent_enable... ret = %d\n", event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv));
		}
		struct timeval tvReadTimeOut = {5, 0}; //5s读超时
		bufferevent_set_timeouts(pEventArgs->pConnectToKeepAliveServerBev, &tvReadTimeOut, NULL);
	}
	else
	{
		printf("connect to slave event:0x%x \n", what);

		if (pEventArgs->pConnectToKeepAliveServerBev != NULL)
		{
			bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
			pEventArgs->pConnectToKeepAliveServerBev = NULL;
		}

		pEventArgs->iConnectToKeepAliveServerErrTimes += 1;
		if(pEventArgs->iConnectToKeepAliveServerErrTimes >= 3)
		{
			pEventArgs->iConnectToKeepAliveServerErrTimes = 0;
			TRACE_ERR("Can't connect to keep alive server, get new keep alive server...\n");
			struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_master_server_event, pEventArgs);
			printf("33333333 连接Master失败! 重连 ret=%d End!\n", event_add(&pEventArgs->stConnectToMasterServerEvent, &tv));
//			event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
//			printf("Can't connect to keep alive server, get new keep alive server... End!\n");
		}
		else
		{
			printf("connect to keep alive server failed! reconnect !\n");
			struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_slave_server_event, pEventArgs);
//			event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv);
			printf("222222222222222 reconnect to Slave... ret = %d\n", event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv));
//			printf("connect to keep alive server failed! reconnect End!\n");
		}
	}
}


HB_VOID func_connect_to_slave_server_event(evutil_socket_t fd, short events, void *hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

	HB_S32 iConnectToKeepAliveServerFd = socket(AF_INET,SOCK_STREAM,0);//返回-1表示失败;
	struct sockaddr_in stConnectToKeepAliveServerAddr1;
	HB_S32 iSize = sizeof(struct sockaddr_in);

	bzero(&stConnectToKeepAliveServerAddr1,sizeof(stConnectToKeepAliveServerAddr1));
	stConnectToKeepAliveServerAddr1.sin_family = AF_INET;
	stConnectToKeepAliveServerAddr1.sin_port = htons(0); //服务器绑定的端口
	stConnectToKeepAliveServerAddr1.sin_addr.s_addr = inet_addr(cLocalIp);//服务器的IP地址
	bind(iConnectToKeepAliveServerFd,(struct sockaddr*)&stConnectToKeepAliveServerAddr1,iSize);//返回-1表示失败

	if (NULL == pEventArgs->pConnectToKeepAliveServerBev)
	{
		pEventArgs->pConnectToKeepAliveServerBev = bufferevent_socket_new(pEventArgs->pEventBase, iConnectToKeepAliveServerFd, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
	}
	struct sockaddr_in stConnectToKeepAliveServerAddr;
	bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
	stConnectToKeepAliveServerAddr.sin_family = AF_INET;
	stConnectToKeepAliveServerAddr.sin_port = htons(pEventArgs->iKeepAliveServerPort);
	inet_pton(AF_INET, pEventArgs->cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
	printf("connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);

	bufferevent_setcb(pEventArgs->pConnectToKeepAliveServerBev, recv_msg_from_slave_server_cb, NULL, connect_to_slave_server_cb, pEventArgs);
	printf("000000connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);
	if (bufferevent_socket_connect(pEventArgs->pConnectToKeepAliveServerBev, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in)) < 0)
	{
		if (NULL != pEventArgs->pConnectToKeepAliveServerBev)
		{
			bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
			pEventArgs->pConnectToKeepAliveServerBev = NULL;
		}
		struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_slave_server_event, pEventArgs);
		printf("000000000 reconnect to Slave... ret = %d\n", event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv));
		return ;
	}
	if (bufferevent_enable(pEventArgs->pConnectToKeepAliveServerBev, EV_READ | EV_WRITE) < 0)
	{
		if (NULL != pEventArgs->pConnectToKeepAliveServerBev)
		{
			bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
			pEventArgs->pConnectToKeepAliveServerBev = NULL;
		}
		struct timeval tv = {RECONNET_TO_SERVER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToKeepAliveServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_slave_server_event, pEventArgs);
		printf("111111 reconnect to Slave... ret = %d\n", event_add(&pEventArgs->stConnectToKeepAliveServerEvent, &tv));
		return ;
	}
	printf("111111111connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);
	struct timeval tvConnectTimeOut = {5, 0}; //5s连接超时
	bufferevent_set_timeouts(pEventArgs->pConnectToKeepAliveServerBev, NULL, &tvConnectTimeOut);
	printf("22222222222connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);
}

