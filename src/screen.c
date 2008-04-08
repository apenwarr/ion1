/*
 * ion/screen.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <X11/Xlib.h>
/*#include <X11/Xmu/Error.h>*/
#include <X11/Xproto.h>

#include "common.h"
#include "screen.h"
#include "cursor.h"
#include "frame.h"
#include "global.h"
#include "clientwin.h"
#include "event.h"
#include "workspace.h"
#include "draw.h"
#include "thingp.h"


static void screen_remove_child(WScreen *scr, WThing *thing);

static WThingFuntab screen_funtab={
	deinit_screen,
	screen_remove_child,
};

IMPLOBJ(WScreen, WWindow, &screen_funtab)


/*{{{ Error handling */


static bool redirect_error=FALSE;
static bool ignore_badwindow=TRUE;


static int my_redirect_error_handler(Display *dpy, XErrorEvent *ev)
{
	redirect_error=TRUE;
	return 0;
}


static int my_error_handler(Display *dpy, XErrorEvent *ev)
{
	static char msg[128], request[64], num[32];
	
	/* Just ignore bad window and similar errors; makes the rest of
	 * the code simpler.
	 */
	if((ev->error_code==BadWindow ||
		(ev->error_code==BadMatch && ev->request_code==X_SetInputFocus) ||
		(ev->error_code==BadDrawable && ev->request_code==X_GetGeometry)) &&
	   ignore_badwindow)
		return 0;

#if 0
	XmuPrintDefaultErrorMessage(dpy, ev, stderr);
#else
	XGetErrorText(dpy, ev->error_code, msg, 128);
	sprintf(num, "%d", ev->request_code);
	XGetErrorDatabaseText(dpy, "XRequest", num, "", request, 64);

	if(request[0]=='\0')
		sprintf(request, "<unknown request>");

	if(ev->minor_code!=0){
		warn("[%d] %s (%d.%d) %#lx: %s", ev->serial, request,
			 ev->request_code, ev->minor_code, ev->resourceid,msg);
	}else{
		warn("[%d] %s (%d) %#lx: %s", ev->serial, request,
			 ev->request_code, ev->resourceid,msg);
	}
#endif

	kill(getpid(), SIGTRAP);
	
	return 0;
}


/*}}}*/


/*{{{ Utility functions */


Window create_simple_window_bg(const WWindow *p, int x, int y, int w, int h,
							   ulong background)
{
	return XCreateSimpleWindow(wglobal.dpy, p->win, x, y, w, h,
							   0, BlackPixel(wglobal.dpy, SCREEN_OF(p)->xscr),
							   background);
}


Window create_simple_window(const WWindow *p, int x, int y, int w, int h)
{
	return create_simple_window_bg(p, x, y, w, h,
								   GRDATA_OF(p)->frame_colors.bg);
}


/*}}}*/


/*{{{ Init */


static void scan_initial_windows(WScreen *scr)
{
	Window dummy_root, dummy_parent, *wins;
	uint nwins, i, j;
	XWMHints *hints;
	
	XQueryTree(wglobal.dpy, scr->root.win,
			   &dummy_root, &dummy_parent, &wins, &nwins);
	
	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		if(FIND_WINDOW(wins[i])!=NULL){
			wins[i]=None;
			continue;
		}
		hints=XGetWMHints(wglobal.dpy, wins[i]);
		if(hints!=NULL && hints->flags&IconWindowHint){
			for(j=0; j<nwins; j++){
				if(wins[j]==hints->icon_window){
					wins[j]=None;
					break;
				}
			}
		}
		if(hints!=NULL)
			XFree((void*)hints);

	}

	for(i=0; i<nwins; i++){
		if(wins[i]==None)
			continue;
		manage_clientwin(wins[i], MANAGE_INITIAL);
	}
	
	XFree((void*)wins);
}


WScreen *preinit_screen(int xscr)
{
	Display *dpy=wglobal.dpy;
	WScreen *scr;
	WRectangle geom;
	Window rootwin;
	
	/* Try to select input on the root window */
	rootwin=RootWindow(dpy, xscr);
	
	redirect_error=FALSE;

	XSetErrorHandler(my_redirect_error_handler);
	XSelectInput(dpy, rootwin, ROOT_MASK);
	XSync(dpy, 0);
	XSetErrorHandler(my_error_handler);

	if(redirect_error){
		warn("Unable to redirect root window events for screen %d.", xscr);
		return NULL;
	}
	
	scr=ALLOC(WScreen);
	
	if(scr==NULL){
		warn_err();
		return NULL;
	}
	
	/* Init the struct */
	WTHING_INIT(scr, scr, WScreen);
	
	geom.x=0; geom.y=0;
	geom.w=DisplayWidth(dpy, xscr);
	geom.h=DisplayHeight(dpy, xscr);
	
	init_window((WWindow*)scr, rootwin, geom);
	
	scr->root.bindmap=&(wglobal.main_bindmap);
	
	scr->xscr=xscr;
	scr->default_cmap=DefaultColormap(dpy, xscr);

	scr->workspace_count=0;
	scr->current_workspace=0;
	scr->w_unit=7;
	scr->h_unit=13;
	scr->confsel=FALSE;
	
	return scr;
}


void postinit_screen(WScreen *scr)
{
	init_workspaces(scr);
	scan_initial_windows(scr);
	set_cursor(scr->root.win, CURSOR_DEFAULT);
}


/*}}}*/


/*{{{ Deinit */


void deinit_screen(WScreen *scr)
{
	destroy_subthings((WThing*)scr);
}


static void screen_remove_child(WScreen *scr, WThing *thing)
{
	WWorkspace *ws;
	
	if(!WTHING_IS(thing, WWorkspace))
		return;
	
	ws=(WWorkspace*)thing;
	
	scr->workspace_count--;
	
	if(ws==scr->current_workspace && wglobal.opmode!=OPMODE_DEINIT){
		scr->current_workspace=NULL;
		unlink_thing(thing);
		ws=FIRST_THING(scr, WWorkspace);
		if(ws!=NULL)
			switch_workspace(ws);
	}
}


/*}}}*/


/*{{{ Misc */


WScreen *screen_of(const WThing *t)
{
	WScreen *scr=(WScreen*)(t->screen);
	assert(scr!=NULL);
	return scr;
}


Window root_of(const WThing *t)
{
	WScreen *scr=screen_of(t);
	return scr->root.win;
}


WGRData *grdata_of(const WThing *t)
{
	WScreen *scr=screen_of(t);
	return &(scr->grdata);
}


bool same_screen(const WThing *t1, const WThing *t2)
{
	WScreen *scr1=SCREEN_OF(t1);
	WScreen *scr2=SCREEN_OF(t2);
	return scr1==scr2;
}


/*}}}*/
