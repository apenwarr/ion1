/*
 * ion/function.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_FUNCTION_H
#define INCLUDED_FUNCTION_H

#include "common.h"
#include <libtu/tokenizer.h>

INTRSTRUCT(WFunction)

#include "thing.h"


enum{
	FUNTAB_MAIN=0,
	FUNTAB_MOVERES=1,
	FUNTAB_INPUT=2
};

	
typedef void WFuncHandler(WThing *thing, WFunction *func,
						  int n, const Token *args);


DECLSTRUCT(WFunction){
	WFuncHandler *callhnd;
	WObjDescr *objdescr;
	char *argtypes;
	char *name;
	void *fn;
};


extern WFunction *lookup_func(const char *name, int funtab);
extern int complete_mainfunc(char *nam, char ***cp_ret, char **beg);

#endif /* INCLUDED_FUNCTION_H */
