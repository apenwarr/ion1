/*
 * libtu/optparser.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <string.h>
#include <stdlib.h>

#include <libtu/util.h>
#include <libtu/misc.h>
#include <libtu/optparser.h>
#include <libtu/output.h>


#define O_ARGS(o)		(o->flags&OPT_OPT_ARG)
#define O_ARG(o)		(o->flasg&OPT_ARG)
#define O_OPT_ARG(o)	(O_ARGS(o)==OPT_OPT_ARG)
#define O_ID(o)			(o->optid)

#define OPT_ID_HELP OPT_ID_RESERVED('h')
#define OPT_ID_VERSION OPT_ID_RESERVED('V')
#define OPT_ID_ABOUT OPT_ID_RESERVED('a'|OPT_ID_NOSHORT_FLAG)


static OptParserOpt common_opts[]={
	{OPT_ID_HELP, "help", 0, NULL, DUMMY_TR("Show this  help")},
	{OPT_ID_VERSION, "version", 0, NULL, DUMMY_TR("Show program version")},
	{OPT_ID_ABOUT, "about", 0, NULL, DUMMY_TR("Show about text")},
	{0, NULL, 0, NULL, NULL}
};


static OptParserCommonInfo dummy_cinfo[]={
	NULL, /* version */
	"Usage: $p\n", /* usage_tmpl */
	NULL /* about */
};
	

static const OptParserOpt *o_opts=NULL;
static char *const *o_current=NULL;
static int o_left=0;
static const char* o_chain_ptr=NULL;
static int o_args_left=0;
static const char*o_tmp=NULL;
static int o_error=0;
static int o_mode=OPTP_CHAIN;
static const OptParserCommonInfo *o_cinfo=NULL;


/* */


static void print_help(const OptParserOpt *opts, bool midlong,
					   const OptParserCommonInfo *cinfo);


/* */


void optparser_init(int argc, char *const argv[], int mode,
					const OptParserOpt *opts, const OptParserCommonInfo *cinfo)
{
	o_mode=mode;
	o_opts=(opts==NULL ? common_opts : opts);
	o_current=argv+1;
	o_left=argc-1;
	o_chain_ptr=NULL;
	o_args_left=0;
	o_tmp=NULL;
	o_cinfo=(cinfo==NULL ? dummy_cinfo : cinfo);
}


/* */


static const OptParserOpt *find_chain_opt(char p, const OptParserOpt *o)
{
	for(;O_ID(o);o++){
		if((O_ID(o)&~OPT_ID_RESERVED_FLAG)==p)
			return o;
	}
	return NULL;
}


static bool is_option(const char *p)
{
	 if(p==NULL)
		  return FALSE;
	 if(*p++!='-')
		  return FALSE;
	 if(*p++!='-')
		  return FALSE;
	 if(*p=='\0')
		  return FALSE;
	 return TRUE;
}
	

/* */

enum{
	SHORT, MIDLONG, LONG
};


static int optparser_do_get_opt()
{
#define RET(X) return o_tmp=p, o_error=X
	const char *p, *p2=NULL;
	bool dash=TRUE;
	int type=SHORT;
	const OptParserOpt *o;
	int l;

	while(o_args_left)
		optparser_get_arg();

	o_tmp=NULL;

	/* Are we doing a chain (i.e. opt. of style 'tar xzf')? */
	if(o_chain_ptr!=NULL){
		p=o_chain_ptr++;
		if(!*o_chain_ptr)
			o_chain_ptr=NULL;

		o=find_chain_opt(*p, o_opts);
		
		if(o==NULL)
			RET(E_OPT_INVALID_CHAIN_OPTION);

		goto do_arg;
	}
	
	if(o_left<1)
		return OPT_ID_END;

	o_left--;
	p=*o_current++;
	
	if(*p!='-'){
		dash=FALSE;
		if(o_mode!=OPTP_NO_DASH)
			RET(OPT_ID_ARGUMENT);
		p2=p;
	}else if(*(p+1)=='-'){
		/* --foo */
		if(*(p+2)=='\0'){
			/* -- arguments */
			o_args_left=o_left;
			RET(OPT_ID_ARGUMENT);
		}
		type=LONG;
		p2=p+2;
	}else{
		/* -foo */
		if(*(p+1)=='\0'){
			/* - */
			o_args_left=1;
			RET(OPT_ID_ARGUMENT);
		}
		if(*(p+2)!='\0' && o_mode==OPTP_MIDLONG)
			type=MIDLONG;
	
		p2=p+1;
	}

	o=o_opts;

again:
	for(; O_ID(o); o++){
		if(type==LONG){
			/* Do long option (--foo=bar) */
			if(o->longopt==NULL)
				continue;
			l=strlen(o->longopt);
			if(strncmp(p2, o->longopt, l)!=0)
				continue;
			
			if(p2[l]=='\0'){
				if(O_ARGS(o)==OPT_ARG)
					 RET(E_OPT_MISSING_ARGUMENT);
				return O_ID(o);
			}else if(p2[l]=='='){
				if(!O_ARGS(o))
					 RET(E_OPT_UNEXPECTED_ARGUMENT);
				if(p2[l+1]=='\0')
					 RET(E_OPT_MISSING_ARGUMENT);
				o_tmp=p2+l+1;
				o_args_left=1;
				return O_ID(o);
			}
			continue;
		}else if(type==MIDLONG){
			if(o->longopt==NULL)
				continue;
			
			if(strcmp(p2, o->longopt)!=0)
				continue;
		}else{ /* type==SHORT */
			if(*p2!=(O_ID(o)&~OPT_ID_RESERVED_FLAG))
				continue;
			   
			if(*(p2+1)!='\0'){
				if(o_mode==OPTP_CHAIN || o_mode==OPTP_NO_DASH){
					/*valid_chain(p2+1, o_opts)*/
					o_chain_ptr=p2+1;
					p++;
				}else if(o_mode==OPTP_IMMEDIATE){
					if(!O_ARGS(o)){
						if(*(p2+1)!='\0')
							RET(E_OPT_UNEXPECTED_ARGUMENT);
					}else{
						if(*(p2+1)=='\0')
							RET(E_OPT_MISSING_ARGUMENT);
						o_tmp=p2+1;
						o_args_left=1;
					}
					return O_ID(o);
				}else{
					RET(E_OPT_SYNTAX_ERROR);
				}
			}
		}
		
	do_arg:
		
		if(!O_ARGS(o))
			return O_ID(o);
		
		if(!o_left || is_option(*o_current)){
			if(O_ARGS(o)==OPT_OPT_ARG)
				return O_ID(o);
			RET(E_OPT_MISSING_ARGUMENT);
		}

		o_args_left=1;
		return O_ID(o);
	}
	
	if(o!=&(common_opts[3])){
		o=common_opts;
		goto again;
	}

	if(dash)
		RET(E_OPT_INVALID_OPTION);
	
	RET(OPT_ID_ARGUMENT);
#undef RET
}


int optparser_get_opt()
{
	int oid=optparser_do_get_opt();
	
	if(oid<=0 || (oid&OPT_ID_RESERVED_FLAG)==0)
		return oid;
	
	switch(oid){
	case OPT_ID_ABOUT:
		if(o_cinfo->about!=NULL)
			printf("%s", o_cinfo->about);
		break;
		
	case OPT_ID_VERSION:
		if(o_cinfo->version!=NULL)
			printf("%s\n", o_cinfo->version);
		break;
			
	case OPT_ID_HELP:
		print_help(o_opts, o_mode==OPTP_MIDLONG, o_cinfo);
		break;
	}
	
	exit(EXIT_SUCCESS);
}


/* */


const char* optparser_get_arg()
{
	const char *p;
	
	if(o_tmp!=NULL){
		/* If o_args_left==0, then were returning an invalid option
		 * otherwise an immediate argument (e.g. -funsigned-char
		 * where '-f' is the option and 'unsigned-char' the argument)
		 */
		if(o_args_left>0)
			o_args_left--;
		p=o_tmp;
		o_tmp=NULL;
		return p;
	}
		
	if(o_args_left<1 || o_left<1)
		return NULL;

	o_left--;
	o_args_left--;
	return *o_current++;
}


/* */

static void warn_arg(const char *e)
{
	const char *p=optparser_get_arg();
	
	if(p==NULL)
		warn("%s (null)", e);
	else
		warn("%s \'%s\'", e, p);
}


static void warn_opt(const char *e)
{
	if(o_tmp!=NULL && o_chain_ptr!=NULL)
		warn("%s \'-%c\'", e, *o_tmp);
	else
		warn_arg(e);
}


void optparser_print_error()
{
	switch(o_error){
	case E_OPT_INVALID_OPTION:
	case E_OPT_INVALID_CHAIN_OPTION:
		warn_opt(TR("Invalid option"));
		break;

	case E_OPT_SYNTAX_ERROR:
		warn_arg(TR("Syntax error while parsing"));
		break;
				 
	case E_OPT_MISSING_ARGUMENT:
		warn_opt(TR("Missing argument to"));
		break;

	case E_OPT_UNEXPECTED_ARGUMENT:
		warn_opt(TR("No argument expected:"));
		break;
		
	case OPT_ID_ARGUMENT:
		warn(TR("Unexpected argument"));
		break;

	default:
		warn(TR("(unknown error)"));
	}
	
	o_tmp=NULL;
	o_error=0;
}


/* */


static uint opt_w(const OptParserOpt *opt, bool midlong)
{
	uint w=0;
	
	if((opt->optid&OPT_ID_NOSHORT_FLAG)==0){
		w+=2; /* "-o" */
		if(opt->longopt!=NULL)
			w+=2; /* ", " */
	}
	
	if(opt->longopt!=NULL)
		w+=strlen(opt->longopt)+(midlong ? 1 : 2);
	
	if(O_ARGS(opt)){
		if(opt->argname==NULL)
			w+=4;
		else
			w+=1+strlen(opt->argname); /* "=ARG" or " ARG" */
		
		if(O_OPT_ARG(opt))
			w+=2; /* [ARG] */
	}
	
	return w;
}


#define TERM_W 80
#define OFF1 2
#define OFF2 2
#define SPACER1 "  "
#define SPACER2 "  "

static void print_opt(const OptParserOpt *opt, bool midlong,
					  uint maxw, uint tw)
{
	FILE *f=stdout;
	const char *p, *p2, *p3;
	uint w=0;
	
	fprintf(f, SPACER1);
	
	/* short opt */
	
	if((O_ID(opt)&OPT_ID_NOSHORT_FLAG)==0){
		fprintf(f, "-%c", O_ID(opt)&~OPT_ID_RESERVED_FLAG);
		w+=2;
		
		if(opt->longopt!=NULL){
			fprintf(f, ", ");
			w+=2;
		}
	}
	
	/* long opt */
	
	if(opt->longopt!=NULL){
		if(midlong){
			w++;
			fprintf(f, "-%s", opt->longopt);
		}else{
			w+=2;
			fprintf(f, "--%s", opt->longopt);
		}
		w+=strlen(opt->longopt);
	}
	
	/* arg */
	
	if(O_ARGS(opt)){
		w++;
		if(opt->longopt!=NULL && !midlong)
			putc('=', f);
		else
			putc(' ', f);
		
		if(O_OPT_ARG(opt)){
			w+=2;
			putc('[', f);
		}
		
		if(opt->argname!=NULL){
			fprintf(f, "%s", opt->argname);
			w+=strlen(opt->argname);
		}else{
			w+=3;
			fprintf(f, "ARG");
		}

		if(O_OPT_ARG(opt))
			putc(']', f);
	}
	
	while(w++<maxw)
		putc(' ', f);
	
	/* descr */
	
	p=p2=opt->descr;
	
	if(p==NULL){
		putc('\n', f);
		return;
	}
	
	fprintf(f, SPACER2);

	maxw+=OFF1+OFF2;
	tw-=maxw;
	
	while(strlen(p)>tw){
		p3=p2=p+tw-2;
		
		while(*p2!=' ' && p2!=p)
			p2--;
		
		while(*p3!=' ' && *p3!='\0')
			p3++;
		
		if((uint)(p3-p2)>tw){
			/* long word - just wrap */
			p2=p+tw-2;
		}
			
		writef(f, p, p2-p);
		if(*p2==' ')
			putc('\n', f);
		else
			fprintf(f, "\\\n");
		
		p=p2+1;
		
		w=maxw;
		while(w--)
			putc(' ', f);
	}
	
	fprintf(f, "%s\n", p);
}

					  
static void print_opts(const OptParserOpt *opts, bool midlong,
					   const OptParserCommonInfo *cinfo)
{
	uint w, maxw=0;
	const OptParserOpt *o;
	
	o=opts;
again:
	for(; O_ID(o); o++){
		w=opt_w(o, midlong);
		if(w>maxw)
			maxw=w;
	}
	
	if(o!=&(common_opts[3])){
		o=common_opts;
		goto again;
	}

	o=opts;
again2:
	for(; O_ID(o); o++)
		print_opt(o, midlong, maxw, TERM_W);
	
	if(o!=&(common_opts[3])){
		printf("\n");
		o=common_opts;
		goto again2;
	}
}


static void print_help(const OptParserOpt *opts, bool midlong,
					   const OptParserCommonInfo *cinfo)
{
	const char *tmp, *p=cinfo->usage_tmpl;
	size_t len;
	size_t start;
	
	while(1){
		tmp=strchr(p, '$');

		if(tmp==NULL){
			writef(stdout, p, strlen(p));
			return;
		}
		
		if(tmp!=p)
			writef(stdout, p, tmp-p);
		
		p=tmp+1;
		
		if(*p=='p'){
			tmp=prog_execname();
			writef(stdout, tmp, strlen(tmp));
		}else if(*p=='o'){
			print_opts(opts, midlong, cinfo);
		}
		p++;
	}
}

