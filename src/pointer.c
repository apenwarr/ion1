/*
 * ion/pointer.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "pointer.h"
#include "frame.h"
#include "cursor.h"
#include "event.h"
#include "binding.h"
#include "global.h"
#include "client.h"
#include "draw.h"
#include "wedln.h"
#include "split.h"
#include "resize.h"
#include "focus.h"
#include "grab.h"


/*{{{ Variables */


static uint p_button=0, p_state=0;
static WBindmap *p_bindmap=NULL;
static int p_x=-1, p_y=-1;
static int p_orig_x=-1, p_orig_y=-1;
static bool p_motion=FALSE;
static int p_clickcnt=0;
static Time p_time=0;
static int p_tab_x, p_tab_y;
static bool p_motiontmp_dirty;
static WBinding *p_motiontmp=NULL;
static WThing *p_dbltmp=NULL;
static int p_dir, p_primn;
static WScreen *p_screen=NULL;


/*}}}*/


/*{{{ Misc. */


/*void get_pointer_rootpos(Window rootwin, int *xret, int *yret)
{
	Window root, win;
	int x, y, wx, wy;
	uint mask;
	
	XQueryPointer(wglobal.dpy, rootwin, &root, &win,
				  xret, yret, &wx, &wy, &mask);
}*/


static bool time_in_treshold(Time time)
{
	Time t;
	
	if(time<p_time)
		t=p_time-time;
	else
		t=time-p_time;
	
	return t<wglobal.dblclick_delay;
}


static bool motion_in_treshold(int x, int y)
{
	return (x>p_x-CF_DRAG_TRESHOLD && x<p_x+CF_DRAG_TRESHOLD &&
			y>p_y-CF_DRAG_TRESHOLD && y<p_y+CF_DRAG_TRESHOLD);
}


bool find_window_at(Window rootwin, int x, int y, Window *childret)
{
	int dstx, dsty;
	
	if(!XTranslateCoordinates(wglobal.dpy, rootwin, rootwin,
							  x, y, &dstx, &dsty, childret))
		return FALSE;
	
	if(*childret==None)
		return FALSE;
	
	return TRUE;
}


static bool inrect(WRectangle g, int x, int y)
{
	return (x>=g.x && x<g.x+g.w && y>=g.y && y<g.y+g.h);
}


/*}}}*/


/*{{{ Call handlers */


void callhnd_button_void(WThing *thing, WFunction *func,
						 int n, const Token *args)
{
}


void callhnd_drag_void(WThing *thing, WFunction *func,
					   int n, const Token *args)
{
}


static void call_button(WBinding *binding, XButtonEvent *ev)
{
	WButtonHandler *fn;
	WThing *thing;

	if(binding==NULL)
		return;

	if(binding->func->callhnd==callhnd_button_void){
		fn=(WButtonHandler*)binding->func->fn;
	}else if(binding->func->callhnd==callhnd_drag_void){
		fn=((WDragHandler*)binding->func->fn)->release;
	}else{
		call_binding(binding, wglobal.grab_holder);
		return;
	}
	
	thing=find_parent(wglobal.grab_holder, binding->func->objdescr);
	if(thing!=NULL)
		fn(thing, ev);
}


static void call_motion(WBinding *binding, XMotionEvent *ev, int dx, int dy)
{
	WMotionHandler *fn;
	WThing *thing;
	
	if(binding==NULL)
		return;

	if(binding->func->callhnd!=callhnd_drag_void){
		call_binding(binding, wglobal.grab_holder);
		return;
	}
	
	fn=((WDragHandler*)binding->func->fn)->motion;
	thing=find_parent(wglobal.grab_holder, binding->func->objdescr);
	if(thing!=NULL)
		fn(thing, ev, dx, dy);
}


/*}}}*/


/*{{{ Frame press */


static int get_quadrant(int x, int y, int w, int h)
{
	int a, b;

	a=2*(y>(h-x*h/w));	/* 00/
						 * 0/2 
						 * /22 */
	
	b=(y>(x*h/w)); 	 	/* \00
				   	 	 * 1\0
					 	 * 11\ */
	
	return a+b;
}
						
static WBindmap *frame_press(WFrame *frame, XButtonEvent *ev, WThing **thing)
{
	WScreen *scr=SCREEN_OF(frame);
	int cw=CF_CORNER_SIZE, ch=CF_CORNER_SIZE;
	int actx=-1, tabnum=-1;
	bool ret;
	WRectangle g;
	int tw, x;
	int qd, off;
	WClient *client;

	qd=get_quadrant(ev->x, ev->y, FRAME_W(frame), FRAME_H(frame));
	
	if(qd==0 || qd==3)
		p_dir=VERTICAL;
	else
		p_dir=HORIZONTAL;
	
	if(qd==0 || qd==1)
		p_primn=TOP_OR_LEFT;
	else
		p_primn=BOTTOM_OR_RIGHT;

	frame_bar_geom(frame, &g);
	
	if(inrect(g, ev->x, ev->y)){
		off=((!scr->grdata.shortcut_corner && frame->shortcut!=0)
			 ? FRAME_SHORTCUT_W : 0);
		x=ev->x-g.x-off;
		tw=frame->tab_w+scr->grdata.spacing;
		x/=tw;
		p_tab_x=off+x*tw+g.x+FRAME_X(frame);
		p_tab_y=FRAME_Y(frame)+g.y;
		
		client=FIRST_THING(frame, WClient);
		while(x--){
			if(client==NULL)
				break;
			client=NEXT_THING(client, WClient);
		}
		
		if(client!=NULL)
			*thing=(WThing*)client;
		
		return &(wglobal.tab_bindmap);
	}
	return &(wglobal.main_bindmap);
}


/*}}}*/


/*{{{ handle_button_press/release/motion */


bool handle_button_press(XButtonEvent *ev)
{
	WBinding *pressbind=NULL;
	WThing *thing=NULL;
	uint button, state;
	WBindmap *bindmap=NULL;
	
	p_motiontmp=NULL;
	p_dir=VERTICAL;
	p_primn=TOP_OR_LEFT;
	
	state=ev->state;
	button=ev->button;
	
	thing=FIND_WINDOW_T(ev->window, WThing);
	
	if(thing==NULL){
		return FALSE;
	}else if(WTHING_IS(thing, WClientWin)){
		/* client */
		bindmap=&(wglobal.main_bindmap);
	}else if(WTHING_IS(thing, WFrame)){
		/* frame */
		bindmap=frame_press((WFrame*)thing, ev, &thing);
	}else if(WTHING_IS(thing, WInput)){
		/* input */
		bindmap=&(wglobal.input_bindmap);
	}else{
		/* unknown */
		return FALSE;
	}
	
	do_grab_kb_ptr(ev->root, thing, FocusChangeMask);
	
	if(p_clickcnt==1 && time_in_treshold(ev->time) && p_button==button &&
	   p_state==state && thing==p_dbltmp && p_bindmap==bindmap){
		pressbind=lookup_binding(bindmap, ACT_BUTTONDBLCLICK, state, button);
	}
	
	if(pressbind==NULL){
		pressbind=lookup_binding(bindmap, ACT_BUTTONPRESS, state, button);
	}
	
end:
	p_dbltmp=thing;
	p_bindmap=bindmap;
	p_button=button;
	p_state=state;
	p_orig_x=p_x=ev->x_root;
	p_orig_y=p_y=ev->y_root;
	p_time=ev->time;
	p_motion=FALSE;
	p_clickcnt=0;
	p_motiontmp_dirty=TRUE;
	p_screen=SCREEN_OF(thing);
	
	call_button(pressbind, ev);
	
	return TRUE;
}


bool handle_button_release(XButtonEvent *ev)
{
	WBinding *binding=NULL;
	
	if(p_button!=ev->button)
	   	return FALSE;

	if(p_motion==FALSE){
		p_clickcnt=1;
		binding=lookup_binding(p_bindmap, ACT_BUTTONCLICK, p_state, p_button);
	}else if(p_motiontmp_dirty){
		binding=lookup_binding(p_bindmap, ACT_BUTTONMOTION, p_state, p_button);
	}else{
		binding=p_motiontmp;
	}

	call_button(binding,  ev);
	
	return TRUE;
}


void handle_pointer_motion(XMotionEvent *ev)
{
	WThing *tmp;
	int dx, dy;
	
	if(p_motion==FALSE && motion_in_treshold(ev->x_root, ev->y_root))
		return;
	
	if(p_motiontmp_dirty){
		p_motiontmp=lookup_binding(p_bindmap, ACT_BUTTONMOTION, p_state, p_button);
		p_motiontmp_dirty=FALSE;
	}

	p_time=ev->time;
	dx=ev->x_root-p_x;
	dy=ev->y_root-p_y;
	p_x=ev->x_root;
	p_y=ev->y_root;	

	call_motion(p_motiontmp, ev, dx, dy);
	
	p_motion=TRUE;
}


/*}}}*/


/*{{{ Tab drag */


static void p_tabdrag(WClient *client, XMotionEvent *ev, int dx, int dy)
{
	WGRData *grdata=&(p_screen->grdata);
	WFrame *frame;
	int x, y;

	if(p_motion==FALSE){
		frame=FIND_PARENT(client, WFrame);
		
		if(frame==NULL)
			return;
		
		change_grab_cursor(CURSOR_DRAG);
		
		grdata->tabdrag_geom.w=frame->tab_w;
		grdata->tabdrag_geom.h=grdata->bar_h;
		XResizeWindow(wglobal.dpy, grdata->tabdrag_win, 
					  frame->tab_w, grdata->bar_h);
		XSelectInput(wglobal.dpy, grdata->tabdrag_win, ExposureMask);
		   
		client->flags|=CLIENT_DRAG;
		draw_frame_bar(frame, FALSE);
	}
	
	x=p_tab_x+(p_x-p_orig_x);
	y=p_tab_y+(p_y-p_orig_y);
	
	XMoveWindow(wglobal.dpy, grdata->tabdrag_win, x, y);
	
	if(p_motion==FALSE)
		XMapRaised(wglobal.dpy, grdata->tabdrag_win);
}	


static void p_tabdrag_end(WClient *client, XButtonEvent *ev)
{
	WGRData *grdata=&(p_screen->grdata);
	WFrame *frame, *newframe=NULL;
	Window win;
	
	XUnmapWindow(wglobal.dpy, grdata->tabdrag_win);	
	XSelectInput(wglobal.dpy, grdata->tabdrag_win, 0);
	
	client->flags&=~CLIENT_DRAG;
	
	frame=FIND_PARENT(client, WFrame);
	if(frame!=NULL)
		draw_frame_bar(frame, TRUE);

	/* Must be same screen */
	if(ev->root!=p_screen->root.win)
		return;
	
	if(find_window_at(ev->root, ev->x_root, ev->y_root, &win))
		newframe=find_frame_of(win);

	if(newframe==NULL)
		return;
	
	frame_attach_client(newframe, client, TRUE);
	set_focus((WThing*)newframe);
}


WDragHandler p_tabdrag_handler={
	(WMotionHandler*)p_tabdrag,
	(WButtonHandler*)p_tabdrag_end
};


/*}}}*/


/*{{{ Resize */


void p_resize(WFrame *frame, XMotionEvent *ev, int dx, int dy)
{
	if(p_motion==FALSE){
		begin_resize((WWindow*)frame, p_dir, p_primn);
		change_grab_cursor(CURSOR_RESIZE);
	}
		
	if(p_dir==VERTICAL)
		dx=dy;
		
	if(p_primn==TOP_OR_LEFT)
		dx=-dx;
		
	/*resize((WWindow*)frame, dx, p_dir, p_primn);*/
	resize((WWindow*)frame, dx, p_primn, FALSE);
}
			

void p_resize_end(WFrame *frame, XButtonEvent *ev)
{
	end_resize((WWindow*)frame);
}


WDragHandler p_resize_handler={
	(WMotionHandler*)p_resize,
	(WButtonHandler*)p_resize_end
};


/*}}}*/

