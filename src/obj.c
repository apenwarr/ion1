/*
 * ion/obj.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include "common.h"
#include "obj.h"
#include "objp.h"


WObjDescr OBJDESCR(WObj)={
	"WObj",
	NULL,
	NULL
};


bool wobj_is(const WObj *obj, const WObjDescr *descr)
{
	WObjDescr *d=obj->obj_type;
	
	while(d!=NULL){
		if(d==descr)
			return TRUE;
		d=d->ancestor;
	}
	return FALSE;
}


const void *wobj_cast(const WObj *obj, const WObjDescr *descr)
{
	WObjDescr *d=obj->obj_type;
	
	while(d!=NULL){
		if(d==descr)
			return (void*)obj;
		d=d->ancestor;
	}
	return NULL;
}

