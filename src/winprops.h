/*
 * ion/winprops.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_WINPROPS_H
#define INCLUDED_WINPROPS_H

#include "common.h"
#include "obj.h"

enum TransientMode{
	TRANSIENT_MODE_NORMAL,
	TRANSIENT_MODE_CURRENT,
	TRANSIENT_MODE_OFF
};

INTRSTRUCT(WWinProp)

DECLSTRUCT(WWinProp){
	char *data;
	char *wclass;
	char *winstance;
	
	int flags;
	int switchto;
	int stubborn;
	int transient_mode;
	int max_w, max_h;
	int aspect_w, aspect_h;
	
	WWinProp *next, *prev;
};


extern WWinProp *find_winprop(const char *wclass, const char *winstance);
extern WWinProp *find_winprop_win(Window win);
extern void free_winprop(WWinProp *winprop);
extern void register_winprop(WWinProp *winprop);

#endif /* INCLUDED_WINPROPS_H */
