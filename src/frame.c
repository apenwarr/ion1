/*
 * ion/frame.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "frame.h"
#include "frameid.h"
#include "clientwin.h"
#include "screen.h"
#include "property.h"
#include "focus.h"
#include "event.h"
#include "binding.h"
#include "global.h"
#include "client.h"
#include "workspace.h"
#include "draw.h"
#include "thingp.h"
#include "split.h"
#include "input.h"
#include "query.h"
#include "splitmisc.h"
#include "shortcut.h"


#define FRAME_MIN_H(SCR) \
	((SCR)->grdata.client_off.y-(SCR)->grdata.client_off.h\
	 +CF_MIN_HEIGHT*(SCR)->h_unit)
#define FRAME_MIN_W(SCR) \
	((SCR)->grdata.client_off.x-(SCR)->grdata.client_off.w\
	 +CF_MIN_WIDTH*(SCR)->w_unit)

#define BAR_X(FRAME, GRDATA) ((GRDATA)->bar_off.x)
#define BAR_Y(FRAME, GRDATA) ((GRDATA)->bar_off.y)
/*#define BAR_Y(FRAME, GRDATA) ((FRAME)->win.geom.h+(GRDATA)->bar_off.y) */
#define BAR_W(FRAME, GRDATA) ((FRAME)->win.geom.w+(GRDATA)->bar_off.w)
#define BAR_H(FRAME, GRDATA) ((GRDATA)->bar_h)

#define FRAME_TO_CLIENT_W(W, GRDATA) ((W)+(GRDATA)->client_off.w)
#define FRAME_TO_CLIENT_H(H, GRDATA) ((H)+(GRDATA)->client_off.h)
#define CLIENT_TO_FRAME_W(W, GRDATA) ((W)-(GRDATA)->client_off.w)
#define CLIENT_TO_FRAME_H(H, GRDATA) ((H)-(GRDATA)->client_off.h)

#define CLIENT_X(FRAME, GRDATA) ((GRDATA)->client_off.x)
#define CLIENT_Y(FRAME, GRDATA) ((GRDATA)->client_off.y)
#define CLIENT_W(FRAME, GRDATA) FRAME_TO_CLIENT_W(FRAME_W(FRAME), GRDATA)
#define CLIENT_H(FRAME, GRDATA) FRAME_TO_CLIENT_H(FRAME_H(FRAME), GRDATA)


WThingFuntab frame_funtab={deinit_frame, frame_remove_child};
IMPLOBJ(WFrame, WWindow, &frame_funtab)


static void move_clients(WFrame *dest, WFrame *src);


/*{{{ Destroy/create frame */


static bool init_frame(WFrame *frame, WScreen *scr,
					   WRectangle geom, int id, int flags)
{
	Window win;
	XSetWindowAttributes attr;
	WGRData *grdata=&(scr->grdata);
	int sp=grdata->spacing;
	
	frame->flags=flags;
	frame->frame_id=(id==0 ? new_frame_id() : use_frame_id(id));
	frame->client_count=0;
	frame->current_client=NULL;
	frame->current_input=NULL;
	
#if 1
	attr.background_pixel=grdata->act_frame_colors.bg;
	attr.border_pixel=grdata->bgcolor;
	win=XCreateWindow(wglobal.dpy, scr->root.win,
					  geom.x, geom.y, geom.w-sp*2, geom.h-sp*2,
					  sp, CopyFromParent, InputOutput,
					  CopyFromParent, CWBackPixel|CWBorderPixel, &attr);
#else
	attr.background_pixmap=ParentRelative;
	win=XCreateWindow(wglobal.dpy, scrroot.win,
					  geom.x, geom.y, geom.w, geom.h,
					  0, CopyFromParent, InputOutput,
					  CopyFromParent, CWBackPixmap, &attr);
#endif
	
	if(!init_window((WWindow*)frame, win, geom)){
		XDestroyWindow(wglobal.dpy, win);
		return FALSE;
	}

	frame->win.min_w=FRAME_MIN_W(scr);
	frame->win.min_h=FRAME_MIN_H(scr);
	frame->tab_w=BAR_W(frame, grdata);

	XSelectInput(wglobal.dpy, win, FRAME_MASK);
	
	frame->win.bindmap=&(wglobal.main_bindmap);
	grab_bindings(&(wglobal.main_bindmap), win);
	
	return TRUE;
}


WFrame *create_frame(WScreen *scr, WRectangle geom, int id, int flags)
{
	CREATETHING_IMPL(WFrame, frame, scr, (p, scr, geom, id, flags));
}


void deinit_frame(WFrame *frame)
{
	deinit_window((WWindow*)frame);
	if (shortcut_is_valid(frame->shortcut))
		shortcut_remove(frame->shortcut);
	free_frame_id(frame->frame_id);
}


void destroy_frame(WFrame *frame)
{
	WWorkspace *ws;
	WScreen *scr;
	WFrame *other;

	other=find_sister_frame(frame);
	
	if(frame->client_count!=0){
		if(other==NULL){
			fwarn(frame,
				  errmsg("Last frame on workspace and not empty"
						 " - refusing to destroy."));
			return;
		}
		move_clients(other, frame);
		
		assert(frame->client_count==0);
	}
	
	ws=FIND_PARENT(frame, WWorkspace);

	if(ws!=NULL){
		scr=FIND_PARENT(ws, WScreen);
		assert(scr!=NULL);
		
		if(scr->workspace_count<=1 && ws->splitree==(WObj*)frame){
			fwarn(frame,
				  errmsg("Cannot destroy only frame on only workspace."));
			return;
		}
	}
				 
	destroy_thing((WThing*)frame);
	
	if(other!=NULL)
		set_focus((WThing*)other);
}


void closedestroy(WFrame *frame)
{
	if(frame->current_client!=NULL)
		close_client(frame->current_client);
	else
		destroy_frame(frame);
}


void frame_remove_child(WFrame *frame, WThing *thing)
{
	if(WTHING_IS(thing, WClient)){
		frame_detach_client(frame, (WClient*)thing);
	}else if((WThing*)(frame->current_input)==thing){
		frame->current_input=NULL;
		if(IS_ACTIVE_FRAME(frame))
			focus_frame(frame);
	}
}


/*}}}*/


/*{{{ Client switching */


static void do_frame_switch_client(WFrame *frame, WClient *client)
{
	show_client(client);
	
	if(frame->current_client!=NULL)
		hide_client(frame->current_client);
	
	if(frame->current_input==NULL){
		if(IS_ACTIVE_FRAME(frame)){
			set_previous((WThing*)client);
			set_focus((WThing*)client);
		}
	}else{
		XRaiseWindow(wglobal.dpy, frame->current_input->win.win);
	}

	frame->current_client=client;
}


void frame_switch_client(WFrame *frame, WClient *client)
{
	if(client!=frame->current_client){
		do_frame_switch_client(frame, client);
		draw_frame_bar(frame, FALSE);
	}
}


void frame_switch_nth(WFrame *frame, int num)
{
	WClient *client=NTH_THING(frame, num, WClient);

	if(client!=NULL)
		frame_switch_client(frame, client);
}


void frame_switch_next(WFrame *frame)
{
	WClient *client=NULL;
	
	if(frame->current_client!=NULL)
		client=NEXT_THING(frame->current_client, WClient);
	if(client==NULL)
		client=FIRST_THING(frame, WClient);
	
	if(client==NULL)
		return;

	frame_switch_client(frame, client);
}


void frame_switch_prev(WFrame *frame)
{
	WClient *client=NULL;
	
	if(frame->current_client!=NULL)
		client=PREV_THING(frame->current_client, WClient);
	if(client==NULL)
		client=LAST_THING(frame, WClient);

	if(client==NULL)
		return;
	
	frame_switch_client(frame, client);
}


/*}}}*/


/*{{{ Focus */


void activate_frame(WFrame *frame)
{
	draw_frame(frame, FALSE);
}


void deactivate_frame(WFrame *frame)
{
	draw_frame(frame, FALSE);
}


void focus_frame(WFrame *frame)
{
	if(frame->current_input!=NULL)
		focus_window((WWindow*)frame->current_input);
	else if(frame->current_client!=NULL)
		focus_client(frame->current_client);
	else
		focus_window((WWindow*)frame);
}


/*}}}*/


/*{{{ Attach/detach */


static bool do_frame_detach_client(WFrame *frame, WClient *client,
								   bool reparent, bool destroy);


bool frame_attach_client(WFrame *frame, WClient *client, bool switchto)
{
	WFrame *oframe=NULL;
	WClientWin *cwin;
	
	if(CLIENT_HAS_FRAME(client)){
		if(CLIENT_FRAME(client)==frame)
			return TRUE;
		do_frame_detach_client(CLIENT_FRAME(client), client, FALSE, FALSE);
	}

	reparent_fit_client_frame(client, frame);
	
	/* Only set the ID for the main window */
	set_client_frame_id(client, frame->frame_id);
	
#if 0
	link_thing((WThing*)frame, (WThing*)client);
#else
	if(frame->current_client!=NULL && wglobal.opmode!=OPMODE_INIT)
		link_thing_after((WThing*)(frame->current_client), (WThing*)client);
	else
		link_thing((WThing*)frame, (WThing*)client);
#endif
	
	if(frame->client_count==0)
		switchto=TRUE;
	
	frame->client_count++;

	if(switchto)
		do_frame_switch_client(frame, client);
	else
		hide_client(client);    /* Perhaps notify_hide_client -
							  		unmap is unnecessary */
	frame_recalc_bar(frame);
	
	return TRUE;
}


static void move_clients(WFrame *dest, WFrame *src)
{
	WClient *client, *next;

	for(client=FIRST_THING(src, WClient); client!=NULL; client=next){
		next=NEXT_THING(client, WClient);
		frame_attach_client(dest, client, FALSE);
	}
}

	
/* */


static bool do_frame_detach_client(WFrame *frame, WClient *client,
								   bool reparent, bool destroy)
{
	WClient *next=NULL;

	if(frame->current_client==client){
#if 0
		next=NEXT_THING(client, WClient);
		if(next==NULL)
			next=PREV_THING(client, WClient);
		frame->current_client=NULL;
#else
		next=PREV_THING(client, WClient);
		if(next==NULL)
			next=NEXT_THING(client, WClient);
		frame->current_client=NULL;
#endif		
	}
	
	unlink_thing((WThing*)client);
	frame->client_count--;
	
	if(reparent)
		reparent_client(client, SCREEN_OF(frame)->root.win, 0, 0);
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		if(next!=NULL)
			do_frame_switch_client(frame, next);
	
		frame_recalc_bar(frame);
	}

	return TRUE;
}


void frame_detach_client(WFrame *frame, WClient* client)
{
	do_frame_detach_client(frame, client, FALSE, FALSE);
}


/* */


void frame_add_clientwin(WFrame *frame, WClient *client, WClientWin *cwin)
{
	reparent_fit_clientwin_frame(cwin, frame);

	if(cwin==FIRST_THING(client, WClientWin)){
		set_integer_property(cwin->win, wglobal.atom_frame_id,
							 frame->frame_id);
	}
	
	if(client==frame->current_client){
		show_clientwin(cwin);
		if(IS_ACTIVE_FRAME(frame))
			focus_clientwin(cwin);
	}else{
		hide_clientwin(cwin);
	}
}


/*}}}*/


/*{{{ Fit/reconf */


void frame_fit_input(WFrame *frame)
{
	WRectangle geom;
	
	if(frame->current_input==NULL)
		return;

	frame_client_geom(frame, &geom);
	
	input_resize(frame->current_input, geom);
}


static void frame_fit_clients(WFrame *frame)
{
	WClient *client;
	
	FOR_ALL_TYPED(frame, client, WClient){
		fit_client_frame(client, frame);
	}
}


static void frame_reconf_clients(WFrame *frame)
{
	WClient *client;
	
	FOR_ALL_TYPED(frame, client, WClient){
		reconf_client_frame(client, frame);
	}
}


void frame_recalc_bar(WFrame *frame)
{
	WScreen *scr=SCREEN_OF(frame);
	int bar_w=BAR_W(frame, &(scr->grdata));
	int tab_w;
	int textw;
	WClient *client;
	
	if(shortcut_is_valid(frame->shortcut))
		bar_w-=FRAME_SHORTCUT_W+scr->grdata.spacing;
	
	if(frame->client_count==0){
		frame->tab_w=bar_w;
		draw_frame_bar(frame, TRUE);
		return;
	}
		
	tab_w=(bar_w-(frame->client_count-1)*scr->grdata.spacing)/frame->client_count;
	frame->tab_w=tab_w;
	
	textw=BORDER_IW(&(scr->grdata.tab_border), tab_w);
	
	FOR_ALL_TYPED(frame, client, WClient){
		client_make_label(client, textw);
	}

	draw_frame_bar(frame, TRUE);
}


/*}}}*/


/*{{{ Move/resize */


void set_frame_geom(WFrame *frame, WRectangle geom)
{
	WScreen *scr=SCREEN_OF(frame);
	int sp=scr->grdata.spacing;
	bool wchg=(FRAME_W(frame)!=geom.w);
	
	frame->win.geom=geom;

#if 1
	XMoveResizeWindow(wglobal.dpy, FRAME_WIN(frame),
					  FRAME_X(frame), FRAME_Y(frame),
					  FRAME_W(frame)-sp*2, FRAME_H(frame)-sp*2);
#else
	XMoveResizeWindow(wglobal.dpy, FRAME_WIN(frame),
					  FRAME_X(frame), FRAME_Y(frame),
					  FRAME_W(frame), FRAME_H(frame));
#endif
	frame_fit_clients(frame);
	frame_fit_input(frame);
	
	if(wchg)
		frame_recalc_bar(frame);
}


void set_frame_pos(WFrame *frame, int x, int y)
{
	frame->win.geom.x=x;
	frame->win.geom.y=y;

	XMoveWindow(wglobal.dpy, FRAME_WIN(frame), FRAME_X(frame), FRAME_Y(frame));

	frame_reconf_clients(frame);
}


/*}}}*/


/*{{{ Attach tagged clients */


static void do_attach_tagged(WFrame *frame)
{
	WClient *client;
	int tot=0, sc=0;

	for(client=wglobal.client_list;
		client!=NULL;
		client=client->g_client_next){
		
		if(!(client->flags&CLIENT_TAGGED))
			continue;
		tot++;
		
		client_toggle_tagged(client);

		if(!same_screen((WThing*)frame, (WThing*)client))
	   		continue;
		
		frame_attach_client(frame, client, FALSE);
		sc++;
	}
	
	if(tot!=sc){
		fwarn(frame, errmsg("Could only attach %d/%d clients"
							"(others not on same screen).", sc, tot));
	}
}


void frame_attach_tagged(WFrame *frame)
{
	do_attach_tagged(frame);
}


/*}}}*/


/*{{{ Split */


static WWindow* split_create_frame(WScreen *scr, WRectangle geom)
{
	return (WWindow*)create_frame(scr, geom, 0, 0);
}


void split_vert(WFrame *frame)
{
	WWindow *wwin;
	wwin=split_window((WWindow*)frame, VERTICAL, frame->win.min_h,
					  split_create_frame);
	if(wwin!=NULL)
		warp(wwin);
}


void split_horiz(WFrame *frame)
{
	WWindow *wwin;
	wwin=split_window((WWindow*)frame, HORIZONTAL, frame->win.min_w,
					  split_create_frame);
	if(wwin!=NULL)
		warp(wwin);
}


void split_top(WWorkspace *ws, char *str)
{
	WWindow *wwin;
	int dir, primn;
	
	if(str==NULL)
		return;
	
	if(!strcmp(str, "left")){
		primn=TOP_OR_LEFT;
		dir=HORIZONTAL;
	}else if(!strcmp(str, "right")){
		primn=BOTTOM_OR_RIGHT;
		dir=HORIZONTAL;
	}else if(!strcmp(str, "top")){
		primn=TOP_OR_LEFT;
		dir=VERTICAL;
	}else if(!strcmp(str, "bottom")){
		primn=BOTTOM_OR_RIGHT;
		dir=VERTICAL;
	}else{
		return;
	}
	
	wwin=split_toplevel(ws, dir, primn,
						FRAME_MIN_H(SCREEN_OF(ws)),
						split_create_frame);
	if(wwin!=NULL)
		warp(wwin);
}


/*}}}*/

/*{{{ Misc */


void frame_bar_geom(const WFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);
	
	geom->x=BAR_X(frame, grdata);
	geom->y=BAR_Y(frame, grdata);
	geom->w=BAR_W(frame, grdata);
	geom->h=BAR_H(frame, grdata);
}

void frame_client_geom(const WFrame *frame, WRectangle *geom)
{
	WGRData *grdata=GRDATA_OF(frame);

	geom->x=CLIENT_X(frame, grdata);
	geom->y=CLIENT_Y(frame, grdata);
	geom->w=CLIENT_W(frame, grdata);
	geom->h=CLIENT_H(frame, grdata);
}


/*}}}*/

