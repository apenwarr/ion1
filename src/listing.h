/*
 * ion/listing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef EDLN_LISTING_H
#define EDLN_LISTING_H

#include "common.h"

INTRSTRUCT(WListing)

#include "draw.h"
#include "drawp.h"

DECLSTRUCT(WListing){
	char **strs;
	int nstrs;
	int *itemrows;
	int ncol, nrow, nitemcol, visrow;
	int firstitem, firstoff;
	int itemw, itemh, toth;
};

extern void init_listing(WListing *l);
extern void deinit_listing(WListing *l);
void setup_listing(WListing *l, XFontStruct *font, char **strs, int nstrs);
extern void fit_listing(DrawInfo *dinfo, WListing *l);
extern void draw_listing(DrawInfo *dinfo, WListing *l, bool complete);
extern bool scrollup_listing(WListing *l);
extern bool scrolldown_listing(WListing *l);

#endif /* EDLN_LISTING_H */
