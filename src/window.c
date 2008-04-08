/*
 * ion/window.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "window.h"
#include "focus.h"
#include "thingp.h"


static WThingFuntab window_funtab={
	deinit_window,
	NULL
};

IMPLOBJ(WWindow, WThing, &window_funtab)


/*{{{ Init, create */


bool init_window(WWindow *p, Window win, WRectangle geom)
{
	p->flags=0;
	p->win=win;
	p->geom=geom;
	p->xic=NULL;
	p->split=NULL;
	p->bindmap=NULL;
	
	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)p);
	
	return TRUE;
}


void deinit_window(WWindow *wwin)
{
	if(wwin->xic!=NULL)
		XDestroyIC(wwin->xic);

	if(wglobal.current_wswindow==wwin)
		wglobal.current_wswindow=NULL;

	XDeleteContext(wglobal.dpy, wwin->win, wglobal.win_context);
	XDestroyWindow(wglobal.dpy, wwin->win);
}


/*}}}*/


/*{{{ Find, X Window -> thing */


WThing *find_window(Window win)
{
	WThing *thing;
	
	if(XFindContext(wglobal.dpy, win, wglobal.win_context,
					(XPointer*)&thing)!=0)
		return NULL;
	
	return thing;
}


WThing *find_window_t(Window win, const WObjDescr *descr)
{
	WThing *thing=find_window(win);
	
	if(thing==NULL)
		return NULL;
	
	if(!wobj_is((WObj*)thing, descr))
		return NULL;
	
	return thing;
}


WFrame *find_frame_of(Window win)
{
	WWindow *window=find_window_of(win);
	
	if(window==NULL || !WTHING_IS(window, WFrame))
		return NULL;
	
	return (WFrame*)window;
}


WWindow *find_window_of(Window win)
{
	WThing *thing=FIND_WINDOW_T(win, WThing);
	
	if(thing==NULL)
		return NULL;
	
	return window_of(thing);
}


WWindow *window_of(WThing *thing)
{
	while(1){
		if(thing->t_parent==NULL)
			return NULL;
		
		if(WTHING_IS(thing->t_parent, WWorkspace))
			break;
		
		thing=thing->t_parent;
	}
	
	if(WTHING_IS(thing, WWindow))
		return (WWindow*)thing;
	
	return NULL;
}


/*}}}*/


/*{{{ Mapping */


void do_map_window(WWindow *wwin)
{
	XMapWindow(wglobal.dpy, wwin->win);
	
	wwin->flags|=WWINDOW_MAPPED;
}


void do_unmap_window(WWindow *wwin)
{
	WFrame *frame;

	if(!(wwin->flags&WWINDOW_MAPPED))
		return;

	XUnmapWindow(wglobal.dpy, wwin->win);

	wwin->flags&=~WWINDOW_MAPPED;

	/*if(wglobal.current_wswindow==wwin)
		wglobal.current_wswindow=NULL;*/
}


void map_window(WWindow *wwin)
{
	if(wwin->flags&WWINDOW_UNMAPPABLE)
		return;
	
	do_map_window(wwin);
}


void unmap_window(WWindow *wwin)
{
	do_unmap_window(wwin);
}


/*}}}*/


/*{{{ Misc */


void focus_window(WWindow *wwin)
{
	set_input_focus(wwin->win);
}


/*}}}*/

