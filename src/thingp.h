/*
 * ion/thingp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_THINGP_H
#define INCLUDED_THINGP_H

#include "common.h"
#include "thing.h"
#include "screen.h"
#include "workspace.h"
#include "frame.h"
#include "client.h"
#include "clientwin.h"
#include "objp.h"

typedef void WThingDeinitFn(WThing*);
typedef void WThingRemoveChildFn(WThing*, WThing*);

INTRSTRUCT(WThingFuntab)

DECLSTRUCT(WThingFuntab){
	void (*deinit_fn)();
	void (*remove_child_fn)();
};


#define CREATETHING_IMPL(OBJ, LOWOBJ, PARENT, INIT_ARGS)                  \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; }        \
	WTHING_INIT(p, ((WThing*)(PARENT))->screen, OBJ);                     \
	if(!init_##LOWOBJ INIT_ARGS){ free((void*)p); return NULL; } return p

#define WTHING_INIT(P, SCR, TYPE)    \
	WOBJ_INIT(P, TYPE);              \
	((WThing*)(P))->flags=0;         \
	((WThing*)(P))->t_parent=NULL;   \
	((WThing*)(P))->t_children=NULL; \
	((WThing*)(P))->t_next=NULL;     \
	((WThing*)(P))->t_prev=NULL;     \
	((WThing*)(P))->screen=SCR

#endif /* INCLUDED_THINGP_H */
