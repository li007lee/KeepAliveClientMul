/*
 * main.c
 *
 *  Created on: 2017年6月13日
 *      Author: root
 */

#include "common_args.h"
#include "keepalive.h"
#include "dev_opt.h"

HB_CHAR cLocalIp[16] = {0};

HB_S32 main(HB_S32 argc, HB_CHAR **argv)
{

	HB_S32 iBaseNums = 0;
	HB_S32 iClientNums = 0;
	if (argc < 3)
	{
		printf("Usage: ./heartbeat_client_X64 [LocalIp] [Base个数] [每个Base中的客户端个数]\n\n");
		return -1;
	}
	else
	{
		strncpy(cLocalIp, argv[1], sizeof(cLocalIp));
		iBaseNums = atoi(argv[2]);
		iClientNums = atoi(argv[3]);
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

	HB_S32 i = 0;
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

	return 0;
}
