/*
 * ion/command.c
 *
 * Copyright (c) Lukas Schroeder 2002
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "event.h"
#include "clientwin.h"
#include "screen.h"
#include "frame.h"
#include "property.h"
#include "pointer.h"
#include "key.h"
#include "focus.h"
#include "cursor.h"
#include "signal.h"
#include "global.h"
#include "draw.h"
#include "input.h"
#include "selection.h"
#include "thing.h"
#include "sizehint.h"
#include "readfds.h"
#include "wedln.h"
#include "window.h"
#include "objp.h"
#include "grab.h"

/*{{{ Prototypes */

static bool commandmode_keyboard_handler(WThing *thing, XEvent *ev);

/*}}}*/

/* {{{ Definitions and friends */

static bool commandmode_active=FALSE;

WBindmap commandmode_bindmap;

/*}}}*/

/*{{{ auxiliary functions */

static WThing *get_current_from_thing(WThing *thing)
{
	WScreen *scr;
	scr=SCREEN_OF(thing);
	return (WThing*)find_current(scr->current_workspace);
}

/*}}}*/


/*{{{ init/deinit,enter/leave */

void commandmode_enter(WThing *thing)
{
	WScreen *scr;
	WThing *wwin;

	if(commandmode_active==TRUE){
		fprintf(stderr, "%s() commandmode already active!\n", __FUNCTION__);
		return;
	}

	if(wglobal.input_mode!=INPUT_NORMAL){
		fprintf(stderr, "%s() can only enter commandmode from normal input mode!\n", __FUNCTION__);
		return;
	}

	if(!WTHING_IS(thing, WWindow)){
		scr=SCREEN_OF(thing);
		wwin=(WThing*)find_current(scr->current_workspace);
		thing=wwin;
	}
	
	commandmode_active=TRUE;
	grab_establish(thing, commandmode_keyboard_handler, FocusChangeMask);

	wglobal.input_mode=INPUT_NORMAL;
}

void commandmode_leave(WThing *thing)
{
	if(commandmode_active){
		commandmode_active=FALSE;
		grab_remove(commandmode_keyboard_handler);
	}
}

/*}}}*/


/*{{{ event handling */

static void commandmode_keypress_handler(WThing *thing, XKeyEvent *ev)
{
	WScreen *scr;
	WBinding *binding=NULL;
	WBindmap **bindptr;
	bool topmap=TRUE;
	bool toplvl=FALSE;

	if(ev->type==KeyRelease)
		return;

	/* 
	 * when holding a grab ev->window will be the window holding the pointer
	 * (X's PointerRoot focus mode); this can either be a window
	 * handled by the window manager or a Window of an application, and "thing"
	 * will just be too old an information if e.g. the focus changed while
	 * holding the command-mode grab
	 * so, to get the right context for call_binding() we'll do a lookup to find
	 * the current WWindow...
	 */

	thing=get_current_from_thing(thing);
	if(!thing || !WTHING_IS(thing, WWindow))
		return;
	
	bindptr=&(((WWindow*)thing)->bindmap);
	toplvl=WTHING_IS(thing, WFrame);

	if(!thing || !WTHING_IS(thing, WEdln))
		binding=lookup_binding(&commandmode_bindmap, ACT_KEYPRESS, ev->state, ev->keycode);
	else
		binding=lookup_binding(*bindptr, ACT_KEYPRESS, ev->state, ev->keycode);

	if(!binding){
		if(ismod(ev->keycode))
			return;
	}

	if(bindptr && *bindptr){
		while((*bindptr)->parent!=NULL){
			topmap=FALSE;
			*bindptr=(*bindptr)->parent;
		}
	}
	
	if(binding!=NULL && binding->submap!=NULL){
#if 0
/* we dont allow submap grabs in commandmode; (yet?) */
		*bindptr=binding->submap;
		if(toplvl && wglobal.input_mode==INPUT_NORMAL){
			grab_kb_ptr(thing);
			change_grab_cursor(CURSOR_WAITKEY);
			wglobal.input_mode=INPUT_SUBMAPGRAB;
		}
#endif
		return;
	}

	if(binding!=NULL){
		/* Get the screen now for waitrel grab - the thing might
		 * have been destroyed when call_binding returns.
		 */
		scr=SCREEN_OF(thing);
		call_binding(binding, thing);
	}else{
		if(topmap && WTHING_IS(thing, WEdln))
			adhoc_insstr((WEdln*)thing, ev);
	}
}

static Bool commandmode_event_scanner(Display *dpy, XEvent *ev, char *args)
{
	switch(ev->type){
	case FocusIn:
	case FocusOut:
		return (ev->xfocus.mode==NotifyUngrab);
		break;
	case EnterNotify:
		return (ev->xcrossing.mode==NotifyUngrab);
		break;
	default:
		break;
	}
	return False;
}

static bool commandmode_keyboard_handler(WThing *thing, XEvent *ev)
{
	/* we get called with commandmode_active==TRUE, if there was
	   another GrabHandler active at commandmode_leave() time */
	if(commandmode_active==FALSE)
		return TRUE;

	if(ev->type==KeyPress)
		commandmode_keypress_handler(thing, &ev->xkey);

	if(!commandmode_active){
		/* prevent FocusIn, FocusOut, EnterNotify events at command-mode-leave time;
		 * not doing so would result in flipping the focus to the window containing
		 * the pointer (X's PointerRoot mode, when grab's are held).
		 */
		XEvent tmp;
		XSync(wglobal.dpy, False);
		while(XCheckIfEvent(wglobal.dpy, &tmp, commandmode_event_scanner, NULL));

		/* enforce refocus on the "current thing"; this is necessary to get
		 * the focussing of WClientWin's right when the commandmode was used to
		 * "switch_tab" only and the clients received their FocusOut.
		 */
		thing=get_current_from_thing(thing);
		if(thing && WTHING_IS(thing, WWindow))
			set_focus(thing);
	}
	return !commandmode_active;
}

/*}}}*/
