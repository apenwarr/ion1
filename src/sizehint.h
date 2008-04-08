/*
 * ion/sizehint.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SIZEHINT_H
#define INCLUDED_SIZEHINT_H

#include "common.h"

void correct_size(int *wp, int *hp, XSizeHints *hints, bool min);
void get_clientwin_size_hints(WClientWin *cwin);

#endif /* INCLUDED_SIZEHINT_H */
