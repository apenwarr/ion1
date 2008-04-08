/*
 * ion/modules.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef CF_NO_MODULE_SUPPORT

#include <dlfcn.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "modules.h"


INTRSTRUCT(Module)
	
DECLSTRUCT(Module){
	void *handle;
	char *name;
	Module *next;
};

static Module *modules=NULL, *last_module=NULL;
static size_t longest_name=0;


/*{{{ Loading and initialization code */


static bool call_init(void *handle, char *name)
{
	char *p=scat(name, "_init");
	bool (*initfn)(void);

	if(p==NULL){
		warn_err();
		return FALSE;
	}
	
	initfn=(bool (*)())dlsym(handle, p);
	
	free(p);
	
	if(initfn==NULL)
		return TRUE;
	
	return initfn();
}


bool load_module(const char *fname)
{
	void *handle;
	Module *m;
	const char *p;
	char *n;
	size_t l;
	
	handle=dlopen(fname, RTLD_LAZY);
	
	if(handle==NULL){
		warn_obj(fname, "%s", dlerror());
		return FALSE;
	}

	/* Get the module name without directory or extension */
	
	p=strrchr(fname, '/');
	
	if(p!=NULL)
		fname=p+1;
	
	for(p=fname; *p!='\0'; p++){
		if(!isalnum(*p) && *p!='_')
			break;
	}
	
	n=ALLOC_N(char, p-fname);
	
	if(n==NULL){
		warn_err();
		goto err1;
	}
	 
	memcpy(n, fname, p-fname);
	n[p-fname]='\0';
	
	if(!call_init(handle, n))
		goto err2;
	
	/* Allocate space for module info */
	
	m=ALLOC(Module);
	
	if(m==NULL){
		warn_err();
		goto err2;
	}
	
	m->name=n;
	m->handle=handle;
	m->next=NULL;
	
	if(last_module==NULL){
		modules=last_module=m;
	}else{
		last_module->next=m;
		last_module=m;
	}
	
	l=strlen(n);
	
	if(l>longest_name)
		longest_name=l;
	
	return TRUE;
	
err2:
	free(n);
err1:
	dlclose(handle);
	return FALSE;
}


/*}}}*/


/*{{{ Deinit */


static void call_deinit(void *handle, char *name)
{
	char *p=scat(name, "_deinit");
	void (*deinitfn)(void);

	if(p==NULL){
		warn_err();
		return;
	}
	
	deinitfn=(void (*)())dlsym(handle, p);
	
	free(p);
	
	if(deinitfn!=NULL)
		deinitfn();
}


static void do_unload_module(Module *m)
{
	call_deinit(m->handle, m->name);

	dlclose(m->handle);
	if(m->name!=NULL)
		free(m->name);
	free(m);
}


void unload_modules()
{
	Module *m, *next;
	
	for(m=modules; m!=NULL; m=next){
		next=m->next;
		do_unload_module(m);
	}
	modules=NULL;
	last_module=NULL;
	longest_name=0;
}


/*}}}*/


/*{{{ Hooks and iterative symbol lookup */


static char *symtmp=NULL;
static Module *cmod=NULL;


static void *get_sym(const Module *m, const char *sym)
{
	sprintf(symtmp, "%s_%s", m->name, sym);
	return dlsym(m->handle, symtmp);
}


void *miter_begin(const char *sym)
{
	cmod=NULL;
	
	if(modules==NULL)
		return NULL;
	
	symtmp=ALLOC_N(char, strlen(sym)+longest_name+1+1);
	
	if(symtmp==NULL){
		warn_err();
		return NULL;
	}
	
	cmod=modules;
	
	return miter_next(sym);
}


void *miter_next(const char *sym)
{
	void *r;
	
	do{
		if(cmod==NULL)
			return NULL;
	
		r=get_sym(cmod, sym);
		cmod=cmod->next;	
	}while(r==NULL);
	
	return r;
}


void miter_end()
{
	if(symtmp!=NULL){
		free(symtmp);
		symtmp=NULL;
	}
}


void call_hooks(const char *hook)
{
	void (*fn)();
	
	for(fn=(void (*)())miter_begin(hook);
		fn!=NULL;
		fn=(void (*)())miter_next(hook)){
		
		fn();
	}
	
	miter_end();
}


void call_hooks_p(const char *hook, void *p)
{
	void (*fn)(void *);
	
	for(fn=(void (*)(void *))miter_begin(hook);
		fn!=NULL;
		fn=(void (*)(void *))miter_next(hook)){
		
		fn(p);
	}
	
	miter_end();
}


/*}}}*/


#else /* CF_MODULE_SUPPORT */


#include "common.h"


/*{{{ Dummy functions for systems without sufficient dynamic
 * linking support
 */

bool load_module(const char *name)
{
	warn_obj(name, "Unabled to load: module support not enabled.");
	return FALSE;
}


void unload_modules()
{
	
}


void call_hooks(const char *hook)
{
	
}


void call_hooks_p(const char *hook, void *p)
{
	
}


void *miter_begin(const char *sym)
{
	return NULL;
}


void *miter_next(const char *sym)
{
	return NULL;
}


void miter_end()
{
	
}


/*}}}*/


#endif
