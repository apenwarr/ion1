/*
 * ion/pointer.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_POINTER_H
#define INCLUDED_POINTER_H

#include "common.h"
#include "clientwin.h"
#include "function.h"

INTRSTRUCT(WDragHandler)

enum{
	POINTER_NORMAL,
	POINTER_MENU,
	POINTER_MENU_MOVE
};

typedef void WButtonHandler(WThing *thing, XButtonEvent *ev);
typedef void WMotionHandler(WThing *thing, XMotionEvent *ev, int dx, int dy);

DECLSTRUCT(WDragHandler){
	WMotionHandler *motion;
	WButtonHandler *release;
};

extern bool handle_button_press(XButtonEvent *ev);
extern bool handle_button_release(XButtonEvent *ev);
extern void handle_pointer_motion(XMotionEvent *ev);
extern void get_pointer_rootpos(int *xret, int *yret);
extern bool find_window_at(Window rootwin, int x, int y,
						   Window *childret);

#endif /* INCLUDED_POINTER_H */
