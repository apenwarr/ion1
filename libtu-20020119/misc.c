/*
 * libtu/misc.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libtu/misc.h>


void *malloczero(size_t size)
{
	void *p=malloc(size);
	
	if(p!=NULL)
		memset(p, 0, size);
	
	return p;
}


void *remalloczero(void *ptr, size_t oldsize, size_t newsize)
{
	void *p=NULL;
	
	if(newsize!=0){
		p=malloc(newsize);
	
		if(p==NULL)
			return NULL;
	
		memset(p, 0, newsize);
	
		if(newsize<oldsize)
			oldsize=newsize;
		
		if(ptr!=NULL)
			memcpy(p, ptr, oldsize);
	}
	
	if(ptr!=NULL)
		free(ptr);

	return p;
}


char *scopy(const char *p)
{
	char*pn;
	size_t l=strlen(p);
	
	pn=(char*)malloc(l+1);
	
	if(pn==NULL)
		return NULL;
	
	memcpy(pn, p, l+1);
	
	return pn;
}
	
	
char *scat(const char *p1, const char *p2)
{
	size_t l1, l2;
	char*pn;
	
	l1=strlen(p1);
	l2=strlen(p2);
	
	pn=(char*)malloc(l1+l2+1);
	
	if(pn==NULL)
		return NULL;
	
	memcpy(pn, p1, l1);
	memcpy(pn+l1, p2, l2+1);
	
	return pn;
}


char *scat3(const char *p1, const char *p2, const char *p3)
{
	size_t l1, l2, l3;
	char *pn;
	
	l1=strlen(p1);
	l2=strlen(p2);
	l3=strlen(p3);
	
	pn=(char*)malloc(l1+l2+l3+1);
	
	if(pn==NULL)
		return NULL;
	
	memcpy(pn, p1, l1);
	memcpy(pn+l1, p2, l2);
	memcpy(pn+l1+l2, p3, l3+1);
	
	return pn;
}


char *scatn(const char *s1, ssize_t l1, const char *s2, ssize_t l2)
{
	size_t tlen=1;
	char *s;
	
	if(l1<0)
		l1=strlen(s1);
	
	if(l2<0)
		l2=strlen(s2);
	
	tlen+=l1+l2;
	
	s=(char*)malloc(tlen);
	
	if(s==NULL)
		return NULL;
	
	memcpy(s, s1, l1);
	memcpy(s+l1, s2, l2);
	s[l1+l2]='\0';
	
	return s;
}


/* */


const char *simple_basename(const char *name)
{
	const char *p;
	
	p=name+strlen(name)-1;
	
	/* Skip any trailing slashes */
	while(*p=='/'){
		/* root? */
		if(p==name)
			return name;
		p--;
	}
	
	while(p!=name){
		if(*p=='/')
			return p+1;
		p--;
	}
	
	return name;
}


void stripws(char *p)
{
	int l;
	
	l=strspn(p, " ");
	if(l!=0)
		strcpy(p, p+l);
	l=strlen(p);
	
	while(--l>=0){
		if(*(p+l)!=' ')
			break;
	}
	*(p+l+1)='\0';
}


/* */


bool readf(FILE *f, void *buf, size_t n)
{
	return fread(buf, 1, n, f)!=1;
}


bool writef(FILE *f, const void *buf, size_t n)
{
	return fwrite(buf, 1, n, f)!=1;
}
