/*
 * ion/split.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <X11/Xmd.h>

#include "common.h"
#include "screen.h"
#include "workspace.h"
#include "frame.h"
#include "focus.h"
#include "global.h"
#include "split.h"
#include "window.h"
#include "objp.h"
#include "splitwin.h"


IMPLOBJ(WWsSplit, WObj, NULL)


/*{{{ Misc. */


int wwin_size(WWindow *wwin, int dir)
{
	if(dir==HORIZONTAL)
		return wwin->geom.w;
	return wwin->geom.h;
}


int wwin_other_size(WWindow *wwin, int dir)
{
	if(dir==HORIZONTAL)
		return wwin->geom.w;
	return wwin->geom.h;
}


int wwin_pos(WWindow *wwin, int dir)
{
	if(dir==HORIZONTAL)
		return wwin->geom.x;
	return wwin->geom.y;
}


int tree_size(WObj *obj, int dir)
{
	if(WOBJ_IS(obj, WWindow))
		return wwin_size((WWindow*)obj, dir);
	
	if(dir==HORIZONTAL)
		return ((WWsSplit*)obj)->geom.w;
	return ((WWsSplit*)obj)->geom.h;
}


int tree_pos(WObj *obj, int dir)
{
	if(WOBJ_IS(obj, WWindow))
		return wwin_pos((WWindow*)obj, dir);
	
	if(dir==HORIZONTAL)
		return ((WWsSplit*)obj)->geom.x;
	return ((WWsSplit*)obj)->geom.y;
}


static WRectangle tree_geom(WObj *obj)
{
	if(WOBJ_IS(obj, WWindow))
		return ((WWindow*)obj)->geom;
	
	return ((WWsSplit*)obj)->geom;
}


int tree_other_size(WObj *obj, int dir)
{
	if(WOBJ_IS(obj, WWindow))
		return wwin_other_size((WWindow*)obj, dir);
	
	if(dir==HORIZONTAL)
		return ((WWsSplit*)obj)->geom.h;
	return ((WWsSplit*)obj)->geom.w;
}


static int wwin_calcresize(WWindow *wwin, int dir, int nsize)
{
	if(dir==HORIZONTAL){
		if(nsize<wwin->min_w)
			return wwin->min_w;
	}else{
		if(nsize<wwin->min_h)
			return wwin->min_h;
	}
	
	return nsize;
}


static int wwin_resize(WWindow *wwin, int dir, int npos, int nsize)
{
	WRectangle geom;
	
	if(dir==VERTICAL){
		geom.x=wwin->geom.x;
		geom.w=wwin->geom.w;
		geom.y=npos;
		geom.h=nsize;
		wwin->flags&=~WWINDOW_HFORCED;
	}else{
		geom.x=npos;
		geom.w=nsize;
		geom.y=wwin->geom.y;
		geom.h=wwin->geom.h;
		wwin->flags&=~WWINDOW_WFORCED;
	}
	
	if(WTHING_IS(wwin, WFrame))
		set_frame_geom((WFrame*)wwin, geom);
	
	return nsize;
}


static WWsSplit *split_of(WObj *obj)
{
	if(WOBJ_IS(obj, WWindow))
		return ((WWindow*)obj)->split;
	
	assert(WOBJ_IS(obj, WWsSplit));
	
	return ((WWsSplit*)obj)->parent;
}


static void set_split_of(WObj *obj, WWsSplit *split)
{
	if(WOBJ_IS(obj, WWindow)){
		((WWindow*)obj)->split=split;
		return;
	}
	
	assert(WOBJ_IS(obj, WWsSplit));
	
	((WWsSplit*)obj)->parent=split;
}


/*}}}*/


/*{{{ Low-level resize code */


static int tree_calcresize(WObj *node_, int dir, int primn,
						   int nsize)
{
	WWsSplit *node;
	WObj *o1, *o2;
	int s1, s2, ns1, ns2;
	
	assert(node_!=NULL);
	
	/* Reached a window? */
	if(!WTHING_IS(node_, WWsSplit)){
		assert(WTHING_IS(node_, WWindow));
		return wwin_calcresize((WWindow*)node_, dir, nsize);
	}
	
	node=(WWsSplit*)node_;
	
	if(node->dir!=dir){
		/* Found a split in the other direction than the resize */
		s1=tree_calcresize(node->tl, dir, primn, nsize);
		s2=tree_calcresize(node->br, dir, primn, nsize);
		
		if(s1>s2){
			/*if(nsize>tree_size(node->tl, dir)){
				tree_calcresize(node->tl, dir, primn, s2);
				s1=s2;
			}else*/{
				tree_calcresize(node->br, dir, primn, s1);
			}
		}else if(s2>s1){
			/*if(nsize>tree_size(node->br, dir)){
				tree_calcresize(node->br, dir, primn, s1);
			}else*/{
				tree_calcresize(node->tl, dir, primn, s2);
				s1=s2;
			}
		}
		node->res=ANY;
		node->knowsize=ANY;
		return (node->tmpsize=s1);
	}else{
		if(primn==TOP_OR_LEFT){
			/* Resize top or left node first */
			o1=node->tl;
			o2=node->br;
		}else{
			/* Resize bottom or right node first */
			o1=node->br;
			o2=node->tl;
			primn=BOTTOM_OR_RIGHT;
		}
		
		s2=tree_size(o2, dir);
		ns1=nsize-s2;
		s1=tree_calcresize(o1, dir, primn, ns1);
		
		/*if(s1!=ns1){*/
			ns2=nsize-s1;
			s2=tree_calcresize(o2, dir, primn, ns2);
			node->res=ANY;
		/*}else{
			node->res=primn;
		}*/
		
		node->knowsize=primn;
		node->tmpsize=s1;
		return s1+s2;
	}
}


/* Resize according to parameters calculated by wcalcres
 */
int tree_do_resize(WObj *node_, int dir, int npos, int nsize)
{
	WWsSplit *node;
	int tls, brs;
	int s=0;
	int pos=npos;
	
	if(node_==NULL){
		return 0;
	}
	assert(node_!=NULL);
	
	/* Reached a window? */
	if(!WTHING_IS(node_, WWsSplit)){
		assert(WTHING_IS(node_, WWindow));
		return wwin_resize((WWindow*)node_, dir, npos, nsize);
	}
	
	node=(WWsSplit*)node_;
	
	if(node->dir!=dir){
		tree_do_resize(node->tl, dir, npos, nsize);
		s=tree_do_resize(node->br, dir, npos, nsize);
	}else{
		if(node->knowsize==TOP_OR_LEFT){
			tls=node->tmpsize;
			brs=nsize-tls;
		}else if(node->knowsize==BOTTOM_OR_RIGHT){
			brs=node->tmpsize;
			tls=nsize-brs;
		}else{
			fprintf(stderr, "Resize is broken!");
			brs=nsize/2;
			tls=nsize-brs;
		}
		
		
		/*if(node->res!=BOTTOM_OR_RIGHT)*/
			s+=tree_do_resize(node->tl, dir, npos, tls);
		/*else
			s+=tree_size(node->tl, dir);*/
		
		npos+=s;
		
		/*if(node->res!=TOP_OR_LEFT)*/
			s+=tree_do_resize(node->br, dir, npos, brs);
		/*else
			s+=tree_size(node->br, dir);*/
	}
	
	if(dir==VERTICAL){
		node->geom.y=pos;
		node->geom.h=s;
	}else{
		node->geom.x=pos;
		node->geom.w=s;
	}
	
	return s;
}


/* Calculate parameters for resizing <split> and possibly anything
 * above it so that the <dir>-dimension of <from> becomes <nsize>
 * (if possible).
 */
static void wcalcres(WWsSplit *split, int dir, int primn,
					 int nsize, int from, WResizeTmp *ret)
{
	int s, s2, ds, rs, rp;
	WObj *other=(from==TOP_OR_LEFT ? split->br : split->tl);
	WWsSplit *p;
	
	s=tree_size((WObj*)split, dir);
	
	if(dir!=split->dir){
		/* It might not be possible to shrink the other as much */
		ds=tree_calcresize(other, dir, primn, nsize);
		nsize=ds;
		s2=0;
	}else{
		if(primn!=from)
			s2=tree_calcresize(other, dir, from, s-nsize);
		else
			s2=tree_calcresize(other, dir, from, tree_size(other, dir));
	}
	ds=nsize+s2;

	p=split->parent;
	
	if(p==NULL || ds==s){
		rs=s;
		rp=tree_pos((WObj*)split, dir);
		ret->postmp=rp;
		ret->sizetmp=rs;
		/* Don't have to resize the other branch if the split is not
		 * in the wanted direction
		 */
		if(split->dir!=dir)
			ret->startnode=(from==TOP_OR_LEFT ? split->tl : split->br);
		else
			ret->startnode=(WObj*)split;
	}else{
		wcalcres(p, dir, primn, ds,
				 (p->tl==(WObj*)split ? TOP_OR_LEFT : BOTTOM_OR_RIGHT),
				 ret);
		rp=ret->winpostmp;
		rs=ret->winsizetmp;
		
		if(rs!=ds && dir!=split->dir)
			tree_calcresize(other, dir, primn, rs);
	}
	
	nsize=rs-s2;
	
	split->knowsize=from;
	split->tmpsize=nsize;
	split->res=ANY;
	
	if(from==TOP_OR_LEFT)
		ret->winpostmp=rp;
	else
		ret->winpostmp=rp+s2;
	ret->winsizetmp=nsize;
}


static int calcresize_obj(WObj *obj, int dir, int primn, int nsize,
						  WResizeTmp *ret)
{
	WWsSplit *split=split_of(obj);
	
	nsize=tree_calcresize(obj, dir, primn, nsize);
	
	ret->dir=dir;
	
	if(split==NULL){
		ret->winsizetmp=ret->sizetmp=tree_size(obj, dir);
		ret->winpostmp=ret->postmp=tree_pos(obj, dir);
		ret->startnode=NULL;
		ret->dir=dir;
	}else{
		wcalcres(split, dir, primn, nsize, 
				 (split->tl==obj ? TOP_OR_LEFT : BOTTOM_OR_RIGHT),
				 ret);
	}
	return ret->winsizetmp;
}


/*}}}*/



/*{{{ Resize interface */


int calcresize_window(WWindow *wwin, int dir, int primn, int nsize,
					  WResizeTmp *ret)
{
	return calcresize_obj((WObj*)wwin, dir, primn, nsize, ret);
}


void resize_tmp(const WResizeTmp *tmp)
{
	tree_do_resize(tmp->startnode, tmp->dir, tmp->postmp, tmp->sizetmp);
}


/*}}}*/


/*{{{ Split */


WWsSplit *create_split(int dir, WObj *tl, WObj *br, WRectangle geom)
{
	WWsSplit *split=ALLOC(WWsSplit);
	
	if(split==NULL){
		warn_err();
		return NULL;
	}
	
	WOBJ_INIT(split, WWsSplit);
	
	split->dir=dir;
	split->tl=tl;
	split->br=br;
	split->geom=geom;
	split->parent=NULL;
	split->current=0;
	
	return split;
}


static WWindow *do_split_at(WWorkspace *ws, WObj *obj, int dir, int primn,
							int minsize, WSplitCreate *fn)
{
	int s, sn, gs, pos;
	WWsSplit *split, *nsplit;
	WRectangle geom;
	WWindow *nwin;
	WResizeTmp rtmp;
	
	assert(obj!=NULL);
	
	if(primn!=TOP_OR_LEFT && primn!=BOTTOM_OR_RIGHT)
		primn=BOTTOM_OR_RIGHT;
	if(dir!=HORIZONTAL && dir!=VERTICAL)
		dir=VERTICAL;
	
	/* First possibly resize <obj> so that the area allocated by it
	 * is big enough to fit the new object and <obj> itself without
	 * them becoming too small.
	 */

	s=tree_size(obj, dir);
	sn=s/2;
	
	if(sn<minsize)
		sn=minsize;
	
	gs=tree_calcresize(obj, dir, primn, s-sn);

	if(gs+sn>s){
		s=calcresize_obj(obj, dir, ANY, gs+sn, &rtmp);
		if(gs+sn>s){
			warn("Cannot resize: minimum size reached");
			return NULL;
		}
		resize_tmp(&rtmp);
	}
	
	/* Create split and new window
	 */
	geom=tree_geom(obj);
	
	nsplit=create_split(dir, NULL, NULL, geom);
	
	if(nsplit==NULL)
		return NULL;
	
	if(dir==VERTICAL){
		if(primn==BOTTOM_OR_RIGHT)
			geom.y+=gs;
		geom.h=sn;
	}else{
		if(primn==BOTTOM_OR_RIGHT)
			geom.x+=gs;
		geom.w=sn;
	}
	
	nwin=fn(SCREEN_OF(ws), geom);
	
	if(nwin==NULL){
		free(nsplit);
		return NULL;
	}
	
	add_workspace_window(ws, nwin);
	
	/* Now that everything's ok, resize (and move) the original
	 * obj.
	 */
	pos=tree_pos(obj, dir);
	if(primn!=BOTTOM_OR_RIGHT)
		pos+=sn;
	s=tree_calcresize(obj, dir, primn, gs);
	tree_do_resize(obj, dir, pos, s);

	/* Set up split structure
	 */
	split=split_of(obj);
	
	set_split_of(obj, nsplit);
	nwin->split=nsplit;
	
	if(primn==BOTTOM_OR_RIGHT){
		nsplit->tl=obj;
		nsplit->br=(WObj*)nwin;
	}else{
		nsplit->tl=(WObj*)nwin;
		nsplit->br=obj;
	}
	
	if(split!=NULL){
		if(obj==split->tl)
			split->tl=(WObj*)nsplit;
		else
			split->br=(WObj*)nsplit;
		nsplit->parent=split;
	}else{
		ws->splitree=(WObj*)nsplit;
	}
	
	return nwin;
}


WWindow *split_window(WWindow *wwin, int dir, int minsize, WSplitCreate *fn)
{
	WWorkspace *ws=FIND_PARENT(wwin, WWorkspace);
	
	assert(ws!=NULL);
	
	return do_split_at(ws, (WObj*)wwin, dir, BOTTOM_OR_RIGHT, minsize, fn);
}


WWindow *split_toplevel(WWorkspace *ws, int dir, int primn, int minsize,
						WSplitCreate *fn)
{
	if(ws->splitree==NULL)
		return NULL;
	
	return do_split_at(ws, ws->splitree, dir, primn, minsize, fn);
}


/*}}}*/


/*{{{ Navigation */


static WWindow *left_or_topmost_current(WObj *obj, int dir)
{
	WWsSplit *split;
	
	while(1){
		if(WOBJ_IS(obj, WWindow))
			return (WWindow*)obj;
		
		assert(WOBJ_IS(obj, WWsSplit));
		
		split=(WWsSplit*)obj;
		
		if(split->dir==dir){
			obj=split->tl;
			continue;
		}
		
		obj=(split->current==0 ? split->tl : split->br);
	}
	
	assert(0);
}


static WWindow *right_or_bottomost_current(WObj *obj, int dir)
{
	WWsSplit *split;
	
	while(1){
		if(WOBJ_IS(obj, WWindow))
			return (WWindow*)obj;
		
		assert(WOBJ_IS(obj, WWsSplit));
		
		split=(WWsSplit*)obj;
		
		if(split->dir==dir){
			obj=split->br;
			continue;
		}
		
		obj=(split->current==0 ? split->tl : split->br);
	}
	
	assert(0);
}


WWindow *find_current(WWorkspace *ws)
{
	return left_or_topmost_current(ws->splitree, -1);
}


static WWsSplit *find_split(WObj *obj, int dir, int *from)
{
	WWsSplit *split;

	if(WOBJ_IS(obj, WWindow))
		split=((WWindow*)obj)->split;
	else
		split=((WWsSplit*)obj)->parent;
	
	while(split!=NULL){
		if(split->dir==dir){
			if(obj==split->tl)
				*from=TOP_OR_LEFT;
			else
				*from=BOTTOM_OR_RIGHT;
			break;
		}
			
		obj=(WObj*)split;
		split=split->parent;
	}
	
	return split;
}


static WWindow *right_or_down(WWindow *wwin, int dir)
{
	WObj *prev=(WObj*)wwin;
	WWorkspace *ws;
	WWsSplit *split;
	int from;

	while(1){
		split=find_split(prev, dir, &from);

		if(split==NULL)
			break;
		
		if(from==TOP_OR_LEFT)
			return left_or_topmost_current(split->br, dir);
		
		prev=(WObj*)split;
	}
	
	return left_or_topmost_current(prev, dir);
}


static WWindow *up_or_left(WWindow *wwin, int dir)
{
	WObj *prev=(WObj*)wwin;
	WWorkspace *ws;
	WWsSplit *split;
	int from;

	while(1){
		split=find_split(prev, dir, &from);

		if(split==NULL)
			break;
		
		if(from==BOTTOM_OR_RIGHT)
			return right_or_bottomost_current(split->tl, dir);
		
		prev=(WObj*)split;
	}
	
	return right_or_bottomost_current(prev, dir);
}


static void goto_wwin(WWindow *wwin)
{
	if(wwin==NULL || wglobal.current_wswindow==wwin)
		return;
	set_previous((WThing*)wwin);
	warp(wwin);
}

void goto_above(WWindow *wwin)
{
	if(wwin!=NULL)
		goto_wwin(up_or_left(wwin, VERTICAL));
}

void goto_below(WWindow *wwin)
{
	if(wwin!=NULL)
		goto_wwin(right_or_down(wwin, VERTICAL));
}

void goto_left(WWindow *wwin)
{
	if(wwin!=NULL)
		goto_wwin(up_or_left(wwin, HORIZONTAL));
}


void goto_right(WWindow *wwin)
{
	if(wwin!=NULL)
		goto_wwin(right_or_down(wwin, HORIZONTAL));
}


/*}}}*/


/*{{{ Remove */


void workspace_remove_window(WWorkspace *ws, WWindow *wwin)
{
	WWsSplit *split;
	
	split=wwin->split;
	
	if(split==NULL){
		ws->splitree=NULL;
		return;
	}
	
	if(split->tl==(WObj*)wwin)
		split->tl=NULL;
	else
		split->br=NULL;
	
	remove_split(ws, split);
}


bool remove_split(WWorkspace *ws, WWsSplit *split)
{
	WWsSplit *split2;
	WObj *other;
	int osize, nsize, npos;
	int primn;

	if(split->tl==NULL){
		other=split->br;
		primn=TOP_OR_LEFT;
	}else{
		other=split->tl;
		primn=BOTTOM_OR_RIGHT;
	}
	
	split2=split->parent;

	if(split2!=NULL){
		if((WObj*)split==split2->tl)
			split2->tl=other;
		else
			split2->br=other;
	}else{
		ws->splitree=other;
	}
	
	if(other==NULL)
		return FALSE;
		
	if(WOBJ_IS(other, WWindow))
		((WWindow*)other)->split=split2;
	else
		((WWsSplit*)other)->parent=split2;
	
	if(wglobal.opmode!=OPMODE_DEINIT){
		nsize=tree_size((WObj*)split, split->dir);
		npos=tree_pos((WObj*)split, split->dir);
		nsize=tree_calcresize(other, split->dir, primn, nsize);
		tree_do_resize(other, split->dir, npos, nsize);
	}

	free(split);
	
	return TRUE;
}


/*}}}*/


/*{{{ Misc */


void set_current_wswindow(WWindow *wwin)
{
	WWsSplit *split=wwin->split;
	WObj *prev=(WObj*)wwin;
	
	while(split!=NULL){
		split->current=(split->tl==prev ? 0 : 1);
		prev=(WObj*)split;
		split=split->parent;
	}
	
	wglobal.current_wswindow=wwin;
}


/*}}}*/

