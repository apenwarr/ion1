/*
 * ion/shortcut.h
 *
 * Copyright (c) Lukas Schroeder 2002
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SHORTCUT_H
#define INCLUDED_SHORTCUT_H

#include <X11/keysym.h>
#include "function.h"
#include "client.h"
#include "wedln.h"


extern bool shortcut_is_valid(int cut);
extern void shortcut_remove(int cut);
extern void shortcut_set(WThing *thing, int cut);

extern void set_shortcut(WFrame *frame);
extern void goto_shortcut(WFrame *frame);


#endif /* INCLUDED_SHORTCUT_H */
