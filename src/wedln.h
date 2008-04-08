/*
 * ion/wedln.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef WEDLN_H
#define WEDLN_H

#include "common.h"

INTROBJ(WEdln)

#include "thing.h"
#include "window.h"
#include "xic.h"
#include "listing.h"
#include "input.h"
#include "edln.h"

typedef void WEdlnHandler(WThing *p, char *str, char *userdata);

DECLOBJ(WEdln){
	WInput input;

	WListing complist;
	Edln edln;

	char *prompt;
	int prompt_len;
	int prompt_w;
	int vstart;
	
	WEdlnHandler *handler;
	char *userdata;
};

extern WEdln *create_wedln(WWindow *wwin, WRectangle geom, WEdlnHandler *handler,
						   const char *prompt, const char *dflt);
extern void deinit_wedln(WEdln *edln);
extern void wedln_finish(WEdln *wedln);
extern void wedln_paste(WEdln *wedln);
extern void wedln_draw(WEdln *wedln, bool complete);

#endif /* WEDLN_H */
