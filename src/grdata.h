/*
 * ion/grdata.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_GRDATA_H
#define INCLUDED_GRDATA_H

#include "common.h"

INTRSTRUCT(WGRData)
INTRSTRUCT(WColorGroup)
INTRSTRUCT(WBorder)

#define BORDER_TL_TOTAL(BORDER) ((BORDER)->tl+(BORDER)->ipad)
#define BORDER_BR_TOTAL(BORDER) ((BORDER)->br+(BORDER)->ipad)
#define BORDER_IX(BORDER, X) (X+BORDER_TL_TOTAL(BORDER))
#define BORDER_IY(BORDER, Y) (Y+BORDER_TL_TOTAL(BORDER))
#define BORDER_IW(BORDER, W) (W-BORDER_TL_TOTAL(BORDER)-BORDER_BR_TOTAL(BORDER))
#define BORDER_IH(BORDER, H) (H-BORDER_TL_TOTAL(BORDER)-BORDER_BR_TOTAL(BORDER))

DECLSTRUCT(WColorGroup){
	Pixel bg, hl, sh, fg;
};


DECLSTRUCT(WBorder){
	int tl, br, ipad;
};


DECLSTRUCT(WGRData){
	/* configurable data */
	bool bar_inside_frame;
	int spacing;
	int shortcut_corner; /* 0=left; 1=right; */
	
	WColorGroup act_frame_colors, frame_colors;
	WColorGroup act_tab_colors, tab_colors;
	WColorGroup act_tab_sel_colors, tab_sel_colors;
	WColorGroup act_tab_shortcut_colors, tab_shortcut_colors;
	WColorGroup input_colors;
	Pixel bgcolor, selection_bgcolor, selection_fgcolor;
	bool shortcutc_set;
	
	WBorder frame_border;
	WBorder tab_border;
	WBorder input_border;

	XFontStruct *font, *tab_font;
	
	/* calculated data (from configurable) */
	int bar_h;
	WRectangle client_off, bar_off, border_off;
	
	/* other data */
	GC gc;
	GC tab_gc;
	GC xor_gc;
	GC stipple_gc;
	GC copy_gc;
	Pixmap stick_pixmap;
	int stick_pixmap_w;
	int stick_pixmap_h;

	Window moveres_win;
	WRectangle moveres_geom;
	Window tabdrag_win;
	WRectangle tabdrag_geom;
};

#endif /* INCLUDED_GRDATA_H */
