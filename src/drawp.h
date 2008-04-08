/*
 * ion/drawp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_DRAWP_H
#define INCLUDED_DRAWP_H

#include "common.h"
#include "draw.h"

INTRSTRUCT(DrawInfo)

enum{
	ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
};


DECLSTRUCT(DrawInfo){
	Window win;
	GC gc;
	WRectangle geom;
	WColorGroup *colors;
	WBorder *border;
	XFontStruct *font;
	WGRData *grdata;
};


#define WIN dinfo->win
#define XGC dinfo->gc
#define BORDER dinfo->border
#define COLORS dinfo->colors
#define GRDATA dinfo->grdata

#define X dinfo->geom.x
#define Y dinfo->geom.y
#define W dinfo->geom.w
#define H dinfo->geom.h
#define C_X ((X)+(BORDER)->tl)
#define C_Y ((Y)+(BORDER)->tl)
#define C_W ((W)-(BORDER)->tl-(BORDER)->br)
#define C_H ((H)-(BORDER)->tl-(BORDER)->br)
#define I_X BORDER_IX(BORDER, X)
#define I_Y BORDER_IY(BORDER, Y)
#define I_W BORDER_IW(BORDER, W)
#define I_H BORDER_IH(BORDER, H)

#define FONT (dinfo->font)

extern void do_draw_border(Window win, GC gc, int x, int y, int w, int h,
						   int tl, int br, Pixel tlc, Pixel brc);
extern void draw_border(DrawInfo *dinfo);
extern void draw_box(DrawInfo *dinfo, bool fill);
extern void draw_textbox(DrawInfo *dinfo,
						 const char *str, int align, bool fill);

#endif /* INCLUDED_DRAWP_H */
