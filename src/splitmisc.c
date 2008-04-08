/*
 * ion/splitmisc.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "splitmisc.h"
#include "split.h"


static WObj *do_find_at(WObj *obj, int x, int y)
{
	WWsSplit *split;
	WWindow *wwin;
	WRectangle geom;
	
	if(!WTHING_IS(obj, WWsSplit)){
		if(!WTHING_IS(obj, WFrame))
			return NULL;
		wwin=(WWindow*)obj;
		if(x<wwin->geom.x || x>=(wwin->geom.x+wwin->geom.w) ||
		   y<wwin->geom.y || y>=(wwin->geom.y+wwin->geom.h))
			return NULL;
		return obj;
	}
	
	split=(WWsSplit*)obj;
	
	if(x<split->geom.x || x>=(split->geom.x+split->geom.w) ||
	   y<split->geom.y || y>=(split->geom.y+split->geom.h))
		return NULL;
	
	obj=do_find_at(split->tl, x, y);
	if(obj==NULL)
		obj=do_find_at(split->br, x, y);
	return obj;
}


WFrame *find_frame_at(WWorkspace *ws, int x, int y)
{
	WObj *obj=ws->splitree;
	
	obj=do_find_at(obj, x, y);
	
	return (WFrame*)obj;
}


WFrame *do_find_frame(WObj *ptr, int primn)
{
	WFrame *frame;
	WWsSplit *split;
	
	do{
		if(WOBJ_IS(ptr, WFrame))
			return (WFrame*)ptr;
		
		if(!WOBJ_IS(ptr, WWsSplit))
			return NULL;
		
		split=(WWsSplit*)ptr;
		
		if(primn==TOP_OR_LEFT)
			frame=do_find_frame(split->tl, primn);
		else
			frame=do_find_frame(split->br, primn);
		
		if(frame!=NULL)
			return frame;
		
		if(primn==TOP_OR_LEFT)
			ptr=split->br;
		else
			ptr=split->tl;
	}while(1);
}
					  

WFrame *find_sister_frame(WFrame *frame)
{
	WWsSplit *split=frame->win.split;
	
	while(split!=NULL){
		if(split->tl==(WObj*)frame)
			frame=do_find_frame(split->br, TOP_OR_LEFT);
		else
			frame=do_find_frame(split->tl, BOTTOM_OR_RIGHT);
		
		if(frame!=NULL)
			return frame;
		
		split=split->parent;
	}

	return NULL;
}

