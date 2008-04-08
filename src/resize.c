/*
 * ion/resize.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "global.h"
#include "split.h"
#include "resize.h"
#include "signal.h"
#include "event.h"
#include "cursor.h"
#include "sizehint.h"
#include "draw.h"
#include "grab.h"


#define XOR_RESIZE (!wglobal.opaque_resize)

static bool resize_handler(WThing *thing, XEvent *ev);

static bool resize_mode=FALSE;
static int tmpsize;
static WResizeTmp rtmp;


/*{{{ Resize timer */


static void tmr_end_resize(WTimer *unused)
{
	WThing *holder;
	if(!resize_mode){
		return;
	}
	holder=grab_get_holder(resize_handler);
	assert(holder && WTHING_IS(holder, WWindow));
	
	end_resize((WWindow*)holder);
}

static WTimer resize_timer=INIT_TIMER(tmr_end_resize);


/*}}}*/


/*{{{ Rubberband and size display */


static void res_draw_rubberband(WScreen *scr, WRectangle geom)
{
	if(rtmp.startnode!=NULL){
		if(WOBJ_IS(rtmp.startnode, WWindow))
			geom=((WWindow*)rtmp.startnode)->geom;
		else
			geom=((WWsSplit*)rtmp.startnode)->geom;
	}
	
	if(rtmp.dir==HORIZONTAL){
		geom.x=rtmp.winpostmp;
		geom.w=rtmp.winsizetmp;
	}else{
		geom.y=rtmp.winpostmp;
		geom.h=rtmp.winsizetmp;
	}
	
	draw_rubberband(scr, geom, rtmp.dir==VERTICAL);
}


static void cwin_size(WScreen *scr, int *w, int *h, XSizeHints *sh)
{
	correct_size(w, h, sh, FALSE);
	
	if(sh->flags&PBaseSize){
		*w-=sh->base_width;
		*h-=sh->base_height;
	}
	
	if(sh->flags&PResizeInc){
		*w/=sh->width_inc;
		*h/=sh->height_inc;
	}else{
		*w/=scr->w_unit;
		*h/=scr->h_unit;
	}
}


static void res_draw_moveres(WScreen *scr, WWindow *wwin)
{
	int w, h, x, y;
	WClientWin *cwin;
	
	if(rtmp.dir==HORIZONTAL){
		x=rtmp.winpostmp;
		w=rtmp.winsizetmp;
		h=wwin->geom.h;
		y=wwin->geom.y;
	}else{
		x=wwin->geom.x;
		w=wwin->geom.w;
		y=rtmp.winpostmp;
		h=rtmp.winsizetmp;
	}

	if(WTHING_IS(wwin, WFrame)){
		w+=FRAME_CLIENT_WOFF(scr);
		h+=FRAME_CLIENT_HOFF(scr);
		if(((WFrame*)wwin)->current_client!=NULL &&
			(cwin=FIRST_THING(((WFrame*)wwin)->current_client,
							  WClientWin))!=NULL){
			cwin_size(scr, &w, &h, &(cwin->size_hints));
		}else{
			w/=scr->w_unit;
			h/=scr->h_unit;
		}
	}
	
	set_moveres_size(scr, w, h);
}


/*}}}*/


/*{{{ Resize/common */


static int do_calc_resize(int s, int unit, int off)
{
	s-=off;
	s=(s/unit)*unit;
	return s+off;
}


static int calc_resize(WScreen *scr, int s, int dir)
{
	if(dir==VERTICAL)
		return do_calc_resize(s, scr->h_unit, FRAME_CLIENT_HOFF(scr));
	else
		return do_calc_resize(s, scr->w_unit, FRAME_CLIENT_WOFF(scr));
}


void begin_resize(WWindow *wwin, int dir, int primn)
{
	WScreen *scr=SCREEN_OF(wwin);
	
	tmpsize=wwin_size(wwin, dir);
	resize_mode=TRUE;
	
	if(XOR_RESIZE)
		XGrabServer(wglobal.dpy);
	
	XMoveWindow(wglobal.dpy, scr->grdata.moveres_win,
				wwin->geom.x+(wwin->geom.w-scr->grdata.moveres_geom.w)/2,
				wwin->geom.y+(wwin->geom.h-scr->grdata.moveres_geom.h)/2);
	XMapRaised(wglobal.dpy, scr->grdata.moveres_win);
	
	calcresize_window(wwin, dir, primn, tmpsize, &rtmp);

	res_draw_moveres(scr, wwin);
	
	if(XOR_RESIZE)
		res_draw_rubberband(scr, wwin->geom);
}


void resize(WWindow *wwin, int delta, int primn, bool settmp)
{
	WScreen *scr=SCREEN_OF(wwin);
	int s, old;
	int dir=rtmp.dir;
	
	if(XOR_RESIZE)
		res_draw_rubberband(scr, wwin->geom);
	
	tmpsize+=delta;
	s=calc_resize(SCREEN_OF(wwin), tmpsize, dir);
	old=rtmp.winsizetmp;
	
	calcresize_window(wwin, dir, primn, s, &rtmp);
	
	if(settmp)
		tmpsize=rtmp.winsizetmp;

	res_draw_moveres(scr, wwin);
	
	if(XOR_RESIZE)
		res_draw_rubberband(scr, wwin->geom);
	else
		resize_tmp(&rtmp);
}


void end_resize(WWindow *wwin)
{
	WScreen *scr=SCREEN_OF(wwin);
	
	if(!resize_mode)
		return;
	
	resize_mode=FALSE;
	
	reset_timer(&resize_timer);
	
	if(XOR_RESIZE){
		res_draw_rubberband(scr, wwin->geom);
		resize_tmp(&rtmp);
		XUngrabServer(wglobal.dpy);
	}
	XUnmapWindow(wglobal.dpy, scr->grdata.moveres_win);
}


void cancel_resize(WWindow *wwin)
{
	WScreen *scr=SCREEN_OF(wwin);
	
	if(!resize_mode)
		return;
	
	resize_mode=FALSE;

	reset_timer(&resize_timer);
	
	if(XOR_RESIZE){
		res_draw_rubberband(scr, wwin->geom);
		XUngrabServer(wglobal.dpy);
	}
	XUnmapWindow(wglobal.dpy, scr->grdata.moveres_win);
}


/*}}}*/


/*{{{ Keyboard resize */

static bool resize_handler(WThing *thing, XEvent *xev)
{
	XKeyEvent *ev=&xev->xkey;
	WScreen *scr;
	WBinding *binding=NULL;
	WBindmap **bindptr;

	if(!resize_mode){
		return TRUE;
	}

	if(ev->type==KeyRelease)
		return FALSE;

	assert(thing && WTHING_IS(thing, WWindow));
	binding=lookup_binding(&wglobal.moveres_bindmap, ACT_KEYPRESS, ev->state, ev->keycode);

	if(!binding)
		return FALSE;

	if(binding){
		/* Get the screen now for waitrel grab - the thing might
		 * have been destroyed when call_binding returns.
		 */
		scr=SCREEN_OF(thing);
		call_binding(binding, thing);
	}

	return !resize_mode;
}

static void begin_keyresize(WWindow *wwin, int dir)
{
	if(resize_mode)
		return;
	
	begin_resize(wwin, dir, ANY);
		
	grab_establish((WThing*)wwin, resize_handler, FocusChangeMask);
	change_grab_cursor(CURSOR_RESIZE);
}


static int get_unit(WWindow *wwin)
{
	WScreen *scr=SCREEN_OF(wwin);
	return (rtmp.dir==VERTICAL ? scr->h_unit : scr->w_unit);
}


void grow(WWindow *wwin)
{
	if(!resize_mode)
		resize_vert(wwin);
	resize(wwin, get_unit(wwin), ANY, TRUE);
	set_timer(&resize_timer, wglobal.resize_delay);
}


void shrink(WWindow *wwin)
{
	if(!resize_mode)
		resize_vert(wwin);
	resize(wwin, -get_unit(wwin), ANY, TRUE);
	set_timer(&resize_timer, wglobal.resize_delay);
}


void resize_vert(WWindow *wwin)
{
	begin_keyresize(wwin, VERTICAL);
}


void resize_horiz(WWindow *wwin)
{
	begin_keyresize(wwin, HORIZONTAL);
}


/*}}}*/


/*{{{ Maximize */


static void do_maximize(WWindow *wwin, int dir, int flag,
						int *saved_size, int max_size)
{
	WResizeTmp rtmp;
	
	if(wwin->flags&flag){
		calcresize_window(wwin, dir, ANY, *saved_size, &rtmp);
		resize_tmp(&rtmp);
		wwin->flags&=~flag;
	}else{
		*saved_size=wwin_size(wwin, dir);
		calcresize_window(wwin, dir, ANY, max_size, &rtmp);
		resize_tmp(&rtmp);
		wwin->flags|=flag;
	}
}

						
void maximize_vert(WWindow *wwin)
{
	WScreen *scr=SCREEN_OF(wwin);
	
	if(scr!=NULL){
		do_maximize(wwin, VERTICAL, WWINDOW_HFORCED,
					&(wwin->saved_h), scr->root.geom.h);
	}
}


void maximize_horiz(WWindow *wwin)
{
	WScreen *scr=SCREEN_OF(wwin);
	
	if(scr!=NULL){
		do_maximize(wwin, HORIZONTAL, WWINDOW_WFORCED,
					&(wwin->saved_w), scr->root.geom.w);
	}
}


/*}}}*/


/*{{{ Set size */


void set_height(WWindow *wwin, uint h)
{
	WResizeTmp rtmp;
	calcresize_window(wwin, VERTICAL, ANY, h, &rtmp);
	resize_tmp(&rtmp);
}


void set_width(WWindow *wwin, uint w)
{
	WResizeTmp rtmp;
	calcresize_window(wwin, HORIZONTAL, ANY, w, &rtmp);
	resize_tmp(&rtmp);
}

void set_heightq(WWindow *wwin, double q)
{
	WScreen *scr=SCREEN_OF(wwin);
	set_height(wwin, q*(double)scr->root.geom.h);
}

void set_widthq(WWindow *wwin, double q)
{
	WScreen *scr=SCREEN_OF(wwin);
	set_width(wwin, q*(double)scr->root.geom.w);
}

/*}}}*/

