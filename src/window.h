/*
 * ion/window.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_WINDOW_H
#define INCLUDED_WINDOW_H

#include "common.h"

INTROBJ(WWindow)

#include "thing.h"
#include "binding.h"
#include "split.h"


#define WWINDOW_UNMAPPABLE	0x0001
#define WWINDOW_MAPPED		0x0002
#define WWINDOW_WFORCED		0x0010
#define WWINDOW_HFORCED		0x0020

#define FIND_WINDOW_T(WIN, TYPE) (TYPE*)find_window_t(WIN, &OBJDESCR(TYPE))
#define FIND_WINDOW(WIN) find_window(WIN)


DECLOBJ(WWindow){
	WThing thing;
	int flags;
	Window win;
	WRectangle geom;
	XIC xic;
	WBindmap *bindmap;
	
	WWsSplit *split;
	int min_w, min_h;
	int saved_w, saved_h;
};


extern bool init_window(WWindow *p, Window win, WRectangle geom);
extern void deinit_window(WWindow *win);

extern WThing *find_window(Window win);
extern WThing *find_window_t(Window win, const WObjDescr *descr);

extern void map_window(WWindow *wwin);
extern void unmap_window(WWindow *wwin);
extern void do_map_window(WWindow *wwin);
extern void do_unmap_window(WWindow *wwin);

extern WWindow *window_of(WThing *thing);
extern WWindow *find_window_of(Window win);

extern void focus_window(WWindow *wwin);

#endif /* INCLUDED_WINDOW_H */
