/*
 * ion/thing.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_THING_H
#define INCLUDED_THING_H

#include "common.h"

INTROBJ(WThing)

#define WTHING_DESTREMPTY	0x00001
#define WTHING_SUBDEST		0x10000

#define WTHING_CHILDREN(THING, TYPE) ((TYPE*)((WThing*)(THING))->t_children)
#define WTHING_PARENT(THING, TYPE) ((TYPE*)((WThing*)(THING))->t_parent)
#define WTHING_HAS_PARENT(THING, WT) \
	(((WThing*)(THING))->t_parent!=NULL && \
	 WTHING_IS(((WThing*)(THING))->t_parent, WT))


#define FIRST_THING(NAM, TYPE) (TYPE*)first_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define NEXT_THING(NAM, TYPE) (TYPE*)next_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define LAST_THING(NAM, TYPE) (TYPE*)last_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define PREV_THING(NAM, TYPE) (TYPE*)prev_thing((WThing*)NAM, &OBJDESCR(TYPE))
#define FIND_PARENT(NAM, TYPE) (TYPE*)find_parent((WThing*)NAM, &OBJDESCR(TYPE))
#define NTH_THING(NAM, N, TYPE) (TYPE*)nth_thing((WThing*)NAM, N, &OBJDESCR(TYPE))
#define FOR_ALL_TYPED(NAM, NAM2, TYPE) \
	for(NAM2=FIRST_THING(NAM, TYPE); NAM2!=NULL; NAM2=NEXT_THING(NAM2, TYPE))

#define WTHING_IS(X, Y) WOBJ_IS(X, Y)

extern void init_thing(WThing *thing);
extern void link_thing(WThing *parent, WThing *child);
/*extern void link_thing_before(WThing *before, WThing *child);*/
extern void link_thing_after(WThing *after, WThing *thing);
extern void unlink_thing(WThing *thing);
extern void destroy_subthings(WThing *thing);
extern void destroy_thing(WThing *thing);

extern WThing *next_thing(WThing *first, const WObjDescr *descr);
extern WThing *prev_thing(WThing *first, const WObjDescr *descr);
extern WThing *first_thing(WThing *parent, const WObjDescr *descr);
extern WThing *last_thing(WThing *parent, const WObjDescr *descr);
extern WThing *find_parent(WThing *p, const WObjDescr *descr);
extern WThing *nth_thing(WThing *parent, int n, const WObjDescr *descr);


DECLOBJ(WThing){
	WObj obj;
	int flags;
	WThing *t_parent, *t_children;
	WThing *t_next, *t_prev;
	void *screen;
};


#endif /* INCLUDED_THING_H */
