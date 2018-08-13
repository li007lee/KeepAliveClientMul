/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "common_args.h"
#include "keepalive.h"
#include "dev_opt.h"

//GLOBLE_PARAM_OBJ glParam;

HB_S32 main(HB_S32 argc, HB_CHAR **argv)
{

	HB_S32 iBaseNums = 0;
	HB_S32 iClientNums = 0;
	if (argc < 3)
	{
		printf("Usage: ./heartbeat_client_X64 [Base个数] [每个Base中的客户端个数]\n\n");
		return -1;
	}
	else
	{
		iBaseNums = atoi(argv[1]);
		iClientNums = atoi(argv[2]);
	}

	printf("----------------------------------------------------------------\n");
	printf("|                       KeepAlive Started                      |\n");
	printf("|              CompileTime : %s %s              |\n", __DATE__, __TIME__);
	printf("----------------------------------------------------------------\n");

	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);
	if (pthread_sigmask(SIG_BLOCK, &signal_mask, NULL) != 0)
	{
		TRACE_ERR("###### block sigpipe error!\n");
		return HB_FAILURE;
	}

	if(evthread_use_pthreads() != 0)
	{
		TRACE_ERR("########## evthread_use_pthreads() err !");
		return HB_FAILURE;
	}

//	memset(&glParam, 0, sizeof(glParam));
//	glParam.pEventBase = event_base_new();
//	if (!glParam.pEventBase)
//	{
//		perror("cmd_base event_base_new()");
//		return HB_FAILURE;
//	}
//	//用于线程安全
//	if(evthread_make_base_notifiable(glParam.pEventBase) != 0)
//	{
//		TRACE_ERR("###### evthread_make_base_notifiable() err!");
//		return HB_FAILURE;
//	}

//	get_dev_info();
	HB_S32 i = 0;
//	pthread_t threadBaseId[16] = {-1};
	for(i=0;i<iBaseNums;i++)
	{
		pthread_t threadBaseId = -1;
		pthread_create(&threadBaseId, NULL, start_keep_alive_base, &iClientNums);
	}


	for(;;)
	{
		sleep(60);
	}

//	pause();

//
//	keepalive_start(iClientNums);
//
//	event_base_dispatch(glParam.pEventBase);
//	event_base_free(glParam.pEventBase);

	return 0;
}
