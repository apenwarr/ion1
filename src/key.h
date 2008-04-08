/*
 * ion/key.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_KEY_H
#define INCLUDED_KEY_H

#include <X11/keysym.h>
#include "function.h"
#include "client.h"
#include "wedln.h"

extern void handle_keypress(XKeyEvent *ev);

extern void quote_next(WClient *cwin);
void adhoc_insstr(WEdln *wedln, XKeyEvent *ev);


#endif /* INCLUDED_KEY_H */
