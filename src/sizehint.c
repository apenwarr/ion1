/*
 * ion/sizehint.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>

#include "common.h"
#include "clientwin.h"
#include "global.h"


/*{{{ correct_size */


static void do_correct_aspect(int max_w, int max_h, int ax, int ay,
							  int *wret, int *hret)
{
	int w=*wret, h=*hret;

	if(ax>ay){
		h=(w*ay)/ax;
		if(max_h>0 && h>max_h){
			h=max_h;
			w=(h*ax)/ay;
		}
	}else{
		w=(h*ax)/ay;
		if(max_w>0 && w>max_w){
			w=max_w;
			h=(w*ay)/ax;
		}
	}
	
	*wret=w;
	*hret=h;
}


static void correct_aspect(int max_w, int max_h, XSizeHints *hints,
						   int *wret, int *hret)
{
	if(!(hints->flags&PAspect))
		return;
	
	if(*wret*hints->max_aspect.y>*hret*hints->max_aspect.x){
		do_correct_aspect(max_w, max_h,
						  hints->min_aspect.x, hints->min_aspect.y,
						  wret, hret);
	}

	if(*wret*hints->min_aspect.y<*hret*hints->min_aspect.x){
		do_correct_aspect(max_w, max_h,
						  hints->max_aspect.x, hints->max_aspect.y,
						  wret, hret);
	}
}


void correct_size(int *wp, int *hp, XSizeHints *hints, bool min)
{
	int w=*wp;
	int h=*hp;
	
	if(min){
		if(w<hints->min_width)
			w=hints->min_width;
		if(h<hints->min_height)
			h=hints->min_height;
		
		correct_aspect(w, h, hints, &w, &h);
	}else{
		if(w>=hints->min_width && h>=hints->min_height)
			correct_aspect(w, h, hints, &w, &h);
	}
	
	if(hints->flags&PResizeInc){
		/* base size should be set to 0 if none given by user program */
		if(w>hints->min_width)
			w=((w-hints->min_width)/hints->width_inc)*hints->width_inc+hints->min_width;
		if(h>hints->min_height)
			h=((h-hints->min_height)/hints->height_inc)*hints->height_inc+hints->min_height;
	}
	
	if(hints->flags&PMaxSize){
		if(w>hints->max_width)
			w=hints->max_width;
		if(h>hints->max_height)
			h=hints->max_height;
	}
	
	*wp=w;
	*hp=h;
}


/*}}}*/


/*{{{ get_sizehints */

#define CWIN_MIN_W(SCR) (SCR)->w_unit
#define CWIN_MIN_H(SCR) (SCR)->h_unit

static void get_sizehints(WScreen *scr, Window win, XSizeHints *hints)
{
	int minh, minw;
	long supplied=0;
	
	memset(hints, 0, sizeof(*hints));
	XGetWMNormalHints(wglobal.dpy, win, hints, &supplied);

	if(!(hints->flags&PMinSize) || hints->min_width<CWIN_MIN_W(scr))
		hints->min_width=CWIN_MIN_W(scr);
	/*if(!(hints->flags&PBaseSize) || hints->base_width<hints->min_width)
		hints->base_width=hints->min_width;*/

	if(!(hints->flags&PMinSize) || hints->min_height<CWIN_MIN_H(scr))
		hints->min_height=CWIN_MIN_H(scr);
	/*if(!(hints->flags&PBaseSize) || hints->base_height<hints->min_height)
		hints->base_height=hints->min_height;*/

	if(hints->flags&PMaxSize){
		if(hints->max_width<hints->min_width)
			hints->max_width=hints->min_width;
		if(hints->max_height<hints->min_height)
			hints->max_height=hints->min_height;
	}
	
	/*hints->flags|=PBaseSize;|PMinSize;*/

	if(hints->flags&PResizeInc){
		if(hints->width_inc<=0 || hints->height_inc<=0){
			warn("Invalid client-supplied width/height increment");
			hints->flags&=~PResizeInc;
		}
	}
	
	if(hints->flags&PAspect){
		if(hints->min_aspect.x<=0 || hints->min_aspect.y<=0 ||
		   hints->min_aspect.x<=0 || hints->min_aspect.y<=0){
			warn("Invalid client-supplied aspect-ratio");
			hints->flags&=~PAspect;
		}
	}
	
	if(!(hints->flags&PWinGravity))
		hints->win_gravity=ForgetGravity;
}


void get_clientwin_size_hints(WClientWin *cwin)
{
	XSizeHints tmp=cwin->size_hints;
	
	get_sizehints(SCREEN_OF(cwin), cwin->win, &(cwin->size_hints));
	
	if(cwin->flags&CWIN_PROP_MAXSIZE){
		cwin->size_hints.max_width=tmp.max_width;
		cwin->size_hints.max_height=tmp.max_height;
		cwin->size_hints.flags|=PMaxSize;
	}
	
	if(cwin->flags&CWIN_PROP_ASPECT){
		cwin->size_hints.min_aspect=tmp.min_aspect;
		cwin->size_hints.max_aspect=tmp.max_aspect;
		cwin->size_hints.flags|=PAspect;
	}
}


/*}}}*/

