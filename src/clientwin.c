/*
 * ion/clientwin.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <limits.h>

#include "common.h"
#include "clientwin.h"
#include "client.h"
#include "frame.h"
#include "frameid.h"
#include "property.h"
#include "event.h"
#include "focus.h"
#include "winprops.h"
#include "sizehint.h"
#include "global.h"
#include "thingp.h"
#include "splitmisc.h"
#include "modules.h"


static WThingFuntab clientwin_funtab={
	deinit_clientwin, 
	NULL
};

IMPLOBJ(WClientWin, WThing, &clientwin_funtab)

static void set_clientwin_state(WClientWin *cwin, int state);
static void send_clientmsg(Window win, Atom a);


/*{{{ Get properties */


void get_protocols(WClientWin *cwin)
{
	Atom *protocols=NULL, *p;
	int n;
	
	cwin->flags&=~(CWIN_P_WM_DELETE|CWIN_P_WM_TAKE_FOCUS);
	
	if(!XGetWMProtocols(wglobal.dpy, cwin->win, &protocols, &n))
		return;
	
	for(p=protocols; n; n--, p++){
		if(*p==wglobal.atom_wm_delete)
			cwin->flags|=CWIN_P_WM_DELETE;
		else if(*p==wglobal.atom_wm_take_focus)
			cwin->flags|=CWIN_P_WM_TAKE_FOCUS;
	}
	
	if(protocols!=NULL)
		XFree((char*)protocols);
}


static void set_winprops(WClientWin *cwin, const WWinProp *winprop)
{
	cwin->flags|=winprop->flags;
	
	if(cwin->flags&CWIN_PROP_MAXSIZE){
		cwin->size_hints.max_width=winprop->max_w;
		cwin->size_hints.max_height=winprop->max_h;
		cwin->size_hints.flags|=PMaxSize;
	}

	if(cwin->flags&CWIN_PROP_ASPECT){
		cwin->size_hints.min_aspect.x=winprop->aspect_w;
		cwin->size_hints.max_aspect.x=winprop->aspect_w;
		cwin->size_hints.min_aspect.y=winprop->aspect_h;
		cwin->size_hints.max_aspect.y=winprop->aspect_h;
		cwin->size_hints.flags|=PAspect;
	}
}


/*}}}*/


/*{{{ Manage/create */


static void configure_cwin_bw(Window win, int bw)
{
	XWindowChanges wc;
	ulong wcmask=CWBorderWidth;
	
	wc.border_width=bw;
	XConfigureWindow(wglobal.dpy, win, wcmask, &wc);
}


static bool init_clientwin(WClientWin *cwin, Window win, int flags,
						   const XWindowAttributes *attr, const WWinProp *props)
{
	cwin->flags=flags;
	cwin->win=win;
	cwin->orig_bw=attr->border_width;
	cwin->geom.h=attr->height;
	cwin->geom.w=attr->width;
	cwin->cmap=attr->colormap;
	cwin->name=get_string_property(cwin->win, XA_WM_NAME, NULL);
	if(cwin->name!=NULL)
		stripws(cwin->name);
	cwin->event_mask=CLIENT_MASK;
	cwin->transient_for=None;
	cwin->state=WithdrawnState;
	
	if(props!=NULL)
		set_winprops(cwin, props);
	
	get_clientwin_size_hints(cwin);
	get_protocols(cwin);

	XSelectInput(wglobal.dpy, win, cwin->event_mask);

	XSaveContext(wglobal.dpy, win, wglobal.win_context, (XPointer)cwin);
	XAddToSaveSet(wglobal.dpy, win);

	if(cwin->orig_bw!=0)
		configure_cwin_bw(win, 0);
	
	return TRUE;
}


WClientWin *create_clientwin(WScreen *scr,
							 Window win, int flags,
							 const XWindowAttributes *attr,
							 const WWinProp *props)
{
	CREATETHING_IMPL(WClientWin, clientwin, scr,
					 (p, win, flags, attr, props));
}

				   
static bool add_clientwin(WClientWin *cwin, int state, 
						  WWinProp *props, XWindowAttributes *attr)
{
	int frame_id=0;
	Window win=cwin->win;
	WClient *client=NULL;
	WClientWin *tfor;
	WFrame *frame=NULL;
	WWorkspace *ws=NULL;
#ifdef CF_SWITCH_NEW_CLIENTS
	bool switchto=TRUE;
#else
	bool switchto=FALSE;
#endif
	WWindow *tmp_wwin;
	int transient_mode=TRANSIENT_MODE_NORMAL;
	
	if(props!=NULL)
		transient_mode=props->transient_mode;

	if(transient_mode!=TRANSIENT_MODE_CURRENT){
		cwin->transient_for=None;
		if(XGetTransientForHint(wglobal.dpy, win, &(cwin->transient_for))){
			tfor=find_clientwin(cwin->transient_for);
			if(tfor!=NULL && transient_mode==TRANSIENT_MODE_NORMAL){
				if(CLIENTWIN_HAS_CLIENT(tfor))
					client=CLIENTWIN_CLIENT(tfor);
			}else{
				cwin->transient_for=None;
			}
		}
	}else{ /*TRANSIENT_MODE_CURRENT*/
		tmp_wwin=find_current(SCREEN_OF(cwin)->current_workspace);
		if(WTHING_IS(tmp_wwin, WFrame)){
			client=((WFrame*)tmp_wwin)->current_client;
			cwin->transient_for=tmp_wwin->win;
		}
	}
	
	get_integer_property(win, wglobal.atom_frame_id, &frame_id);
	
	/* Get client to place this window in */
	if(client==NULL){
		/* Frame first */
		if(frame_id!=0)
			frame=find_frame_by_id(frame_id);
		
		if(frame!=NULL){
			if(SCREEN_OF(frame)!=SCREEN_OF(cwin)){
				warn("The frame id property of window %#x is set to"
					 "a frame on different screen", cwin->win);
				frame=NULL;
			}
		}
			
		if(frame==NULL){
			bool geomset;
			ws=SCREEN_OF(cwin)->current_workspace;
			
#ifdef CF_PLACEMENT_GEOM
			geomset=(cwin->size_hints.win_gravity!=ForgetGravity &&
					 (attr->x>CF_STUBBORN_TRESH &&
					  attr->y>CF_STUBBORN_TRESH));
			if(ws!=NULL && geomset && (!props || !props->stubborn))
				frame=find_frame_at(ws, attr->x, attr->y);
		}
		if(frame==NULL){
#else

#endif
			if(ws!=NULL)
				frame=(WFrame*)find_current(ws);
		
			if(frame==NULL || !WTHING_IS(frame, WFrame)){
				warn("No client-supplied frame for window %d and no"
					 "current frame", win);
				return FALSE;
			}
		}
		
		/* Create the client, don't attach yet */
		client=create_client(SCREEN_OF(cwin));
		
		if(client==NULL)
			return FALSE;
	}

	client_add_clientwin(client, cwin);
	
	/* Attach, switch and focus */
	if(frame!=NULL){
		/* frame!=NULL -> new client */
		if(wglobal.opmode==OPMODE_INIT)
			switchto=(state!=IconicState);
		else if(props!=NULL && props->switchto>=0)
			switchto=props->switchto;
		
		frame_attach_client(frame, client, switchto);
	}else if(!CLIENT_HAS_FRAME(client)){
		hide_clientwin(cwin);
	}
	
	return TRUE;
}


WClientWin* manage_clientwin(Window win, int mflags)
{
	WScreen *scr;
	WClientWin *cwin;
	int state=NormalState;
	XWindowAttributes attr;
	WWinProp *props;
	bool b;
	/*XWMHints *hints;*/
	/*bool dock=FALSE;*/
	
	again:
	/* catch UnmapNotify and DestroyNotify */
	XSelectInput(wglobal.dpy, win, StructureNotifyMask);
	
	if(!XGetWindowAttributes(wglobal.dpy, win, &attr)){
		warn("Window disappeared");
		goto fail2;
	}

	scr=FIND_WINDOW_T(attr.root, WScreen);

	if(scr==NULL)
		return NULL;
	
#if 0
	hints=XGetWMHints(wglobal.dpy, win);

	if(hints!=NULL && hints->flags&StateHint)
		state=hints->initial_state;
	
	if(!dock && state==WithdrawnState){
		if(hints->flags&IconWindowHint && hints->icon_window!=None){
			/* The dockapp might be displaying its "main" window if no
			 * wm that understands dockapps has been managing it.
			 */
			if(mflags&MANAGE_INITIAL)
				XUnmapWindow(wglobal.dpy, win);
			
			XSelectInput(wglobal.dpy, win, 0);
			
			win=hints->icon_window;
			
			/* Is the icon window already being managed? */
			cwin=find_clientwin(win);
			if(cwin!=NULL){
				if(WTHING_IS(cwin, WDockwin))
					return cwin;
				unmanage_clientwin(cwin);
			}
		}
		dock=TRUE;
		goto again;
	}
	
	if(hints!=NULL)
		XFree((void*)hints);
#endif
	
	/* Get the actual state if any */
	get_win_state(win, &state);
	
#if 0
	if(!dock && (attr.override_redirect ||
				 (mflags&MANAGE_INITIAL && attr.map_state!=IsViewable)))
		goto fail2;
#else
	if(attr.override_redirect || (mflags&MANAGE_INITIAL &&
								  attr.map_state!=IsViewable))
		goto fail2;
#endif
	
	if(state!=NormalState && state!=IconicState)
		state=NormalState;
	
	/* Get winprops */
	props=find_winprop_win(win);
	
	/* Allocate and initialize */
	cwin=create_clientwin(scr, win, 0, &attr, props);
	
	if(cwin==NULL)
		goto fail2;
	
#if 0
	if(dock){
		winprop=find_winprop_win(origwin);	
		if(winprop!=NULL)
			cwin->dockpos=winprop->dockpos;
		
		if(!add_dockwin(cwin))
			goto failure;
	}else
#endif
	{
		CALL_ALT_B_ARG(b, add_clientwin, (cwin, state, props, &attr));
		if(!b)
			goto failure;
		
		/*if(!add_clientwin(cwin, state, props, &attr))
			goto failure;*/
	}
	
	/* Check that the window exists. The previous check selectinput do not
	 * seem to catch all cases of window destroyal.
	 */
	XSync(wglobal.dpy, False);
	if(XGetWindowAttributes(wglobal.dpy, win, &attr))
		return cwin;
	
	warn("Window disappeared");
	
	destroy_clientwin(cwin);
	return NULL;

failure:
	unmap_clientwin(cwin);
	return NULL;

fail2:
	XSelectInput(wglobal.dpy, win, 0);
	return NULL;
}


/*}}}*/


/*{{{ Unmanage/destroy */


static void get_clientwin_rootpos(WClientWin *cwin, int *xret, int *yret)
{
	*xret=0;
	*yret=0;
}


void deinit_clientwin(WClientWin *cwin)
{
	int x, y;
	XWindowAttributes attr;
	
	if(cwin->win!=None){
		XSelectInput(wglobal.dpy, cwin->win, 0);

		get_clientwin_rootpos(cwin, &x, &y);
		/*XReparentWindow(wglobal.dpy, cwin->win, SCREEN->root.win, x, y);*/
		if(XGetWindowAttributes(wglobal.dpy, cwin->win, &attr))
			XReparentWindow(wglobal.dpy, cwin->win, attr.root, x, y);
		
		if(cwin->orig_bw!=0)
			configure_cwin_bw(cwin->win, cwin->orig_bw);

		XRemoveFromSaveSet(wglobal.dpy, cwin->win);
		XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
		
		if(wglobal.opmode==OPMODE_DEINIT)
			XMapWindow(wglobal.dpy, cwin->win);
		else
			XDeleteProperty(wglobal.dpy, cwin->win, wglobal.atom_frame_id);
	}

	if(cwin->name!=NULL)
		XFree((void*)cwin->name);
}


/* Used when the the window is not to be managed anymore, but should
 * be mapped (deinit)
 */
/*void unmanage_clientwin(WClientWin *cwin)
{
	do_unmanage_clientwin(cwin, UNMANAGE);
}
*/

/* Used when the window was unmapped */
void unmap_clientwin(WClientWin *cwin)
{
	destroy_thing((WThing*)cwin);
}


/* Used when the window was deastroyed */
void destroy_clientwin(WClientWin *cwin)
{
	XDeleteContext(wglobal.dpy, cwin->win, wglobal.win_context);
	cwin->win=None;
	destroy_thing((WThing*)cwin);
}


/*}}}*/


/*{{{ Kill/close */


void kill_clientwin(WClientWin *cwin)
{
	XKillClient(wglobal.dpy, cwin->win);
}


void close_clientwin(WClientWin *cwin)
{
	if(cwin->flags&CWIN_P_WM_DELETE)
		send_clientmsg(cwin->win, wglobal.atom_wm_delete);
}

/*}}}*/


/*{{{ State (hide/show) */


static void set_clientwin_state(WClientWin *cwin, int state)
{
	if(cwin->state!=state){
		cwin->state=state;
		set_win_state(cwin->win, state);
	}
}


void hide_clientwin(WClientWin *cwin)
{
	if(cwin==NULL)
		return;

	if(cwin->flags&CWIN_KLUDGE_ACROBATIC){
		XMoveWindow(wglobal.dpy, cwin->win, -2*cwin->geom.w, -2*cwin->geom.h);
		return;
	}
			
	set_clientwin_state(cwin, IconicState);
	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
	XUnmapWindow(wglobal.dpy, cwin->win);
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
}


void show_clientwin(WClientWin *cwin)
{
	XWindowChanges wc;

	if(cwin==NULL)
		return;

	/* The main window must be lowered for xprop/xwininfo et all
	 * to work.
	 */
	if(cwin->transient_for==None){
		XLowerWindow(wglobal.dpy, cwin->win);
	}else{
		XRaiseWindow(wglobal.dpy, cwin->win);
		/*wc.stack_mode=Above;
		wc.sibling=cwin->transient_for;
		XConfigureWindow(wglobal.dpy, cwin->win, CWStackMode|CWSibling,
						 &wc);*/
	}
	
	if(cwin->flags&CWIN_KLUDGE_ACROBATIC){
		XMoveWindow(wglobal.dpy, cwin->win, cwin->geom.x, cwin->geom.y);
		if(cwin->state==NormalState)
			return;
	}

	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~(StructureNotifyMask|EnterWindowMask));
	XMapWindow(wglobal.dpy, cwin->win);
	/*if(cwin->state==WithdrawnState)
		XResizeWindow(wglobal.dpy, cwin->win, cwin->geom.w, cwin->geom.h);*/
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
	set_clientwin_state(cwin, NormalState);
}


void focus_clientwin(WClientWin *cwin)
{
	set_input_focus(cwin->win);
	
	if(cwin->flags&CWIN_P_WM_TAKE_FOCUS)
		send_clientmsg(cwin->win, wglobal.atom_wm_take_focus);
	
}

void iconify_clientwin(WClientWin *cwin)
{
#ifndef CF_IGNORE_ICONIFY_REQUEST
	WFrame *frame=FIND_PARENT(cwin, WFrame);

	if(frame!=NULL)
		frame_switch_next(frame);
#endif
}

/*}}}*/


/*{{{ Misc */


WClientWin *find_clientwin(Window win)
{
	return FIND_WINDOW_T(win, WClientWin);
}


static void send_clientmsg(Window win, Atom a)
{
	XClientMessageEvent ev;
	
	ev.type=ClientMessage;
	ev.window=win;
	ev.message_type=wglobal.atom_wm_protocols;
	ev.format=32;
	ev.data.l[0]=a;
	ev.data.l[1]=CurrentTime;
	
	XSendEvent(wglobal.dpy, win, False, 0L, (XEvent*)&ev);
}


void set_clientwin_name(WClientWin *cwin, char *p)
{
	WClient *client=FIND_PARENT(cwin, WClient);
	
	if(p==NULL)
		return;
	
	stripws(p);
	
	if(client!=NULL){
		if(FIRST_THING(client, WClientWin)!=cwin)
			client=NULL;
	}
	
	if(client!=NULL)
		client_unuse_label(client);
	
	if(cwin->name!=NULL)
		XFree(cwin->name);
	
	cwin->name=p;
	
	if(client!=NULL)
		client_use_label(client);
}


/*}}}*/


/*{{{ Resize/reparent/reconf */


void reconf_clientwin(WClientWin *cwin, int rootx, int rooty)
{
	XEvent ce;
	Window win;
	int bdif;
	
	if(cwin==NULL)
		return;
	
	win=cwin->win;

#if 1
	/* Frame has a border (one drawn by X) set */
	bdif=-cwin->orig_bw+GRDATA_OF(cwin)->spacing;
#else
	bdif=-cwin->orig_bw;
#endif
	
	ce.xconfigure.type=ConfigureNotify;
	ce.xconfigure.event=win;
	ce.xconfigure.window=win;
	ce.xconfigure.x=rootx+bdif;
	ce.xconfigure.y=rooty+bdif;
	ce.xconfigure.width=cwin->geom.w;
	ce.xconfigure.height=cwin->geom.h;
	ce.xconfigure.border_width=cwin->orig_bw;
	ce.xconfigure.above=None;
	ce.xconfigure.override_redirect=False;

	XSelectInput(wglobal.dpy, win, cwin->event_mask&~StructureNotifyMask);
	XSendEvent(wglobal.dpy, win, False, StructureNotifyMask, &ce);
	XSelectInput(wglobal.dpy, win, cwin->event_mask);
}


void sendconfig_clientwin(WClientWin *cwin)
{
	WFrame *frame=FIND_PARENT(cwin, WFrame);
	
	if(frame!=NULL){
		reconf_clientwin(cwin, FRAME_X(frame)+cwin->geom.x,
						 FRAME_Y(frame)+cwin->geom.y);
	}
}


void reparent_clientwin(WClientWin *cwin, Window win, int x, int y)
{
	XSelectInput(wglobal.dpy, cwin->win,
				 cwin->event_mask&~StructureNotifyMask);
	XReparentWindow(wglobal.dpy, cwin->win, win, x, y);
	XSelectInput(wglobal.dpy, cwin->win, cwin->event_mask);
}


static WRectangle cwin_geom(WClientWin *cwin, bool bottom,
							WRectangle geom, int w, int h)
{
	WRectangle r;

	r.w=(w<geom.w ? w : geom.w);
	r.h=(h<geom.h ? h : geom.h);
	
	correct_size(&(r.w), &(r.h), &(cwin->size_hints), FALSE);
	
	if(bottom && r.h>cwin->geom.h)
		r.h=cwin->geom.h;
	
	r.x=geom.x+geom.w/2-r.w/2;
	
	if(bottom)
		r.y=geom.y+geom.h-r.h;
	else
		r.y=geom.y+geom.h/2-r.h/2;
	
	return r;
}


static WRectangle cwin_frame_geom(WClientWin *cwin, WFrame *frame)
{
	WRectangle geom;
	frame_client_geom(frame, &geom);
	return cwin_geom(cwin, cwin->transient_for!=None, geom, geom.w, geom.h);
}


void fit_clientwin_frame(WClientWin *cwin, WFrame *frame)
{
	WRectangle geom=cwin_frame_geom(cwin, frame);

	XMoveResizeWindow(wglobal.dpy, cwin->win, geom.x, geom.y, geom.w, geom.h);
	cwin->geom=geom;
	reconf_clientwin(cwin, FRAME_X(frame)+geom.x, FRAME_Y(frame)+geom.y);
}


void reparent_fit_clientwin_frame(WClientWin *cwin, WFrame *frame)
{
	WRectangle geom=cwin_frame_geom(cwin, frame);

	XResizeWindow(wglobal.dpy, cwin->win, geom.w, geom.h);
	reparent_clientwin(cwin, FRAME_WIN(frame), geom.x, geom.y);
	cwin->geom=geom;
	reconf_clientwin(cwin, FRAME_X(frame)+geom.x, FRAME_Y(frame)+geom.y);
}


void reconf_clientwin_frame(WClientWin *cwin, WFrame *frame)
{
	WRectangle geom=cwin_frame_geom(cwin, frame);
	reconf_clientwin(cwin, FRAME_X(frame)+geom.x, FRAME_Y(frame)+geom.y);
}


void refit(WClientWin *cwin, int w, int h)
{
	WFrame *frame;
	WRectangle geom;
	
	frame=FIND_PARENT(cwin, WFrame);
	
	if(frame==NULL)
		return;
	
	frame_client_geom(frame, &geom);
	
#ifndef CF_KLUDGE_RESPECT_SHRINK
	w=geom.w;
	h=geom.h;
#endif
	
	geom=cwin_geom(cwin, cwin->transient_for!=None, geom, w, h);
	XMoveResizeWindow(wglobal.dpy, cwin->win, geom.x, geom.y, geom.w, geom.h);
	cwin->geom=geom;
	reconf_clientwin(cwin, FRAME_X(frame)+geom.x, FRAME_Y(frame)+geom.y);
}


/*}}}*/

