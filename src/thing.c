/*
 * ion/thing.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "thing.h"
#include "thingp.h"
#include "global.h"
#include "client.h"
#include "wedln.h"
#include "query.h"
#include "grab.h"


IMPLOBJ(WThing, WObj, NULL)


/*{{{ Destroy */


static void do_destroy_thing(WThing *t)
{
	WThing *p;
	
	p=t->t_parent;

	/* Make sure that any command sequence doesn't try to
	 * access this thing anymore.
	 */
	query_check_function_thing(t);
	
	destroy_subthings(t);

	if(p!=NULL)
		CALL_FUNTAB(p, WThingFuntab, remove_child_fn)(p, t);

	grab_remove_for(t);
	
	unlink_thing(t);
	
	CALL_FUNTAB(t, WThingFuntab, deinit_fn)(t);
	
	free(t);
	
	if(wglobal.focus_next==t)
		wglobal.focus_next=NULL;
	if(wglobal.grab_holder==t)
		wglobal.grab_holder=NULL;
	if(wglobal.previous==t)
		wglobal.previous=p;
}


void destroy_thing(WThing *t)
{
	WThing *p;
	
	while(1){
		p=t->t_parent;
		do_destroy_thing(t);
		
		if(p==NULL)
			break;
		
		if(!(p->flags&WTHING_DESTREMPTY) || p->t_children!=NULL)
			break;
		
		t=p;
	}
}


void destroy_subthings(WThing *parent)
{
	WThing *t, *prev=NULL;

	assert(!(parent->flags&WTHING_SUBDEST));
	
	parent->flags|=WTHING_SUBDEST;
	
	/* destroy children */
	while(1){
		t=parent->t_children;
		if(t==NULL)
			break;
		assert(t!=prev);
		prev=t;
		do_destroy_thing(t);
	}
	
	parent->flags&=~WTHING_SUBDEST;
}


/*}}}*/


/*{{{ Linking */


void link_thing(WThing *parent, WThing *thing)
{
	LINK_ITEM(parent->t_children, thing, t_next, t_prev);
	thing->t_parent=parent;
}


/*void link_thing_before(WThing *before, WThing *thing)
{
	WThing *parent=before->t_parent;
	LINK_ITEM_BEFORE(parent->t_children, before, thing, t_next, t_prev);
	thing->t_parent=parent;
}*/


void link_thing_after(WThing *after, WThing *thing)
{
	WThing *parent=after->t_parent;
	LINK_ITEM_AFTER(parent->t_children, after, thing, t_next, t_prev);
	thing->t_parent=parent;
}


void unlink_thing(WThing *thing)
{
	WThing *parent=thing->t_parent;

	if(parent==NULL)
		return;
	
	UNLINK_ITEM(parent->t_children, thing, t_next, t_prev);
	thing->t_parent=NULL;
}


/*}}}*/


/*{{{ Scan */


static WThing *get_next_thing(WThing *first, const WObjDescr *descr)
{
	while(first!=NULL){
		if(wobj_is((WObj*)first, descr))
			break;
		first=first->t_next;
	}
	
	return first;
}


static WThing *get_prev_thing(WThing *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	while(1){
		first=first->t_prev;
		if(first->t_next==NULL)
			return NULL;
		if(wobj_is((WObj*)first, descr))
			break;
	}
	
	return first;
}


WThing *next_thing(WThing *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	return get_next_thing(first->t_next, descr);
}


WThing *prev_thing(WThing *first, const WObjDescr *descr)
{
	if(first==NULL)
		return NULL;
	
	return get_prev_thing(first, descr);
}


WThing *first_thing(WThing *parent, const WObjDescr *descr)
{
	if(parent==NULL)
		return NULL;
	
	return get_next_thing(parent->t_children, descr);
}


WThing *last_thing(WThing *parent, const WObjDescr *descr)
{
	WThing *p;
	
	if(parent==NULL)
		return NULL;
	
	p=parent->t_children;
	
	if(p==NULL)
		return NULL;
	
	p=p->t_prev;
	
	if(wobj_is((WObj*)p, descr))
		return p;
	
	return get_prev_thing(p, descr);
}


WThing *find_parent(WThing *p, const WObjDescr *descr)
{
	while(p!=NULL){
		if(wobj_is((WObj*)p, descr))
			break;
		p=p->t_parent;
	}
	
	return p;
}


WThing *nth_thing(WThing *parent, int n, const WObjDescr *descr)
{
	WThing *p;
	
	if(n<0)
		return NULL;
	
	p=first_thing(parent, descr);
	   
	while(n-- && p!=NULL)
		p=next_thing(p, descr);

	return p;
}
