/*
 * master.c
 *
 *  Created on: Aug 20, 2018
 *      Author: root
 */

#include "master.h"

HB_VOID connect_to_slave_server_cb(struct bufferevent *pbevConnectToKeepAliveServer, HB_S16 what, HB_HANDLE hArg);


//从master接收消息的回调函数
HB_VOID recv_msg_from_master_server_cb(struct bufferevent *pbevConnectToMasterServer, HB_HANDLE hArg)
{
	CMD_HEADER_OBJ stCmdHeader;
	HB_CHAR cRecvBuf[1024] = { 0 };
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;
	HB_S32 iErrCode = 0;

	bufferevent_disable(pbevConnectToMasterServer, EV_READ | EV_WRITE);
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
				TRACE_ERR("从Master获取Slave IP失败!\n");
				iErrCode = -1;
				break;
			}

			cJSON *pSlavePort = cJSON_GetObjectItem(pRoot, "SlaveKeepAlivePort");
			if (NULL != pSlaveIp)
			{
				pEventArgs->iKeepAliveServerPort = pSlavePort->valueint;
			}
			else
			{
				cJSON_Delete(pRoot);
				TRACE_ERR("从Master获取Slave 端口失败!\n");
				iErrCode = -2;
				break;
			}

			cJSON_Delete(pRoot);
			TRACE_LOG("从Master获取Slave地址成功 recv reponse :[%s]\n", cRecvBuf+sizeof(CMD_HEADER_OBJ));
#if 1
			pEventArgs->pConnectToKeepAliveServerBev = bufferevent_socket_new(pEventArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
			struct sockaddr_in stConnectToKeepAliveServerAddr;
			bzero(&stConnectToKeepAliveServerAddr, sizeof(stConnectToKeepAliveServerAddr));
			stConnectToKeepAliveServerAddr.sin_family = AF_INET;
			stConnectToKeepAliveServerAddr.sin_port = htons(pEventArgs->iKeepAliveServerPort);
			inet_pton(AF_INET, pEventArgs->cKeepAliveServerIp, (void *) &stConnectToKeepAliveServerAddr.sin_addr);
			printf("connect to slave ServerIp[%s], ServerPort[%d]\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);

			bufferevent_setcb(pEventArgs->pConnectToKeepAliveServerBev, NULL, NULL, connect_to_slave_server_cb, pEventArgs);
			if (bufferevent_enable(pEventArgs->pConnectToKeepAliveServerBev, 0) < 0)
			{
				bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
				pEventArgs->pConnectToKeepAliveServerBev = NULL;
				iErrCode = -4;
				break;
			}
			if (bufferevent_socket_connect(pEventArgs->pConnectToKeepAliveServerBev, (struct sockaddr*) &stConnectToKeepAliveServerAddr, sizeof(struct sockaddr_in)) < 0)
			{
				bufferevent_free(pEventArgs->pConnectToKeepAliveServerBev);
				pEventArgs->pConnectToKeepAliveServerBev = NULL;
				iErrCode = -3;
				break;
			}
#endif
			printf("------------>connect to slave ServerIp[%s], ServerPort[%d] End!\n", pEventArgs->cKeepAliveServerIp, pEventArgs->iKeepAliveServerPort);
		}
		break;
		default:
			TRACE_ERR("default recv reponse[%d] :[%s]\n", stCmdHeader.cmd_func, cRecvBuf+sizeof(CMD_HEADER_OBJ));
			iErrCode = -5;
			break;
	}

	if (iErrCode < 0)
	{
		TRACE_ERR("iErrCode iErrCode iErrCode iErrCode iErrCode ======= %d\n", iErrCode);
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_master_server_event, pEventArgs);
		printf("2222222222 连接Master失败! 重连 ret=%d End!\n", event_add(&pEventArgs->stConnectToMasterServerEvent, &tv));
//		event_add(&pEventArgs->stConnectToMasterServerEvent, &tv);
		TRACE_ERR("iErrCode iErrCode iErrCode iErrCode iErrCode ======= %d  End\n", iErrCode);
		return ;
	}
#endif
}


HB_VOID connect_to_master_server_cb(struct bufferevent *pConnectToMasterServerBev, HB_S16 event, HB_HANDLE hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

	if (event & BEV_EVENT_CONNECTED)
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

		printf("++++++++++++++++++++++++++++++++++++Send to Master : [%s]\n", cSendBuf+sizeof(CMD_HEADER_OBJ));

		bufferevent_setcb(pConnectToMasterServerBev, recv_msg_from_master_server_cb, NULL, connect_to_master_server_cb, pEventArgs);
		if (bufferevent_enable(pConnectToMasterServerBev, EV_READ) < 0)
		{
			printf("000连接Master bufferevent_enable 失败! 重连 DevId=[%s]!\n", pEventArgs->cDevId);
			bufferevent_free(pConnectToMasterServerBev);
			pConnectToMasterServerBev = NULL;
			struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
			event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_master_server_event, pEventArgs);
			printf("000连接Master bufferevent_enable失败! 重连 ret=%d End DevId=[%s]!\n", event_add(&pEventArgs->stConnectToMasterServerEvent, &tv), pEventArgs->cDevId);
			return;
		}
		struct timeval tvReadTimeOut = {15, 0}; //5s读超时
		bufferevent_set_timeouts(pConnectToMasterServerBev, &tvReadTimeOut, NULL);
		//连接成功发送注册消息
		bufferevent_write(pConnectToMasterServerBev, cSendBuf, sizeof(stCmdHeader)+stCmdHeader.cmd_length);
	}
	else
	{
		TRACE_ERR("连接Master失败! 重连 DevId=[%s] err_code=(0X%x)!\n", pEventArgs->cDevId, event);
		bufferevent_free(pConnectToMasterServerBev);
		pConnectToMasterServerBev = NULL;
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_master_server_event, pEventArgs);
		TRACE_ERR("连接Master失败! 重连 ret=%d End DevId=[%s]!\n", event_add(&pEventArgs->stConnectToMasterServerEvent, &tv), pEventArgs->cDevId);
	}
}


//重连master事件
HB_VOID func_connect_to_master_server_event(evutil_socket_t fd, HB_S16 events, HB_HANDLE hArg)
{
	EVENT_ARGS_HANDLE pEventArgs = (EVENT_ARGS_HANDLE)hArg;

	struct bufferevent *pConnectToMasterServerBev = bufferevent_socket_new(pEventArgs->pEventBase, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE);
	struct sockaddr_in stConnectToMasterServerAddr;
	bzero(&stConnectToMasterServerAddr, sizeof(stConnectToMasterServerAddr));
	stConnectToMasterServerAddr.sin_family = AF_INET;
	stConnectToMasterServerAddr.sin_port = htons(MASTER_SERVER_PORT);
	inet_pton(AF_INET, MASTER_SERVER_IP, (void *) &stConnectToMasterServerAddr.sin_addr);
	printf("############ func_connect_to_master_server_event() Connect to master ServerIp[%s], ServerPort[%d]\n", MASTER_SERVER_IP, MASTER_SERVER_PORT);

	bufferevent_setcb(pConnectToMasterServerBev, NULL, NULL, connect_to_master_server_cb, pEventArgs);
	if (bufferevent_enable(pConnectToMasterServerBev, 0) < 0)
	{
		bufferevent_free(pConnectToMasterServerBev);
		pConnectToMasterServerBev = NULL;
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_master_server_event, pEventArgs);
		printf("11111111111111 连接Master失败! 重连 ret=%d End!\n", event_add(&pEventArgs->stConnectToMasterServerEvent, &tv));
		return ;
	}

	if (bufferevent_socket_connect(pConnectToMasterServerBev, (struct sockaddr*) &stConnectToMasterServerAddr, sizeof(struct sockaddr_in)) < 0)
	{
		bufferevent_free(pConnectToMasterServerBev);
		pConnectToMasterServerBev = NULL;
		struct timeval tv = {RECONNET_TO_MASTER_INTERVAL, 0};
		event_assign(&pEventArgs->stConnectToMasterServerEvent, pEventArgs->pEventBase, -1, EV_TIMEOUT, func_connect_to_master_server_event, pEventArgs);
		printf("000000000  连接Master失败! 重连 ret=%d End!\n", event_add(&pEventArgs->stConnectToMasterServerEvent, &tv));
		return ;
	}
}


