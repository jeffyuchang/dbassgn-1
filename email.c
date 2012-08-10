/*
 * src/tutorial/complex.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


PG_MODULE_MAGIC;

typedef struct Email
{
	char		local[128];
	char		domain[128];
}	Email;

////////////////////////////////
int email_grammar_checker(char *string);// check email syntax
int	email_local_domain_divider(char *str,char *slocal,char *sdomain); //email divider
int	email_local_domain_divider_lowercase(char *str,char *slocal,char *sdomain); //email divider and convert string into lowercase
//
int func_is_email_equal(Email *t1,Email *t2);//(=) if two values are equivalent
int func_is_email_not_equal(Email *t1,Email *t2);//(<>) if two values are not equivalent
int func_is_email_greater_than(Email *t1,Email *t2);//(>) the first is greater than the second
int func_is_email_greater_equal(Email *t1,Email *t2);//(>=) the first is greater than the second or equivalent
int func_is_email_less_than(Email *t1,Email *t2);//(<) the first is greater than the second
int func_is_email_less_equal(Email *t1,Email *t2);//(<=) the first is greater than the second or equivalent
int func_is_email_same_domain(Email *t1, Email *t2);//(~) the same domain
int func_is_email_not_same_domain(Email *t1, Email *t2);//(!~) not the same domain
//
int strcpy_to_capitalletters(char *tar,char *src);//copy string and convert string into capital letters
int strcpy_to_lowercase(char *tar,char *src);//copy string and convert string into lowercase
int strcpy_to(char *tar,char *src);//just copy string
//

#define	RET_VALID	1
#define	RET_INVALID	0
#define	IS_TRUE	1
#define	IS_FALSE	0
#define	SW_PASS	1
#define	SW_FAIL	0
/////////////////////////////////


/*
 * Since we use V1 function calling convention, all these functions have
 * the same signature as far as C is concerned.  We provide these prototypes
 * just to forestall warnings when compiled with gcc -Wmissing-prototypes.
 */
Datum		email_in(PG_FUNCTION_ARGS);
Datum		email_out(PG_FUNCTION_ARGS);
Datum		email_recv(PG_FUNCTION_ARGS);
Datum		email_send(PG_FUNCTION_ARGS);

Datum		email_eq(PG_FUNCTION_ARGS);
Datum		email_neq(PG_FUNCTION_ARGS);
Datum		email_gt(PG_FUNCTION_ARGS);
Datum		email_sd(PG_FUNCTION_ARGS);
Datum		email_nsd(PG_FUNCTION_ARGS);
Datum		email_ge(PG_FUNCTION_ARGS);
Datum		email_lt(PG_FUNCTION_ARGS);
Datum		email_le(PG_FUNCTION_ARGS);

Datum		email_cmp(PG_FUNCTION_ARGS);


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(email_in);

Datum
email_in(PG_FUNCTION_ARGS)
{
	char	   *str = PG_GETARG_CSTRING(0);
	Email    *result;
	int ret;

	ret=email_grammar_checker(str);
	if(ret==IS_FALSE)
		errmsg("Invalid input syntax for EmailAddress: \"%s\"",str);

	result = (Email *) palloc(sizeof(Email));
	ret=email_local_domain_divider_lowercase(str,result->local,result->domain);	
	if(ret==IS_FALSE)
		errmsg("Failed to divide \"%s\" into two parts",str);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(email_out);

Datum
email_out(PG_FUNCTION_ARGS)
{
	Email    *email = (Email *) PG_GETARG_POINTER(0);
	char	   *result;

	result = (char *) palloc(264);
	sprintf(result,"%s@%s",email->local,email->domain);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/
#define	_YS_USE_BYTES	//This definition allows functions to send and receive data as bytes.

PG_FUNCTION_INFO_V1(email_recv);

Datum
email_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	Email    *result;
	char *pLocal,*pDomain;
	/*
	FILE *fp;

	result = (Email *) palloc(sizeof(Email));
	fp=fopen("/import/ravel/1/ysso644/srvr/postgresql-9.1.4/src/tutorial/haha.txt","rb+");
	if(fp)
	{
		fseek(fp,0,SEEK_END);
		fprintf(fp,"called email_recv\r\n");
		fclose(fp);
	}	
	*/
	
/*
#ifndef	_YS_USE_BYTES
	pLocal=pg_getmsgstring(buf);
	pDomain=pg_getmsgstring(buf);
#else
	pLocal=pg_getmsgbytes(buf,128);
	pDomain=pg_getmsgbytes(buf,128);
#endif
	strcpy_to(result->local,pLocal);
	strcpy_to(result->domain,pDomain);
*/
	
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(email_send);

Datum
email_send(PG_FUNCTION_ARGS)
{
	Email    *email = (Email *) PG_GETARG_POINTER(0);
	StringInfoData buf;
//	FILE *fp;
	
	pq_begintypsend(&buf);
/*
	fp=fopen("/import/ravel/1/ysso644/srvr/postgresql-9.1.4/src/tutorial/haha.txt","rb+");
	if(fp)
	{
		fseek(fp,0,SEEK_END);
		fprintf(fp,"called email_recv\r\n");
		fclose(fp);
	}	
	*/
	
/*
#ifndef	_YS_USE_BYTES
	pg_sendstring(&buf,email->local);
	pg_sendstring(&buf,email->domain);
#else
	pg_sendbytes(&buf,email->local,128);
	pg_sendbytes(&buf,email->domain,128);
#endif 
*/

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*****************************************************************************
 * New Operators
 *
 *****************************************************************************/

PG_FUNCTION_INFO_V1(email_eq);

Datum
email_eq(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);
	PG_RETURN_BOOL(is_email_eq(a, b));
}

int is_email_eq(Email *e1, Email *e2) {
    if ((strcmp(e1->local, e2->local) == 0) 
        && (strcmp(e1->domain, e2->domain) == 0)) {
	    return IS_TRUE;
	}
	return IS_FALSE;	
}

PG_FUNCTION_INFO_V1(email_neq);

Datum
email_neq(PG_FUNCTION_ARGS)
{
	Email   *a = (Email *) PG_GETARG_POINTER(0);
	Email   *b = (Email *) PG_GETARG_POINTER(1);
	PG_RETURN_BOOL(!is_email_eq(a,b));
}

PG_FUNCTION_INFO_V1(email_gt);

Datum
email_gt(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);
	PG_RETURN_BOOL(is_email_gt(a, b));
}

int is_email_gt(Email *e1, Email *e2) {
	if (strcmp(e1->domain,e2->domain) > 0) {
		return IS_TRUE;
	} else if (strcmp(e1->domain, e2->domain) == 0 && strcmp(e1->local, e2->local) > 0) {
		return IS_TRUE;
	}
	return IS_FALSE;	
}

PG_FUNCTION_INFO_V1(email_sd);

Datum
email_sd(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);
		
	PG_RETURN_BOOL(strcmp(a->domain,b->domain) == 0);
}

PG_FUNCTION_INFO_V1(email_nsd);

Datum
email_nsd(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(strcmp(a->domain, b->domain) != 0);
}

PG_FUNCTION_INFO_V1(email_ge);

Datum
email_ge(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);

	PG_RETURN_BOOL(!is_email_lt(a,b));
}

PG_FUNCTION_INFO_V1(email_lt);

Datum
email_lt(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);
	PG_RETURN_BOOL(is_email_lt(a,b));
}

int is_email_lt(Email *e1, Email *e2){
	if (strcmp(e1->domain, e2->domain) < 0) {
		return true;
	} else if (strcmp(e1->domain, e2->domain) == 0 && strcmp(e1->local, e2->local) < 0) {
		return true;
	}
	return false;
}


PG_FUNCTION_INFO_V1(email_le);

Datum
email_le(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);
	PG_RETURN_BOOL(!is_email_gt(a,b));
}

PG_FUNCTION_INFO_V1(email_cmp);

Datum
email_cmp(PG_FUNCTION_ARGS)
{
	Email    *a = (Email *) PG_GETARG_POINTER(0);
	Email    *b = (Email *) PG_GETARG_POINTER(1);

//	PG_RETURN_INT32(complex_abs_cmp_internal(a, b));
	PG_RETURN_INT32(1);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int strcpy_to(char *tar,char *src)
{
	char *ptar=tar;

	while(*src)
	{
		*tar=*src;
		src++;
		tar++;
	}
	*tar='\0';
	tar++;
	return (int)(tar-ptar);
}

int strcpy_to_capitalletters(char *tar,char *src)
{
	char *ptar=tar;

	while(*src)
	{
		if(*src>='a' && *src<='z')
			*tar='A'+(*src-'a');
		else *tar=*src;
		src++;
		tar++;
	}
	*tar='\0';
	tar++;
	return (int)(tar-ptar);
}

int strcpy_to_lowercase(char *tar,char *src)
{
	char *ptar=tar;

	while(*src)
	{
		if(*src>='A' && *src<='Z')
			*tar='a'+(*src-'A');
		else *tar=*src;
		src++;
		tar++;
	}
	*tar='\0';
	tar++;
	return (int)(tar-ptar);
}

static int is_two_words_equal(char *t1,char *t2)
{
	while(*t1)
	{
		if(!(*t1==*t2))
			return IS_FALSE;
		t2++;
		t1++;
	}
	return IS_TRUE;
}

int	email_local_domain_divider(char *str,char *slocal,char *sdomain) //email divider
{
	while(*str)
	{
		if(*str=='@')
			break;
		*slocal++=*str++;
	}

	*slocal='\0';
	if(*str=='\0')
		return IS_FALSE;
	str++;
	while(*str)
	{
		*sdomain++=*str++;
	}
	*sdomain='\0';
	return IS_TRUE;
}

int	email_local_domain_divider_lowercase(char *str,char *slocal,char *sdomain) //email divider
{
	while(*str)
	{
		if(*str=='@')
			break;
		*slocal++=*str++;
	}

	*slocal='\0';
	if(*str=='\0')
		return IS_FALSE;
	str++;
	while(*str)
	{
		*sdomain++=*str++;
	}
	*sdomain='\0';
	return IS_TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////

int func_is_email_equal(Email *t1,Email *t2)//(=) if two values are equivalent
{
	char buffer1[128]={0,};
	char buffer2[128]={0,};
	int len1,len2;

	//Local Checking
	len1=strcpy_to_capitalletters(buffer1,t1->local);
	len2=strcpy_to_capitalletters(buffer2,t2->local);

	if(len1!=len2)
		return IS_FALSE;
	
	if(is_two_words_equal(buffer1,buffer2)== IS_FALSE)
		return IS_FALSE;
	
	//Domain Checking
	len1=strcpy_to_capitalletters(buffer1,t1->domain);
	len2=strcpy_to_capitalletters(buffer2,t2->domain);

	if(len1!=len2)
		return IS_FALSE;
	if(is_two_words_equal(buffer1,buffer2)== IS_FALSE)
		return IS_FALSE;

	return IS_TRUE;
}

int func_is_email_not_equal(Email *t1,Email *t2)//(=) if two values are not equivalent
{
	return !func_is_email_equal(t1,t2);
}


int func_is_email_greater_than(Email *t1,Email *t2)//(>) the first is greater than the second
{

	char buffer1[128]={0,};
	char buffer2[128]={0,};
	int len1,len2,ret=0;

	//Domain Comparsion
	len1=strcpy_to_capitalletters(buffer1,t1->domain);
	len2=strcpy_to_capitalletters(buffer2,t2->domain);
	ret=strcmp(buffer1,buffer2);

	if(ret<0)
		return IS_FALSE;
	if(ret>0)
		return IS_TRUE;

	//Local Comparsion
	len1=strcpy_to_capitalletters(buffer1,t1->local);
	len2=strcpy_to_capitalletters(buffer2,t2->local);
	ret=strcmp(buffer1,buffer2);

	if(ret>0)
		return IS_TRUE;

	return IS_FALSE;
}

int func_is_email_less_equal(Email *t1,Email *t2)//(<=) the first is greater than the second or equivalent
{
	return !func_is_email_greater_than(t1,t2);
}

int func_is_email_less_than(Email *t1,Email *t2)//(<) the first is less than the second
{
	char buffer1[128]={0,};
	char buffer2[128]={0,};
	int len1,len2,ret=0;

	//Domain Comparsion
	len1=strcpy_to_capitalletters(buffer1,t1->domain);
	len2=strcpy_to_capitalletters(buffer2,t2->domain);
	ret=strcmp(buffer1,buffer2);

	if(ret>0)
		return IS_FALSE;
	if(ret<0)
		return IS_TRUE;

	//Local Comparsion
	len1=strcpy_to_capitalletters(buffer1,t1->local);
	len2=strcpy_to_capitalletters(buffer2,t2->local);
	ret=strcmp(buffer1,buffer2);

	if(ret<0)
		return IS_TRUE;

	return IS_FALSE;
}

int func_is_email_greater_equal(Email *t1,Email *t2)//(>=) the first is greater than the second or equivalent
{
	return !func_is_email_less_than(t1,t2);
}

int func_is_email_same_domain(Email *t1, Email *t2)//(~) the same domain
{

	char buffer1[128]={0,};
	char buffer2[128]={0,};
	int len1,len2,ret=0;
	//Domain Comparsion
	len1=strcpy_to_capitalletters(buffer1,t1->domain);
	len2=strcpy_to_capitalletters(buffer2,t2->domain);
	ret=strcmp(buffer1,buffer2);

	if(ret==0)
		return IS_TRUE;
	return IS_FALSE;
}

int func_is_email_not_same_domain(Email *t1, Email *t2)//(!~) not the same domain
{
	return !func_is_email_same_domain(t1,t2);
}


/////////////////////////////////////////////////////////////////////////////////

static int func_is_number(char ch)
{
	if(ch>='0' && ch<='9')
		return IS_TRUE;

	return IS_FALSE;
}
static int func_is_alphabet(char ch)
{
	if(ch>='a' && ch<='z')
		return IS_TRUE;
	if(ch>='A' && ch<='Z')
		return IS_TRUE;

	return IS_FALSE;
}
static int func_is_dash(char ch)
{
	return (ch=='-')?IS_TRUE:IS_FALSE;
}

static int func_is_dot(char ch)
{
	return (ch=='.')?IS_TRUE:IS_FALSE;
}

static int func_is_at(char ch)
{
	return (ch=='@')?IS_TRUE:IS_FALSE;
}

#if 0
#define	RETURN_MSG(msg,value)	{printf("%s\n",msg);return value;}
#else
#define	RETURN_MSG(msg,value)	{return value;}
#endif

int email_grammar_checker(char *string)
{
	int i=0,j=0;
	int val_start,val_end;
	int iteration_values[2][2]={{0,0},{0,0}};
	int len=0;			//length of string 
	int position_at=0;	//location of at
	int is_dot=0;		//found dot? //rule 3
	int count_at=0;
	int pass=SW_FAIL;	//pass variable (default:FAIL)
	int char_count;		//the number of letter
	char *p;
	char prev_ch=0;
	
/*
1) an email address has two parts, Local and Domain, separated by an '@' char
2) both the Local part and the Domain part consist of a sequence of Words, separated by '.' characters
3) the Local part has one or more Words; the Domain part has two or more Words (i.e. at least one '.')
4) each Word is a sequence of one or more characters, starting with a letter
5) each Word ends with a letter or a digit
6) between the starting and ending chars, there may be any number of letters, digits or hyphens ('-')
*/

//---------------------------------------------------------------------------------------
//RULE 1
//1) an email address has two parts, Local and Domain, separated by an '@' char
//String is not safisfied with rule 1( '@' should be located between Local and Domain.);
	p=string;
	len=0;
	count_at=0;
	pass=SW_FAIL;

	while(*p)
	{
		if(func_is_at(*p)== IS_TRUE)
		{
			pass=SW_PASS;				//this rule is valid because of the presence of @.
			position_at=(int)(p-string);//get position of '@'.
			count_at++;
		}
		p++;
		len++;
	}
	if(count_at>1) // abc@han@abc.com
		pass=SW_FAIL;

	if(pass==SW_FAIL)
		RETURN_MSG("err:r1",RET_INVALID);
		

	//Local Checking scope
	iteration_values[0][0]=0;
	iteration_values[0][1]=position_at;
	//Domain Checking scope
	iteration_values[1][0]=position_at+1;
	iteration_values[1][1]=len;
	pass=SW_PASS;

//----------------------------------------------------------------------------------------
//RULE 2
//2) both the Local part and the Domain part consist of a sequence of Words, separated by '.' characters
//Words should be separated by '.'. So, ".." is not permitted.

	for(j=0;j<2;j++)
	{
		val_start=iteration_values[j][0];
		val_end=iteration_values[j][1];

		if(func_is_dot(string[val_start])== IS_TRUE)					// .abc@mail.com =>FAIL
			pass=SW_FAIL;
		else if(func_is_dot(string[val_end-1])== IS_TRUE) // abc.@mail.com =>FAIL
					pass=SW_FAIL;
	
		if(pass == SW_FAIL) 		
			RETURN_MSG("err:r2-0",RET_INVALID);


		prev_ch= string[val_start];
		for(i=val_start+1;i<val_end;i++)
		{
			if(prev_ch==string[i])
				if(func_is_dot(prev_ch)== IS_TRUE)			//  jjj..jj@mail.com
				{
					pass=SW_FAIL;
					break;
				}
	
			prev_ch=string[i];
		}

		if(pass == SW_FAIL)
			RETURN_MSG("err:r2-1",RET_INVALID);

	}

//----------------------------------------------------------------------------------------
//RULE 3	
//3) the Local part has one or more Words; the Domain part has two or more Words (i.e. at least one '.')

//Local Checking
	char_count=0;
	for(i=0;i<position_at;i++)
	{
		if(func_is_alphabet(string[i]) == IS_TRUE)
			char_count++;
	}
	if(char_count<1)
		pass=SW_FAIL;

	if(pass == SW_FAIL)
			RETURN_MSG("err:r3-1",RET_INVALID);


//Domail Checking
	char_count=0;
	is_dot=0;
	for(i=position_at+2;i<len;i++)
	{
		if(func_is_alphabet(string[i]) == IS_TRUE)
			char_count++;
		else if(func_is_dot(string[i]) == IS_TRUE) 
			is_dot=1; //haha@abc.com 
	}
	if(char_count<2 || is_dot==0) // haha@ad  
		pass=SW_FAIL;

	if(pass == SW_FAIL)
			RETURN_MSG("err:r3-2",RET_INVALID);

//----------------------------------------------------------------------------------------
//RULE 4	
//each Word is a sequence of one or more characters, starting with a letter
	
	for(j=0;j<2;j++)
	{
		val_start=iteration_values[j][0];
		val_end=iteration_values[j][1];

		prev_ch=string[val_start];
		if(func_is_alphabet(prev_ch) == IS_FALSE) //1haha@abc.com
			pass=SW_FAIL;
		else 
		for(i=val_start+1;i<val_end;i++)
		{
			if(func_is_dot(prev_ch)== IS_TRUE)
			{
				if(func_is_alphabet(string[i]) == IS_FALSE) // abc.1ab@abc.com
				{
					pass=SW_FAIL;
					break;
				}
			}
		}

		if(pass == SW_FAIL)
			RETURN_MSG("err:r4",RET_INVALID);

	}


//----------------------------------------------------------------------------------------
//RULE 5
//5) each Word ends with a letter or a digit
	for(j=0;j<2;j++)
	{
		val_start=iteration_values[j][0];
		val_end=iteration_values[j][1];

		if(!((func_is_alphabet(string[val_end-1]) ==IS_TRUE) || (func_is_number(string[val_end-1])==IS_TRUE)))
		// abc [d]  @abc.com is a letter or a digit
			pass=SW_FAIL;
		else
		{
			prev_ch=string[val_start];
			for(i=val_start+1;i<val_end;i++)
			{
				if(func_is_dot(string[i]) == IS_TRUE) // abc[.]@abc.com
				{
					if(!((func_is_alphabet(prev_ch) == IS_TRUE) || (func_is_number(prev_ch) == IS_TRUE)))
					{
						pass=SW_FAIL;
						break;
					}
				}

				prev_ch=string[i];
			}
		}

		if(pass == SW_FAIL)
				RETURN_MSG("err:r5",RET_INVALID);
	}


//----------------------------------------------------------------------------------------
//RULE 6
//6) between the starting and ending chars, there may be any number of letters, digits or hyphens ('-')
	for(j=0;j<2;j++)
	{
		val_start=iteration_values[j][0];
		val_end=iteration_values[j][1];
	
		if(func_is_dash(string[val_start])== IS_TRUE || func_is_dash(string[val_end-1]) == IS_TRUE) // -abc.dsfs-@abc.com
			pass=SW_FAIL;
		else
		{
			prev_ch=string[val_start];
			for(i=val_start+1;i<val_end;i++)
			{
				if(func_is_dot(prev_ch) == IS_TRUE)// [.]-abc@abc.com
				{
					if(func_is_dash(string[i]) == IS_TRUE) // .[-]abc@abc.com
					{
						pass=SW_FAIL;
						break;
					}
				}
				else if(func_is_dot(string[i]) == IS_TRUE)	// abc-[.]ag@abc.com
					{
						if(func_is_dash(prev_ch) == IS_TRUE) // ab[-].ag@abc.com
						{
							pass=SW_FAIL;
							break;
						}
					}

				prev_ch=string[i];
			}
		}
	
		if(pass == SW_FAIL)
			RETURN_MSG("err:r6-1",RET_INVALID);
	}

//------------------------------------------------------------------------------------------
	return RET_VALID;
}

