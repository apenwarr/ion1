/*
 * ion/event.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

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
#include "grab.h"
#include "objp.h"

/*{{{ Prototypes */


static void handle_expose(const XExposeEvent *ev);
static void handle_map_request(const XMapRequestEvent *ev);
static void handle_configure_request(XConfigureRequestEvent *ev);
static void handle_enter_window(XEvent *ev);
static void handle_unmap_notify(const XUnmapEvent *ev);
static void handle_destroy_notify(const XDestroyWindowEvent *ev);
static void handle_client_message(const XClientMessageEvent *ev);
static void handle_focus_in(const XFocusChangeEvent *ev);
static void handle_focus_out(const XFocusChangeEvent *ev);
static void handle_property(const XPropertyEvent *ev);
static void handle_colormap_notify(const XColormapEvent *ev);

/*}}}*/

InputHandler default_input_handler={
	default_keyboard_handler,
	default_pointer_handler
};

void set_input_handler(InputHandler *handler, InputHandlerContext *context)
{
	context->oldhandler=wglobal.input_handler;
	wglobal.input_handler=handler;
}

void restore_input_handler(InputHandlerContext *context)
{
	wglobal.input_handler=context->oldhandler;
}

/*{{{ Event-reading, mainloop */

void get_event(XEvent *ev)
{
	fd_set rfds;
	int nfds=wglobal.conn;
	
	while(1){
		check_signals();
	
		if(QLength(wglobal.dpy)>0){
			XNextEvent(wglobal.dpy, ev);
			return;
		}
		
		XFlush(wglobal.dpy);

		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);

 		set_input_fds(&rfds, &nfds);
 		
 		if(select(nfds+1, &rfds, NULL, NULL, NULL)>0){
 			check_input_fds(&rfds);
 			if(FD_ISSET(wglobal.conn, &rfds)){
 				XNextEvent(wglobal.dpy, ev);
 				return;
 			}
		}
	}
}


void get_event_mask(XEvent *ev, long mask)
{
	fd_set rfds;
	bool found=FALSE;
	
	while(1){
		check_signals();
		
		while(XCheckMaskEvent(wglobal.dpy, mask, ev)){
			if(ev->type!=MotionNotify)
				return;
			found=TRUE;
		}

		if(found)
			return;
		
		FD_ZERO(&rfds);
		FD_SET(wglobal.conn, &rfds);

		select(wglobal.conn+1, &rfds, NULL, NULL, NULL);
	}
}


#define CASE_EVENT(X) case X: 
/*	fprintf(stderr, "[%#lx] %s\n", ev->xany.window, #X);*/


void kill_focusenter_events()
{
	XEvent ev;
	while(XCheckMaskEvent(wglobal.dpy, EnterWindowMask|FocusChangeMask, &ev)) /*nothing */;
}

/* 
 * Workspace switching and focusing with CF_WARP.
 * 
 * - switch_workspace sets focus (immediately) to previous active
 *   winobj on workspace.
 * - skip_focusenter should receive an enter event and focus to the
 *   window that is really wanted to have the focus.
 */

#if 0
void skip_focusenter()
{
	XEvent ev;
	XEvent tmp;
	
	tmp.type=None;
	
	if(wglobal.current_wswindow==NULL ||
	   !on_active_workspace((WThing*)wglobal.current_wswindow))
		return;

	XSync(wglobal.dpy, False);
	
	while(XCheckMaskEvent(wglobal.dpy,
						  EnterWindowMask|FocusChangeMask, &ev)){
	#ifdef CF_WARP
		if(ev.type==EnterNotify && wglobal.focus_next==NULL){
			protect_previous();
			handle_enter_window(&ev);
			unprotect_previous();
			XFlush(wglobal.dpy);
		}else
	#endif
		
		/* Have to handle last focus in or else we may not get
		 * correct colormap.
		 */
		if(ev.type==FocusIn && wglobal.focus_next==NULL)
			memcpy(&tmp, &ev, sizeof(tmp));
	}
	
	if(tmp.type!=None)
		handle_focus_in(&(tmp.xfocus));
}
#endif

void handle_event(XEvent *ev)
{
	switch(ev->type){
	CASE_EVENT(MapRequest)
		handle_map_request(&(ev->xmaprequest));
		break;
	CASE_EVENT(ConfigureRequest)
		handle_configure_request(&(ev->xconfigurerequest));
		break;
	CASE_EVENT(UnmapNotify)
		handle_unmap_notify(&(ev->xunmap));
		break;
	CASE_EVENT(DestroyNotify)
		handle_destroy_notify(&(ev->xdestroywindow));
		break;
	CASE_EVENT(ClientMessage)
		handle_client_message(&(ev->xclient));
		break;
	CASE_EVENT(PropertyNotify)
		handle_property(&(ev->xproperty));
		break;
	CASE_EVENT(FocusIn)
		handle_focus_in(&(ev->xfocus));
		break;
	CASE_EVENT(FocusOut)
		handle_focus_out(&(ev->xfocus));
		break;
	CASE_EVENT(EnterNotify)
		handle_enter_window(ev);
		break;
	CASE_EVENT(Expose)		
		handle_expose(&(ev->xexpose));
		break;
	CASE_EVENT(KeyPress)
		assert(wglobal.input_handler!=NULL);
		assert(wglobal.input_handler->keyboard!=NULL);
		wglobal.input_handler->keyboard(ev);
		break;
	CASE_EVENT(KeyRelease)
		assert(wglobal.input_handler!=NULL);
		assert(wglobal.input_handler->keyboard!=NULL);
		wglobal.input_handler->keyboard(ev);
		break;
	CASE_EVENT(ButtonPress)
		assert(wglobal.input_handler!=NULL);
		if(wglobal.input_handler->pointer!=NULL)
			wglobal.input_handler->pointer(ev);
		break;
	CASE_EVENT(ColormapNotify)
		handle_colormap_notify(&(ev->xcolormap));
		break;
	CASE_EVENT(MappingNotify)
		XRefreshKeyboardMapping(&(ev->xmapping));
		update_modmap();
		break;
	CASE_EVENT(SelectionClear)
		clear_selection();
		break;
	CASE_EVENT(SelectionNotify)
		receive_selection(&(ev->xselection));
		break;
	CASE_EVENT(SelectionRequest)
		send_selection(&(ev->xselectionrequest));
		break;
	}
}


void mainloop()
{
	XEvent ev;
	
	for(;;){
		get_event(&ev);
		handle_event(&ev);
		
		XSync(wglobal.dpy, False);

		if(wglobal.focus_next!=NULL){
			kill_focusenter_events();
			do_set_focus(wglobal.focus_next);
			wglobal.focus_next=NULL;
		}
	}
}


/*}}}*/


/*{{{ Map, unmap, destroy */


static void handle_map_request(const XMapRequestEvent *ev)
{
	WThing *thing;
	
	thing=FIND_WINDOW(ev->window);
	
	if(thing!=NULL)
		return;
	
	manage_clientwin(ev->window, 0);
}


static void handle_unmap_notify(const XUnmapEvent *ev)
{
	WClientWin *cwin;

	/* We are not interested in SubstructureNotify -unmaps. */
	if(ev->event!=ev->window && ev->send_event!=True)
		return;

	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL)
		return;

	unmap_clientwin(cwin);
}


static void handle_destroy_notify(const XDestroyWindowEvent *ev)
{
	WClientWin *cwin;

	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL)
		return;
	
	destroy_clientwin(cwin);
}


/*}}}*/


/*{{{ Client configure/property/message */


static void handle_configure_request(XConfigureRequestEvent *ev)
{
	WClientWin *cwin;
	XWindowChanges wc;
	
	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL){
		wc.border_width=ev->border_width;
		wc.sibling=None;
		wc.stack_mode=ev->detail;
		wc.x=ev->x;
		wc.y=ev->y;
		wc.width=ev->width;
		wc.height=ev->height;
		XConfigureWindow(wglobal.dpy, ev->window, ev->value_mask, &wc);
		return;
	}

	if((ev->value_mask&(CWWidth|CWHeight|CWBorderWidth))!=0){
		/* Transients will not get resized properly unless the
		 * wanted sizes are first set and later modified to fit
		 */
		if(ev->value_mask&CWWidth)
			cwin->geom.w=ev->width;
		if(ev->value_mask&CWHeight)
			cwin->geom.h=ev->height;
		if(ev->value_mask&CWBorderWidth)
			cwin->orig_bw=ev->border_width;
		refit(cwin, cwin->geom.w, cwin->geom.h);
	}else{
		sendconfig_clientwin(cwin);
	}
}


static void handle_client_message(const XClientMessageEvent *ev)
{
	WClientWin *cwin;

	if(ev->message_type!=wglobal.atom_wm_change_state)
		return;
	
	cwin=find_clientwin(ev->window);

	if(cwin==NULL)
		return;
	
	if(ev->format==32 && ev->data.l[0]==IconicState){
		if(cwin->state==NormalState)
			iconify_clientwin(cwin);
	}
}


static void handle_property(const XPropertyEvent *ev)
{
	WClientWin *cwin;
	WClient *client;
	WScreen *scr;
	
	cwin=find_clientwin(ev->window);
	
	if(cwin==NULL)
		return;
	
	switch(ev->atom){
	case XA_WM_NORMAL_HINTS:
		get_clientwin_size_hints(cwin);
		/*refit(cwin);*/
		return;
	
	case XA_WM_NAME:
		/*if(cwin->name!=NULL)
			XFree((void*)cwin->name);
		cwin->name=get_string_property(cwin->win, XA_WM_NAME, NULL);*/
		set_clientwin_name(cwin, get_string_property(cwin->win, XA_WM_NAME,
													 NULL));
		break;
		
	/*case XA_WM_ICON_NAME:
		if(cwin->icon_name!=NULL)
			XFree((void*)cwin->icon_name);
		cwin->icon_name=get_string_property(cwin->win, XA_WM_ICON_NAME, NULL);
		break;*/

	case XA_WM_TRANSIENT_FOR:
		/*warn("Changes in WM_TRANSIENT_FOR property are unsupported.");*/
		/*unmap_clientwin(cwin);
		manage_clientwin(ev->window, 0);*/
		
	default:
		if(ev->atom==wglobal.atom_wm_protocols)
			get_protocols(cwin);
		return;
	}
	
	/*client=FIND_PARENT(cwin, WClient);
	if(client!=NULL)
		client_update_label(client);*/
}


/*}}}*/


/*{{{ Colormap */


static void install_cmap(WScreen *scr, Colormap cmap)
{
	if(cmap==None)
		cmap=scr->default_cmap;
	
	XInstallColormap(wglobal.dpy, cmap);
}


static bool focused_clientwin(WClientWin *cwin)
{
	WClient *client=FIND_PARENT(cwin, WClient);
	WFrame *frame;
	
	if(client==NULL || LAST_THING(client, WClientWin)!=cwin)
		return FALSE;
	
	frame=FIND_PARENT(client, WFrame);
	
	return (frame!=NULL && frame->current_client==client &&
			IS_ACTIVE_FRAME(frame));
}


static void set_cmap(WClientWin *cwin, Colormap cmap)
{
	cwin->cmap=cmap;
	if(focused_clientwin(cwin))
		install_cmap(SCREEN_OF(cwin), cwin->cmap);
}


static void handle_colormap_notify(const XColormapEvent *ev)
{
	WClientWin *cwin;

	if(!ev->new)
		return;

	cwin=find_clientwin(ev->window);

	if(cwin!=NULL)
		set_cmap(cwin, ev->colormap);
}


/*}}}*/


/*{{{ Expose */


static void redraw_wwin(WWindow *wwin)
{
	if(WTHING_IS(wwin, WFrame))
		draw_frame((WFrame*)wwin, FALSE);
	else if(WTHING_IS(wwin, WInput))
		input_draw((WInput*)wwin, FALSE);
}


static void handle_expose(const XExposeEvent *ev)
{
	WWindow *wwin;
	WScreen *scr;
	XEvent tmp;
	
	while(XCheckWindowEvent(wglobal.dpy, ev->window, ExposureMask, &tmp))
		/* nothing */;

	wwin=FIND_WINDOW_T(ev->window, WWindow);

	if(wwin!=NULL){
		redraw_wwin(wwin);
		return;
	}
	
	if(wglobal.grab_holder==NULL || !WTHING_IS(wglobal.grab_holder, WClient))
		return;
	
	FOR_ALL_SCREENS(scr){
		if(scr->grdata.tabdrag_win==ev->window){
			draw_tabdrag((WClient*)wglobal.grab_holder);
			break;
		}
	}
}


/*}}}*/


/*{{{ Enter window, focus */

static void handle_enter_window(XEvent *ev)
{
	XEnterWindowEvent *eev=&(ev->xcrossing);
	WThing *thing=NULL;
	bool inf=TRUE;
	
#if 0
	/* when enabling this check you can't change focus by moving the pointer when in command mode */
	if(grab_held())
		return;
#endif

	if(eev->mode==NotifyGrab)
		return;
	
	/*
	 * mode==NotifyUngrab events are generated en masse on XUngrab*()
	 * and are generally ignored; unfortunately a refocussing
	 * ButtonRelease usually happens in this mode too. argh.
	 */
	if(eev->mode==NotifyUngrab)
		return;

	do{
		if(eev->detail!=NotifyInferior)/* && !eev->send_event)*/
			inf=FALSE;
	}while(XCheckMaskEvent(wglobal.dpy, EnterWindowMask, ev));

	if(inf)
		return;

	if(eev->window==eev->root){
		/* Ignore root window enter */
		return;
	}
	

	thing=FIND_WINDOW_T(eev->window, WThing);
	
	if(thing==NULL)
		return;

	set_previous(thing);
	set_focus(thing);
}


static void activate(WWindow *wwin)
{
	set_current_wswindow(wwin);
	if(WTHING_IS(wwin, WFrame))
		activate_frame((WFrame*)wwin);
}


static void deactivate(WWindow *wwin)
{
	wglobal.current_wswindow=NULL;
	if(WTHING_IS(wwin, WFrame))
		deactivate_frame((WFrame*)wwin);
}


static void restore_focus(WScreen *scr)
{
	WWorkspace *ws=scr->current_workspace;
	WWindow *wwin;
	
	if(ws==NULL)
		return;
	
	wwin=find_current(ws);
	if(wwin==NULL)
		return;
	
	set_focus((WThing*)wwin);
	
}
 

static void handle_focus_in(const XFocusChangeEvent *ev)
{
	WThing *thing;
	WWindow *wwin;
	WScreen *scr;
	Colormap cmap=None;

	if(ev->mode==NotifyGrab || ev->detail > NotifyNonlinearVirtual)
		return;
	
	thing=FIND_WINDOW_T(ev->window, WThing);
	
	if(thing==NULL)
		return;

	/* Set current screen */
	scr=SCREEN_OF(thing);
	wglobal.current_screen=scr;
	
    if(ev->window==scr->root.win){
		if(ev->detail!=NotifyNonlinear && ev->detail!=NotifyNonlinearVirtual)
			restore_focus(scr);
		return;
	}
	
	if(ev->mode==NotifyUngrab){
		if(WTHING_IS(thing, WFrame) && !IS_ACTIVE_FRAME(thing))
			restore_focus(scr);
		return;
	}
	
	/* Handle colormap */
	if(WTHING_IS(thing, WClientWin))
		cmap=((WClientWin*)thing)->cmap;
	
	install_cmap(scr, cmap);

	/* Set active WWindow etc. */
	if(WTHING_IS(thing, WWindow)){
		wwin=(WWindow*)thing;
		if(wwin->xic!=NULL)
			XSetICFocus(wwin->xic);
	}
	
	wwin=window_of(thing);
	
	if(wglobal.current_wswindow==wwin)
		return;
	
	if(wglobal.current_wswindow!=NULL)
		deactivate(wglobal.current_wswindow);

	if(wwin!=NULL)
		activate(wwin);
}

static void handle_focus_out(const XFocusChangeEvent *ev)
{
	WWindow *wwin;
	WScreen *scr;

	/*if(ev->window==SCREEN->root.win){
		SCREEN->active=FALSE;
		wwin=wglobal.current_wswindow;
		if(wwin!=NULL)
			redraw_wwin(wwin);
		return;
	}*/

	if(ev->mode==NotifyGrab || ev->mode==NotifyUngrab || ev->detail>NotifyNonlinearVirtual)
		return;

	wwin=FIND_WINDOW_T(ev->window, WWindow);
	
	if(wwin==NULL)
		return;
	
	scr=SCREEN_OF(wwin);
	if(ev->window==scr->root.win)
		return;

	if(wwin->xic!=NULL)
		XUnsetICFocus(wwin->xic);
}


/*}}}*/


/*{{{ Pointer, keyboard */


void do_grab_kb_ptr(Window win, WThing *thing, long eventmask)
{
	wglobal.grab_holder=thing;
	wglobal.input_mode=INPUT_NORMAL;
	
	XSelectInput(wglobal.dpy, win, ROOT_MASK&~eventmask);
	XGrabPointer(wglobal.dpy, win, True, GRAB_POINTER_MASK,
				 GrabModeAsync, GrabModeAsync, win,
				 x_cursor(CURSOR_DEFAULT), CurrentTime);
	XGrabKeyboard(wglobal.dpy, win, False, GrabModeAsync,
				  GrabModeAsync, CurrentTime);
	XSelectInput(wglobal.dpy, win, ROOT_MASK);
}


void grab_kb_ptr(WThing *thing)
{
	do_grab_kb_ptr(ROOT_OF(thing), thing, FocusChangeMask);
}


void ungrab_kb_ptr()
{
	XUngrabKeyboard(wglobal.dpy, CurrentTime);
	XUngrabPointer(wglobal.dpy, CurrentTime);
	
	wglobal.grab_holder=NULL;
	wglobal.input_mode=INPUT_NORMAL;
}


#define GRAB_EV_MASK (GRAB_POINTER_MASK|ExposureMask|  \
					  KeyPressMask|KeyReleaseMask|     \
					  EnterWindowMask|FocusChangeMask)
	
void default_pointer_handler(XEvent *ev)
{
	XEvent tmp;
	Window win_pressed;
	WThing *t;
	bool mouse_grab_held=FALSE;
	
	if(grab_held())
		return;
	
	win_pressed=ev->xbutton.window;
	if(!handle_button_press(&ev->xbutton))
		return;

	mouse_grab_held=TRUE;

	while(mouse_grab_held){
		XFlush(wglobal.dpy);
		get_event_mask(ev, GRAB_EV_MASK);
		
		switch(ev->type){
		CASE_EVENT(ButtonRelease)
			if(handle_button_release(&(ev->xbutton))){
				ungrab_kb_ptr();
				mouse_grab_held=FALSE;
			}
			break;
		CASE_EVENT(MotionNotify)
			handle_pointer_motion(&(ev->xmotion));
			break;
		CASE_EVENT(Expose)		
			handle_expose(&(ev->xexpose));
			break;
		}
	}
}


void default_keyboard_handler(XEvent *ev)
{
	if(call_grab_handler(ev))
		return;
		
	if(ev->type==KeyPress)
		handle_keypress(&(ev->xkey));
	
	/*skip_focusenter();*/
}


/*}}}*/
