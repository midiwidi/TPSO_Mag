#include <signal.h>
#include <string.h>
#include "signal.h"
#include "helpers.h"
#include "log.h"

void signal_handler(int signum) {
	switch(signum)
	{
		case SIGUSR1: //Cameralink Interrupt occurred (not enabled in the driver)
			break;
		case SIGUSR2: //Event occurred, can be read through uevent in sysfs
			break;
		case SIGALRM: //Timer expired, stop capturing
			break;
		case SIGINT:
			log_write(LOG_NOTICE, "interrupted by user");
			clean_exit(NO_ERROR);
			break;
	}
	return;
}

void signal_init(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action)); /* clean variable */
	action.sa_handler = signal_handler; /* specify signal handler */
	action.sa_flags = 0;		    	/* operation flags setting */
	sigfillset(&action.sa_mask);		/* Block every signal during the handler */

	sigaction(SIGUSR1, &action, NULL); /* attach action with SIGIO */
	sigaction(SIGUSR2, &action, NULL); /* attach action with SIGIO */
	sigaction(SIGALRM, &action, NULL); /* attach action with SIGIO */
	sigaction(SIGINT, &action, NULL); /* attach action with SIGIO */
}
