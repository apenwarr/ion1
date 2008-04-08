/*
 * ion/listing.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include "common.h"
#include "draw.h"
#include "drawp.h"
#include "font.h"
#include "global.h"
#include "listing.h"


#define COL_SPACING 16
#define CONT_INDENT "xx"
#define CONT_INDENT_LEN 2
#define ITEMROWS(L, R) ((L)->itemrows==NULL ? 1 : (L)->itemrows[R])


static int strings_maxw(XFontStruct *font, char **strs, int nstrs)
{
	int maxw=0, w, i;
	
	for(i=0; i<nstrs; i++){
		w=XTextWidth(font, strs[i], strlen(strs[i]));
		if(w>maxw)
			maxw=w;
	}
	
	return maxw;
}


static int getbeg(XFontStruct *font, int maxw, char *str, int l, int *wret)
{
	int n=maxw/MAX_FONT_WIDTH(font);
	int w;

	while(n<l){
		w=XTextWidth(font, str, n);
		if(w>maxw)
			return n-1;
		*wret=w;
		n++;
	}
		
	return n;
}


static int string_nrows(XFontStruct *font, int maxw, char *str)
{
	int wrapw=XTextWidth(font, "\\", 1);
	int ciw=XTextWidth(font, CONT_INDENT, CONT_INDENT_LEN);
	int l2, l=strlen(str);
	int w;
	int nr=1;
	
	while(1){
		w=XTextWidth(font, str, l);
		if(w<maxw)
			break;
		l2=getbeg(font,  maxw-wrapw, str, l, &w);
		if(l2==0)
			break;
		if(nr==1)
			maxw-=ciw;
		nr++;
		l-=l2;
		str+=l2;
	}
	
	return nr;
}


static void draw_multirow(DrawInfo *dinfo, int x, int y,
						  int maxw, int h, char *str)
{
	int wrapw=XTextWidth(FONT, "\\", 1);
	int ciw=XTextWidth(FONT, CONT_INDENT, CONT_INDENT_LEN);
	int l2, l=strlen(str);
	int w;
	int nr=1;
	
	while(1){
		w=XTextWidth(FONT, str, l);
		if(w<maxw)
			break;
		l2=getbeg(FONT, maxw-wrapw, str, l, &w);
		if(l2==0)
			break;
		XDrawImageString(wglobal.dpy, WIN, XGC, x, y, str, l2);
		XDrawImageString(wglobal.dpy, WIN, XGC, x+w, y, "\\", 1);
		if(nr==1){
			maxw-=ciw;
			x+=ciw;
		}
		nr++;
		y+=h;
		l-=l2;
		str+=l2;
	}
	
	XDrawImageString(wglobal.dpy, WIN, XGC, x, y, str, l);
}

						  
static int col_fit(int w, int itemw, int spacing)
{
	int ncol=1;
	int tmp=w-itemw;
	itemw+=spacing;
	
	if(tmp>0)
		ncol+=tmp/itemw;
	
	return ncol;
}

static bool oneup(WListing *l, int *ip, int *rp)
{
	int i=*ip, r=*rp;
	int ir=ITEMROWS(l, i);
	
	if(r>0){
		(*rp)--;
		return TRUE;
	}
	
	if(i==0)
		return FALSE;
	
	(*ip)--;
	*rp=ITEMROWS(l, i-1)-1;
	return TRUE;
}


static bool onedown(WListing *l, int *ip, int *rp)
{
	int i=*ip, r=*rp;
	int ir=ITEMROWS(l, i);
	
	if(r<ir-1){
		(*rp)++;
		return TRUE;
	}
	
	if(i==l->nitemcol-1)
		return FALSE;
	
	(*ip)++;
	*rp=0;
	return TRUE;
}


void setup_listing(WListing *l, XFontStruct *font,
				   char **strs, int nstrs)
{
	int maxw;
	
	if(l->strs!=NULL)
		deinit_listing(l);

	l->itemrows=ALLOC_N(int, nstrs);
	
	maxw=strings_maxw(font, strs, nstrs);
	
	l->strs=strs;
	l->nstrs=nstrs;
	l->itemw=maxw+COL_SPACING;
	l->itemh=FONT_HEIGHT(font);
}


void fit_listing(DrawInfo *dinfo, WListing *l)
{
	int ncol, nrow=0, visrow;
	int i;
	int w=I_W, h=I_H;
	
	ncol=col_fit(w, l->itemw-COL_SPACING, COL_SPACING);

	if(l->itemrows!=NULL){
		for(i=0; i<l->nstrs; i++){
			if(ncol!=1){
				l->itemrows[i]=1;
			}else{
				l->itemrows[i]=string_nrows(FONT, w, l->strs[i]);
				nrow+=l->itemrows[i];
			}
		}
	}
	
	if(ncol>1){
		nrow=l->nstrs/ncol+(l->nstrs%ncol ? 1 : 0);
		l->nitemcol=nrow;
	}else{
		l->nitemcol=l->nstrs;
	}
	
	visrow=h/l->itemh;
	
	if(visrow>nrow)
		visrow=nrow;
	
	l->ncol=ncol;
	l->nrow=nrow;
	l->visrow=visrow;
	l->toth=visrow*l->itemh;

	l->firstitem=l->nitemcol-1;
	l->firstoff=ITEMROWS(l, l->nitemcol-1)-1;
	for(i=1; i<visrow; i++)
		oneup(l, &(l->firstitem), &(l->firstoff));
	
}


void deinit_listing(WListing *l)
{
	int i;
	
	if(l->strs==NULL)
		return;
	
	while(l->nstrs--)
		free(l->strs[l->nstrs]);
	free(l->strs);
	l->strs=NULL;
	if(l->itemrows){
		free(l->itemrows);
		l->itemrows=NULL;
	}
}


void init_listing(WListing *l)
{
	l->nstrs=0;
	l->strs=NULL;
}


static void do_draw_listing(DrawInfo *dinfo, WListing *l)
{
	int r, c, i, x, y;
	XRectangle rect;
	
	rect.x=I_X; rect.y=I_Y; rect.width=I_W; rect.height=I_H;
	XSetClipRectangles(wglobal.dpy, XGC, 0, 0, &rect, 1, Unsorted);

	XSetForeground(wglobal.dpy, XGC, COLORS->fg);
	XSetBackground(wglobal.dpy, XGC, COLORS->bg);
	
	x=I_X;
	c=0;
	while(1){
		y=I_Y+FONT_BASELINE(FONT);
		i=l->firstitem+c*l->nitemcol;
		r=-l->firstoff;
		y+=r*l->itemh;
		while(r<l->visrow){
			if(i>=l->nstrs)
				goto finished;
			
			draw_multirow(dinfo, x, y, I_W, l->itemh,
						  l->strs[i]);

			y+=l->itemh*ITEMROWS(l, i);
			r+=ITEMROWS(l, i);
			i++;
		}
		x+=l->itemw;
		c++;
	}
	
finished:
	XSetClipMask(wglobal.dpy, XGC, None);
}


void draw_listing(DrawInfo *dinfo, WListing *l, bool complete)
{
	draw_box(dinfo, complete);
	do_draw_listing(dinfo, l);
}

static bool do_scrollup_listing(WListing *l, int n)
{
	int i=l->firstitem;
	int r=l->firstoff;
	bool ret=FALSE;
	
	while(n>0){
		if(!oneup(l, &i, &r))
			break;
		ret=TRUE;
		n--;
	}

	l->firstitem=i;
	l->firstoff=r;
	
	return ret;
}


static bool do_scrolldown_listing(WListing *l, int n)
{
	int i=l->firstitem;
	int r=l->firstoff;
	int br=r, bi=i;
	int bc=l->visrow;
	bool ret=FALSE;
	
	while(--bc>0)
		onedown(l, &bi, &br);
	
	while(n>0){
		if(!onedown(l, &bi, &br))
			break;
		onedown(l, &i, &r);
		ret=TRUE;
		n--;
	}

	l->firstitem=i;
	l->firstoff=r;
	
	return ret;
}


bool scrollup_listing(WListing *l)
{
	return do_scrollup_listing(l, l->visrow);
}


bool scrolldown_listing(WListing *l)
{
	return do_scrolldown_listing(l, l->visrow);
}
