/*
 * ion/input.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INPUT_H
#define INPUT_H

#include "common.h"

INTROBJ(WInput)
INTRSTRUCT(WInputFuntab)

#include "window.h"

DECLOBJ(WInput){
	WWindow win;
	WRectangle max_geom;
};

extern bool init_input(WInput *input, WWindow *parent, WRectangle geom);
extern void deinit_input(WInput *input);

extern void input_resize(WInput *input, WRectangle max_geom);
extern void input_refit(WInput *input);
extern void input_draw(WInput *input, bool complete);
extern void input_cancel(WInput *input);
extern void input_scrollup(WInput *input);
extern void input_scrolldown(WInput *input);

#endif /* INPUT_H */
