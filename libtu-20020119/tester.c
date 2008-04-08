/*
 * libtu/tester.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#include <stdio.h>

#include <libtu/misc.h>
#include <libtu/tokenizer.h>
#include <libtu/util.h>

int main(int argc, char *argv[])
{
	Tokenizer*tokz;
	Token tok=TOK_INIT;
	
	libtu_init(argv[0]);
	
	if(!(tokz=tokz_open_file(stdin, "stdin")))
		return EXIT_FAILURE;

	while(tokz_get_token(tokz, &tok)){
		switch(tok.type){
		case TOK_LONG:
			printf("long - %ld\n", TOK_LONG_VAL(&tok));
			break;
		case TOK_DOUBLE:
			printf("double - %g\n", TOK_DOUBLE_VAL(&tok));
			break;
		case TOK_CHAR:
			printf("char - '%c'\n", TOK_CHAR_VAL(&tok));
			break;
		case TOK_STRING:
			printf("string - \"%s\"\n", TOK_STRING_VAL(&tok));
			break;
		case TOK_IDENT:
			printf("ident - %s\n", TOK_IDENT_VAL(&tok));
			break;
		case TOK_COMMENT:
			printf("comment - %s\n", TOK_COMMENT_VAL(&tok));
			break;
		case TOK_OP:
			printf("operator - %03x\n", TOK_OP_VAL(&tok));
			break;
		}
	}
	   
	return EXIT_SUCCESS;
}

