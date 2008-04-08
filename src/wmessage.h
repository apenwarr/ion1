/*
 * ion/wmsg.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef WMESSAGE_H
#define WMESSAGE_H

#include "common.h"

INTROBJ(WMessage)

#include "thing.h"
#include "window.h"
#include "listing.h"
#include "input.h"


DECLOBJ(WMessage){
	WInput input;
	WListing listing;
};

extern WMessage *create_wmsg(WWindow *wwin, WRectangle geom, char *msg);
extern void deinit_wmsg(WMessage *msg);

#endif /* WMESSAGE_H */
