/*
 * ion/focus.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "focus.h"
#include "thingp.h"
#include "global.h"
#include "wedln.h"
#include "query.h"


/*{{{ Previous active thing */


static WThing *current_client_or_frame()
{
	WWindow *cw=wglobal.current_wswindow;
	
	if(cw!=NULL && WTHING_IS(cw, WFrame) &&
	   ((WFrame*)cw)->current_client!=NULL){
		return (WThing*)((WFrame*)cw)->current_client;
	}
	return (WThing*)cw;
}


static WThing *get_next_previous(WThing *next)
{
	WThing *current=current_client_or_frame();
	
	if(current==next || current==NULL)
		return NULL;
	
	if((WThing*)FIND_PARENT(current, WWindow)==next)
		return NULL;
	
	return current;
}


void set_previous(WThing *thing)
{
	WThing *previous;
	
	if(wglobal.previous_protect!=0)
		return;
	
	previous=get_next_previous(thing);
	
	if(previous!=NULL)
		wglobal.previous=previous;
}


void protect_previous()
{
	wglobal.previous_protect++;
}


void unprotect_previous()
{
	assert(wglobal.previous_protect>0);
	wglobal.previous_protect--;
}


/*}}}*/


/*{{{ set_focus, warp */

void set_input_focus(Window win)
{
	XSetInputFocus(wglobal.dpy, win, RevertToParent, CurrentTime);
}

void do_set_focus(WThing *thing)
{
	if(thing==NULL){
		thing=(WThing*)wglobal.current_screen;
		if(thing==NULL)
			return;
	}else if(!on_visible_workspace(thing)){
		return;
	}
	
	query_set_function_thing(thing);

	if(WTHING_IS(thing, WFrame))
		focus_frame((WFrame*)thing);
	else if(WTHING_IS(thing, WWindow))
		focus_window((WWindow*)thing);
	else if(WTHING_IS(thing, WClient))
		focus_client((WClient*)thing);
	else if(WTHING_IS(thing, WClientWin))
		focus_clientwin((WClientWin*)thing);
	else{
		if(wglobal.current_screen!=NULL){
			XSetInputFocus(wglobal.dpy, wglobal.current_screen->root.win,
						   RevertToNone, CurrentTime);
		}
	}
}


void set_focus(WThing *thing)
{
	wglobal.focus_next=thing;
	query_set_function_thing(thing);
}


void warp(WWindow *wwin)
{
#ifdef CF_WARP
	XWarpPointer(wglobal.dpy, None, wwin->win, 0, 0, 0, 0,
				 5, 5);
#endif
	set_focus((WThing*)wwin);
	query_set_function_thing((WThing*)wwin);
}


/*}}}*/

