/*
 * ion/main.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <libtu/util.h>
#include <libtu/optparser.h>

#include <X11/Xlib.h>

#include "common.h"
#include "screen.h"
#include "config.h"
#include "event.h"
#include "cursor.h"
#include "signal.h"
#include "binding.h"
#include "readconfig.h"
#include "global.h"
#include "confws.h"
#include "draw.h"
#include "modules.h"
#include "exec.h"


/*{{{ Global variables and optparser data */


WGlobal wglobal;


/* Options. Getopt is not used because getopt_long is quite gnu-specific
 * and they don't know of '-display foo' -style args anyway.
 * Instead, I've reinvented the wheel in libtu :(.
 */
static OptParserOpt opts[]={
	{OPT_ID('d'), 	"display", 	OPT_ARG, "host:dpy.scr", "X display to use"},
	{'c', 			"cfgfile", 	OPT_ARG, "config_file", "Configuration file"},
	{OPT_ID('o'), 	"onescreen", 0, NULL, "Manage default screen only"},
	{'i',			"clientId", 		OPT_ARG, "client_id", "Session manager client id"},
	{'r',			"restore",	OPT_ARG, "state_file", "Ion state file"},
	{0, NULL, 0, NULL, NULL}
};


static const char ion_usage_tmpl[]=
	"Usage: $p [options]\n\n$o\n";


static const char ion_about[]=
	"Ion " ION_VERSION ", copyright (c) Tuomo Valkonen 1999-2001.\n"
	"This program may be copied and modified under the terms of the "
	"Artistic License.\n";


static OptParserCommonInfo ion_cinfo={
	ION_VERSION,
	ion_usage_tmpl,
	ion_about
};


/*}}}*/

	
/*{{{ Main & init */


static bool initialize(const char *display, const char *cfgfile,
					   bool onescreen);

extern InputHandler default_input_handler;

int main(int argc, char*argv[])
{
	int opt;
	const char *cfgfile=NULL;
	bool onescreen=FALSE;
	
	libtu_init(argv[0]);
	
	wglobal.argc=argc;
	wglobal.argv=argv;
	wglobal.dpy=NULL;
	wglobal.display=NULL;
	wglobal.current_wswindow=NULL;
	wglobal.previous=NULL;
	wglobal.grab_holder=NULL;
	wglobal.input_mode=INPUT_NORMAL;
	wglobal.dblclick_delay=CF_DBLCLICK_DELAY;
	wglobal.resize_delay=CF_RESIZE_DELAY;
	wglobal.opaque_resize=FALSE;
	wglobal.opmode=OPMODE_INIT;
	wglobal.screens=NULL;
	wglobal.current_screen=NULL;
	wglobal.previous_protect=0;
	wglobal.client_id=NULL;
	wglobal.state_file=NULL;
	wglobal.input_handler=&default_input_handler;
 	memset(wglobal.shortcuts, 0, sizeof(wglobal.shortcuts));
	
	/* The rest don't need to be initialized here */

	optparser_init(argc, argv, OPTP_MIDLONG, opts, &ion_cinfo);
	
	while((opt=optparser_get_opt())){
		switch(opt){
		case OPT_ID('d'):
			wglobal.display=scopy(optparser_get_arg());
			if(wglobal.display==NULL){
				warn_err();
				return EXIT_FAILURE;
			}
			break;
		case 'c':
			cfgfile=optparser_get_arg();
			break;
		case OPT_ID('o'):
			onescreen=TRUE;
			break;
		case 'i':
			wglobal.client_id=scopy(optparser_get_arg());
			break;
		case 'r':
			wglobal.state_file=scopy(optparser_get_arg());
			break;
		default:
			optparser_print_error();
			return EXIT_FAILURE;
		}
	}

	if(!initialize(wglobal.display, cfgfile, onescreen))
		return EXIT_FAILURE;
	
	wglobal.opmode=OPMODE_NORMAL;

	/*close(0);
	open("/dev/null", O_RDONLY);*/
	
	mainloop();
	
	/* The code should never return here */
	return EXIT_SUCCESS;
}

extern void debug_dump_thing(WThing *thing);
static bool load_initial_workspaces()
{
	WScreen *scr;
	
	FOR_ALL_SCREENS(scr){
		postinit_graphics(scr);
		read_workspaces(scr);
		postinit_screen(scr);
	}
	return TRUE;
}

static bool initialize(const char*display, const char *cfgfile,
					   bool onescreen)
{
	Display *dpy;
	WScreen *scr, *prev=NULL;
	int i, dscr, nscr;
	bool res;
	
	/* Open the display. */
	dpy=XOpenDisplay(display);
	
	if(dpy==NULL){
		warn("Could not connect to X display '%s'", XDisplayName(display));
		return FALSE;
	}

	if(onescreen){
		dscr=DefaultScreen(dpy);
		nscr=dscr+1;
	}else{
		dscr=0;
		nscr=ScreenCount(dpy);
	}

	/* Initialize */
	wglobal.dpy=dpy;

	wglobal.conn=ConnectionNumber(dpy);
	wglobal.win_context=XUniqueContext();
	
	wglobal.atom_wm_state=XInternAtom(dpy, "WM_STATE", False);
	wglobal.atom_wm_change_state=XInternAtom(dpy, "WM_CHANGE_STATE", False);
	wglobal.atom_wm_protocols=XInternAtom(dpy, "WM_PROTOCOLS", False);
	wglobal.atom_wm_delete=XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wglobal.atom_wm_take_focus=XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wglobal.atom_wm_colormaps=XInternAtom(dpy, "WM_COLORMAP_WINDOWS", False);
	wglobal.atom_frame_id=XInternAtom(dpy, "_PWM_FRAME_ID", False);
	wglobal.atom_workspace=XInternAtom(dpy, "_ION_WORKSPACE", False);
	wglobal.atom_selection=XInternAtom(dpy, "_ION_SELECTION_STRING", False);
	
	trap_signals();
	
	init_bindings();
	load_cursors();	
	
	for(i=dscr; i<nscr; i++){
		scr=preinit_screen(i);
		
		if(scr==NULL)
			continue;
		
		preinit_graphics(scr);
		
		if(prev==NULL){
			wglobal.screens=scr;
		}else{
			LINK_ITEM_AFTER((WThing*)wglobal.screens, (WThing*)prev,
							(WThing*)scr, t_next, t_prev);
		}
		
		prev=scr;
	}
	
	if(prev==NULL){
		if(nscr-dscr>1)
			warn("Could not find a screen to manage.");
		return FALSE;
	}
	
	if(!read_config(cfgfile))
		goto configfail;
	
	if(wglobal.main_bindmap.nbindings==0 ||
	   wglobal.moveres_bindmap.nbindings==0)
		goto configfail;
	
	CALL_ALT_B_ARG(res, load_initial_workspaces, ());

	CALL_HOOKS(init_done_hook);

	/*atexit(deinit);*/
	
	return TRUE;

configfail:
#define MSG \
	"Unable to load configuration or inadequate binding configurations. " \
    "Refusing to start.\nYou *must* install proper configuration files " \
	"either in ~/.ion or "ETCDIR"/ion to use Ion."

	warn(MSG);
	setup_environ(DefaultScreen(dpy));
	XCloseDisplay(dpy);
	wm_do_exec("xmessage 'ion: " MSG "'");
	return FALSE;
}


/*}}}*/


/*{{{ Deinit */

void deinit()
{
	Display *dpy;
	WScreen *scr;
	
	wglobal.opmode=OPMODE_DEINIT;
	
	if(wglobal.dpy==NULL)
		return;

	unload_modules();
	
	FOR_ALL_SCREENS(scr){
		write_workspaces(scr);
		/*CALL_ALT_B_ARG(res, write_workspaces, (scr));*/
		deinit_screen(scr);
	}
	
	dpy=wglobal.dpy;
	wglobal.dpy=NULL;
	
	XCloseDisplay(dpy);
}

/*}}}*/

