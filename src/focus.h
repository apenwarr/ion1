/*
 * ion/focus.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_FOCUS_H
#define INCLUDED_FOCUS_H

#include "global.h"
#include "common.h"
#include "thing.h"
#include "window.h"

extern void skip_focusenter();
extern void set_input_focus(Window win);
extern void do_set_focus(WThing *thing);
extern void set_focus(WThing *thing);
extern void warp(WWindow *thing);
extern void set_previous(WThing *thing);
extern void protect_previous();
extern void unprotect_previous();

#endif /* INCLUDED_FOCUS_H */
