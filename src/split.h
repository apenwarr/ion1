/*
 * ion/split.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SPLIT_H
#define INCLUDED_SPLIT_H

#include "common.h"

INTROBJ(WWsSplit)
INTRSTRUCT(WResizeTmp)

#include "window.h"

enum WSplitDir{
	HORIZONTAL,
	VERTICAL
};


enum PrimaryNode{
	ANY,
	TOP_OR_LEFT,
	BOTTOM_OR_RIGHT
};


DECLOBJ(WWsSplit){
	WObj obj;
	int dir;
	WRectangle geom;
	int tmpsize, knowsize;
	int res;
	int current;
	WObj *tl, *br;
	WWsSplit *parent;
};


DECLSTRUCT(WResizeTmp){
	WObj *startnode;
	int postmp, sizetmp;
	int winpostmp, winsizetmp;
	int dir;
};

extern WWsSplit *create_split(int dir, WObj *tl, WObj *br, WRectangle geom);
extern int tree_do_resize(WObj *node_, int dir, int npos, int nsize);
extern int calcresize_window(WWindow *wwin, int dir, int prim, int nsize,
							 WResizeTmp *tmp);
extern void resize_tmp(const WResizeTmp *tmp);

extern void goto_above(WWindow *wwin);
extern void goto_below(WWindow *wwin);
extern void goto_right(WWindow *wwin);
extern void goto_left(WWindow *wwin);

extern int wwin_size(WWindow *wwin, int dir);
extern int wwin_other_size(WWindow *wwin, int dir);
extern int wwin_pos(WWindow *wwin, int dir);
extern int tree_size(WObj *obj, int dir);
extern int tree_other_size(WObj *obj, int dir);
extern int tree_pos(WObj *obj, int dir);
extern void set_current_wswindow(WWindow *wwin);

#endif /* INCLUDED_SPLIT_H */

