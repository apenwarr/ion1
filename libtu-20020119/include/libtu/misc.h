/*
 * libtu/misc.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef LIBTU_MISC_H
#define LIBTU_MISC_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "types.h"

#define TR(X) X
#define DUMMY_TR(X) X

#define ALLOC(X) (X*)malloczero(sizeof(X))
#define ALLOC_N(X, N) (X*)malloczero(sizeof(X)*(N))
#define REALLOC_N(PTR, X, S, N) (X*)remalloczero(PTR, sizeof(X)*(S), sizeof(X)*(N))

#define FREE(X) do{if(X!=NULL)free(X);}while(0)

extern void* malloczero(size_t size);
extern void* remalloczero(void *ptr, size_t oldsize, size_t newsize);

extern char* scopy(const char *p);
extern char* scat(const char *p1, const char *p2);
extern char* scatn(const char *p1, ssize_t n1, const char *p2, ssize_t n2);
extern char* scat3(const char *p1, const char *p2, const char *p3);
extern void stripws(char *p);

extern const char* simple_basename(const char *name);

/* I dislike fread and fwrite... */
extern bool readf(FILE *fd, void *buf, size_t n);
extern bool writef(FILE *fd, const void *buf, size_t n);

#endif /* LIBTU_MISC_H */
