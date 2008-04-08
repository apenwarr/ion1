/*
 * ion/objp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_WOBJP_H
#define INCLUDED_WOBJP_H

#include "obj.h"

DECLSTRUCT(WObjDescr){
	const char *name;
	WObjDescr *ancestor;
	void *funtab;
};

#define IMPLOBJ(OBJ, ANCESTOR, FUNTAB)                            \
	WObjDescr OBJDESCR(OBJ)={#OBJ, &OBJDESCR(ANCESTOR), (void*)FUNTAB};

#define WOBJ_INIT(O, TYPE) ((WObj*)(O))->obj_type=&OBJDESCR(TYPE)

#define CREATESTRUCT_IMPL_(OBJ, LOWOBJ, INIT, ARGS)                       \
	OBJ *p;  p=ALLOC(OBJ); if(p==NULL){ warn_err(); return NULL; }        \
	INIT;                                                                 \
	if(!init_##LOWOBJ ARGS){ free((void*)p); return NULL; } return p

#define CREATESTRUCT_IMPL(OBJ, LOWOBJ, ARGS) \
	CREATESTRUCT_IMPL_(OBJ, LOWOBJ, /* */, ARGS)

#define SIMPLECREATESTRUCT_IMPL(OBJ, LOWOBJ, ARGS) \
	CREATESTRUCT_IMPL_(OBJ, LOWOBJ, /* */, (p))

#define CREATEOBJ_IMPL(OBJ, LOWOBJ, ARGS) \
	CREATESTRUCT_IMPL_(OBJ, LOWOBJ, WOBJ_INIT(p, OBJ), ARGS)

#define SIMPLECREATEOBJ_IMPL(OBJ, LOWOBJ, ARGS) \
	CREATESTRUCT_IMPL_(OBJ, LOWOBJ, WOBJ_INIT(p, OBJ), (p))

#define FUNTAB(OBJ, TYPE) ((TYPE*)(((WObj*)OBJ)->obj_type->funtab))
#define CALL_FUNTAB(OBJ, TYPE, FN) if(FUNTAB(OBJ, TYPE)->FN!=NULL) FUNTAB(OBJ, TYPE)->FN

#endif /* INCLUDED_WOBJP_H */
