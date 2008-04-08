/*
 * ion/modules.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_MODULES_H
#define INCLUDED_MODULES_H

#include "common.h"
#include "thing.h"
#include "window.h"

extern bool load_module(const char *name);
extern void call_hooks(const char *hook);
extern void call_hooks_p(const char *hook, void *p);
extern void unload_modules();

extern void *miter_begin(const char *sym);
extern void *miter_next(const char *sym);
extern void miter_end();

typedef void Hook();
typedef bool AltHookB();
typedef void *AltHookP();


#define FOR_ALL_SYMBOLS(VAR, TYPE, SYM) \
	for(VAR=(TYPE)miter_begin(SYM);     \
		VAR!=NULL;                      \
		VAR=(TYPE)miter_next(SYM))


#define CALL_HOOKS(HOOK) call_hooks(#HOOK)
#define CALL_HOOKS_P(HOOK, P) call_hooks_p(#HOOK, P)


#define CALL_HOOKS_ARG(NAME, ARGS)                      \
{                                                       \
	Hook *hook_tmp_do_not_use;                          \
	FOR_ALL_SYMBOLS(hook_tmp_do_not_use, Hook*, #NAME){ \
		althook_tmp_do_not_use ARGS;                    \
	}                                                   \
	miter_end();                                        \
}


#define CALL_ALT_B_ARG(VAR, NAME, ARGS)                               \
{                                                                     \
	AltHookB *althook_tmp_do_not_use;                                 \
	VAR=FALSE;                                                        \
	FOR_ALL_SYMBOLS(althook_tmp_do_not_use, AltHookB*, #NAME "_alt"){ \
		VAR=althook_tmp_do_not_use ARGS;                              \
		if(VAR)                                                       \
			break;                                                    \
	}                                                                 \
	miter_end();                                                      \
	if(!VAR)                                                          \
		VAR=NAME ARGS;                                                \
}


#define CALL_ALT_P_ARG(VAR, NAME, ARGS)                               \
{                                                                     \
	AltHookP *althook_tmp_do_not_use;                                 \
	VAR=NULL;                                                         \
	FOR_ALL_SYMBOLS(althook_tmp_do_not_use, AltHookP*, #NAME "_alt"){ \
		VAR=althook_tmp_do_not_use ARGS;                              \
		if(VAR!=NULL)                                                 \
			break;                                                    \
	}                                                                 \
	miter_end();                                                      \
	if(VAR==NULL)                                                     \
		VAR=NAME ARGS;                                                \
}

#endif /* INCLUDED_MODULES_H */
