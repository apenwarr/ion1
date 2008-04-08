/*
 * ion/inputp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INPUTP_H
#define INPUTP_H

#include "input.h"
#include "drawp.h"
#include "thingp.h"

#define INPUT_BORDER_SIZE(GRDATA) \
	(BORDER_TL_TOTAL(&(GRDATA->input_border))+ \
	 BORDER_BR_TOTAL(&(GRDATA->input_border)))

#define INPUT_MASK (ExposureMask|KeyPressMask| \
					ButtonPressMask|FocusChangeMask)

#define INPUT_FONT(GRDATA) ((GRDATA)->font)

typedef void WInputCalcSizeFn(WInput*, WRectangle*);
typedef void WInputScrollFn(WInput*);
typedef void WInputDrawFn(WInput*, bool complete);

DECLSTRUCT(WInputFuntab){
	WThingFuntab thing_funtab;
	void (*calcsize_fn)();
	void (*scrollup_fn)();
	void (*scrolldown_fn)();
	void (*draw_fn)();
};

extern void setup_input_dinfo(WInput *input, DrawInfo *dinfo);

#endif /* INPUTP_H */
