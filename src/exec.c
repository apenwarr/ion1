/*
 * ion/exec.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "exec.h"
#include "frame.h"
#include "property.h"
#include "signal.h"
#include "global.h"


#define SHELL_PATH "/bin/sh"
#define SHELL_NAME "sh"
#define SHELL_ARG "-c"


void wm_do_exec(const char *cmd)
{
	char *argv[4];
	
	wglobal.dpy=NULL;
	
	if(cmd==NULL)
		return;
	
#ifndef CF_NO_SETPGID
	setpgid(0, 0);
#endif
	
	argv[0]=SHELL_NAME;
	argv[1]=SHELL_ARG;
	argv[2]=(char*)cmd; /* stupid execve... */
	argv[3]=NULL;
	execvp(SHELL_PATH, argv);

	die_err_obj(cmd);
}


void wm_exec(WScreen *scr, const char *cmd)
{
	int pid;
	char *tmp;

	pid=fork();
	
	if(pid<0)
		warn_err();
	
	if(pid!=0)
		return;
	
	setup_environ(scr->xscr);
	
	close(wglobal.conn);
	
	tmp=ALLOC_N(char, strlen(cmd)+8);
	
	if(tmp==NULL)
		die_err();
	
	sprintf(tmp, "exec %s", cmd);
	
	wm_do_exec(tmp);
}


void do_open_with(WScreen *scr, const char *cmd, const char *file)
{
	int pid;
	char *tmp;

	pid=fork();
	
	if(pid<0)
		warn_err();
	
	if(pid!=0)
		return;

	setup_environ(scr->xscr);

	close(wglobal.conn);
	
	tmp=ALLOC_N(char, strlen(cmd)+strlen(file)+10);

	if(tmp==NULL)
		die_err();

	sprintf(tmp, "exec %s '%s'", cmd, file);

	wm_do_exec(tmp);
}


void setup_environ(int scr)
{
	char *tmp, *ptr;
	char *display;
	
	display=XDisplayName(wglobal.display);
	
	tmp=ALLOC_N(char, strlen(display)+16);
	
	if(tmp==NULL){
		warn_err();
		return;
	}
	
	sprintf(tmp, "DISPLAY=%s", display); 

	ptr=strchr(tmp, ':');
	if(ptr!=NULL){
		ptr=strchr(ptr, '.');
		if(ptr!=NULL)
			*ptr='\0';
	}

	if(scr>=0)
		sprintf(tmp+strlen(tmp), ".%i", scr);

	putenv(tmp);
	
	/*XFree(display);*/
}


/*
 * Restart and exit
 */

/*#define CKILL_TIMEOUT 60

static bool wait_exit()
{
	time_t timeout;
	
	if(wglobal.n_alive==0)
		return TRUE;
	
	timeout=CKILL_TIMEOUT+time(NULL);
	
	while(1){
		sleep(1);
		if(wglobal.n_alive==0)
			return TRUE;
		if(timeout>time(NULL))
			return FALSE;
	}
}


static void terminate_chld()
{
	int i;
	
	for(i=0; i<wglobal.n_children; i++){
		if(wglobal.children[i]!=0)
			kill(wglobal.children[i], SIGTERM);
	}
			
	if(wait_exit())
		return;

	if(wglobal.children[i]!=0)
		kill(wglobal.children[i], SIGKILL);
			
	wait_exit();
}*/


void wm_exitret(int retval)
{	
	/*if(wglobal.parent!=0)
		kill(wglobal.parent, SIGTERM);
	else
		terminate_chld();*/
	deinit();
	exit(retval);
}


void wm_exit()
{
	wm_exitret(EXIT_SUCCESS);
}


void wm_restart_other(const char *cmd)
{
	/*if(wglobal.parent!=0){
		set_string_property(COMM_WIN, wglobal.atom_private_ipc, cmd);
		kill(wglobal.parent, SIGUSR1);
		deinit();
		exit(0);
	}else*/{
		/*terminate_chld();
		set_string_property(COMM_WIN, wglobal.atom_private_ipc, NULL);
		setup_environ(-1);*/
		deinit();
		if(cmd!=NULL){
			wm_do_exec(cmd);
			/* failure - try to restart ourselves */
		}
		execvp(wglobal.argv[0], wglobal.argv);	
		die_err_obj(wglobal.argv[0]);
	}
}


void wm_restart()
{
	wm_restart_other(NULL);
}

