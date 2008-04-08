/*
 * ion/key.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <ctype.h>

#include "common.h"
#include "binding.h"
#include "global.h"
#include "key.h"
#include "event.h"
#include "cursor.h"
#include "client.h"
#include "grab.h"

/*{{{ prototypes */

static void waitrelease(WScreen *screen);

/*}}}*/

void adhoc_insstr(WEdln *wedln, XKeyEvent *ev)
{
	static XComposeStatus cs={NULL, 0};
	char buf[16]={0,};
	Status stat;
	int n;
	KeySym ksym;
	
	if(wedln->input.win.xic!=NULL){
		if(XFilterEvent((XEvent*)ev, ev->window))
		   return;
		n=XmbLookupString(wedln->input.win.xic, ev, buf, 16, &ksym, &stat);
	}else{
		n=XLookupString(ev, buf, 16, &ksym, &cs);
	}
	
	if(n<=0 || *(uchar*)buf<32)
		return;
	
	edln_insstr(&(wedln->edln), buf);
}

static WBinding *lookup_binding_from_event(WWindow *thing, XKeyEvent *ev)
{
	WBinding *binding;
	WBindmap **bindptr;

	bindptr=&thing->bindmap;
	assert(*bindptr!=NULL);

	binding=lookup_binding(*bindptr, ACT_KEYPRESS, ev->state, ev->keycode);
	return binding;
}

/* dispatch_binding
 * the return values are those expected by GrabHandler's, i.e.
 * you can just pass through the retval obtained from this function
 */
static bool dispatch_binding(WThing *thing, WBinding *binding, XKeyEvent *ev)
{
	WScreen *scr;

	if(binding){
		/* Get the screen now for waitrel grab - the thing might
		 * have been destroyed when call_binding returns.
		 */
		scr=SCREEN_OF(thing);
		call_binding(binding, thing);
		if(ev->state!=0 && binding->waitrel){
			waitrelease(scr);
			/* return FALSE here to prevent uninstalling the waitrelease handler
               immediately after establishing it */
			return FALSE;
		}
	}
	return TRUE;
}

static void send_key(XEvent *ev, WClientWin *cwin)
{
	Window win=cwin->win;
	ev->xkey.window=win;
	ev->xkey.subwindow=None;
	XSendEvent(wglobal.dpy, win, False, KeyPressMask, ev);
}


static bool quote_next_handler(WThing *thing, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
 	if(ev->type==KeyRelease)
		return TRUE;
	if(ismod(ev->keycode))
		return FALSE;
	assert(WTHING_IS(thing, WClientWin));
	send_key(xev, (WClientWin*)thing);
	return TRUE; /* remove the grab */
}

void quote_next(WClient *client)
{
	WClientWin *cwin=LAST_THING(client, WClientWin);

	if(cwin==NULL)
		return;
	
	grab_establish((WThing*)cwin, quote_next_handler, FocusChangeMask);
}
			   
static bool waitrelease_handler(WThing *thing, XEvent *ev)
{
	if(!unmod(ev->xkey.state, ev->xkey.keycode))
		return TRUE;
	return FALSE;
}

static void waitrelease(WScreen *screen)
{
	grab_establish((WThing *)screen, waitrelease_handler, FocusChangeMask);
}

static bool submapgrab_handler(WThing *thing, XEvent *ev)
{
	WBinding *binding;

	/*if(ev->type==KeyRelease)
		return FALSE;*/

	binding=lookup_binding_from_event((WWindow*)thing, &ev->xkey);

	if(binding==NULL)
		if(ismod(ev->xkey.keycode)){
			return FALSE;
		}
	
	return dispatch_binding(thing, binding, &ev->xkey);
}

static void submapgrab(WThing *thing)
{
	grab_establish(thing, submapgrab_handler, FocusChangeMask|KeyReleaseMask);
}

void handle_keypress(XKeyEvent *ev)
{
	WThing *thing=NULL;
	WScreen *scr;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	bool topmap=TRUE;
	bool toplvl=FALSE;
	
	/* this function gets called with grab_holder==NULL */
	
	thing=FIND_WINDOW(ev->window);
	if(thing==NULL || !WTHING_IS(thing, WWindow))
		return;
	
	toplvl=WTHING_IS(thing, WFrame);
	bindptr=&(((WWindow*)thing)->bindmap);
	   
	while((*bindptr)->parent!=NULL){
		topmap=FALSE;
		*bindptr=(*bindptr)->parent;
	}
	
	binding=lookup_binding_from_event((WWindow*)thing, ev);


	if(binding!=NULL && binding->submap!=NULL){
		*bindptr=binding->submap;
		if(toplvl){
			submapgrab(thing);
		}
		return;
	}

	if(binding!=NULL)
		dispatch_binding(thing, binding, ev);
	else{
		if(topmap && WTHING_IS(thing, WEdln))
			adhoc_insstr((WEdln*)thing, ev);
	}
}


