/*
 * ion/property.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_PROPERTY_H
#define INCLUDED_PROPERTY_H

#include <X11/Xatom.h>

#include "common.h"
#include "clientwin.h"

extern int get_property(Display *dpy, Window win, Atom atom, Atom type,
						long len, uchar **p);
extern char *get_string_property(Window win, Atom a, int *nret);
extern void set_string_property(Window win, Atom a, const char *value);
extern bool get_integer_property(Window win, Atom a, int *vret);
extern void set_integer_property(Window win, Atom a, int value);
extern bool get_win_state(Window win, int *state);
extern void set_win_state(Window win, int state);

#endif /* INCLUDED_PROPERTY_H */

