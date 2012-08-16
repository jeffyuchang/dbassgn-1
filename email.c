/*
 * src/tutorial/email.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

//#include "fmgr.h"
#include "utils/builtins.h"
#include "access/hash.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


PG_MODULE_MAGIC;

////////////////////////////////
int email_grammar_checker(char *string);// check email syntax
//
int strcpy_to_lowercase(char *tar,char *src);//copy string and convert string into lowercase
int strcpy_to_lowercase_len(char *tar,char *src,int len);//copy string and convert string into lowercase
int strcpy_to(char *tar,char *src);//just copy string

//

#define	RET_VALID	1
#define	RET_INVALID	0
#define	IS_TRUE	1
#define	IS_FALSE	0
#define	SW_PASS	1
#define	SW_FAIL	0
/////////////////////////////////
//#define	DBG_MSG


/*
 * Since we use V1 function calling convention, all these functions have
 * the same signature as far as C is concerned.  We provide these prototypes
 * just to forestall warnings when compiled with gcc -Wmissing-prototypes.
 */
Datum		email_in(PG_FUNCTION_ARGS);
Datum		email_out(PG_FUNCTION_ARGS);
Datum		email_recv(PG_FUNCTION_ARGS);
Datum		email_send(PG_FUNCTION_ARGS);

Datum		email_eq(PG_FUNCTION_ARGS); 	//=
Datum		email_neq(PG_FUNCTION_ARGS);	//<>
Datum		email_gt(PG_FUNCTION_ARGS);		//>
Datum		email_sd(PG_FUNCTION_ARGS);		//~
Datum		email_nsd(PG_FUNCTION_ARGS);	//!=
Datum		email_ge(PG_FUNCTION_ARGS);		//>=
Datum		email_lt(PG_FUNCTION_ARGS);		//<
Datum		email_le(PG_FUNCTION_ARGS);		//<=

Datum		email_cmp(PG_FUNCTION_ARGS);
Datum		email_hash(PG_FUNCTION_ARGS);


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/
#define	GET_VARSIZE(x)	(VARSIZE(x)-VARHDRSZ)

PG_FUNCTION_INFO_V1(email_in);

Datum
email_in(PG_FUNCTION_ARGS)
{
	char *str=PG_GETARG_CSTRING(0);
	VarChar *rt;
	int len,ret;
	
	ret=email_grammar_checker(str);

	if(ret==IS_FALSE)
	ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for email: \"%s\"",str)));
	len=strlen(str);
	
	if(len>256)
	ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid length of input for email: len(%d) \"%s\"",
						len,str)));

	rt=(VarChar*) palloc(len+VARHDRSZ);
	SET_VARSIZE(rt,len+VARHDRSZ);
	strcpy_to_lowercase_len(VARDATA(rt),str,len);//copy string and convert string into lowercase
		
	PG_RETURN_VARCHAR_P(rt);

}

PG_FUNCTION_INFO_V1(email_out);

Datum
email_out(PG_FUNCTION_ARGS)
{
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	int len;
	char *rt;
	
	len = GET_VARSIZE(str);
	rt=palloc(len+1);
	memcpy(rt,VARDATA(str),len);
	rt[len]='\0';

	PG_RETURN_CSTRING(rt);
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
        StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
        VarChar *result;
        char *str;
        int nbytes;

        str=pq_getmsgtext(buf,buf->len-buf->cursor,&nbytes);
        result=(VarChar*) palloc((nbytes)+VARHDRSZ);
        SET_VARSIZE(result,nbytes+VARHDRSZ);
        memcpy(VARDATA(result),str,nbytes);
        pfree(str);
        PG_RETURN_VARCHAR_P(result);
}

PG_FUNCTION_INFO_V1(email_send);

Datum
email_send(PG_FUNCTION_ARGS)
{
	return textsend(fcinfo);
}

/*****************************************************************************
 * New Operators
 *
 *****************************************************************************/
static int getLocal_Domain(char *ret_local,char *ret_domain,VarChar *email) {
	int len,i,j;
	char *ptr;
	len = GET_VARSIZE(email);
	ptr=VARDATA(email);
	for(i=0;i<len;i++)
	{
		if(*ptr=='@')
		{
			ptr++;
			break;
		}
		ret_local[i]=*ptr;
		ptr++;
	}
	ret_local[i]='\0';
	for(j=0;j<len-i-1;j++)
	{
		ret_domain[j]=*ptr;
		ptr++;
	}
	ret_domain[j]='\0';
//	elog(LOG,"[%s]=[%s]",ret_local,ret_domain);
	return j;
}

static int is_email_eq(VarChar *email1, VarChar *email2) {
	
	char str_local[128],str_domain[128],str1_local[128],str1_domain[128]={0,};
	
	getLocal_Domain(str_local,str_domain,email1);	
	getLocal_Domain(str1_local,str1_domain,email2);

	if (!strcmp(str_local, str1_local) && !strcmp(str_domain, str1_domain)) {
		return IS_TRUE;
	}
	return IS_FALSE;
}

static int is_email_gt(VarChar *email1, VarChar *email2) {
	
	char str_local[128],str_domain[128],str1_local[128],str1_domain[128]={0,};
	int result = IS_FALSE;
	int domain_cmp, local_cmp;
	
	getLocal_Domain(str_local,str_domain,email1);
	getLocal_Domain(str1_local,str1_domain, email2);
	
	domain_cmp = strcmp(str_domain, str1_domain);
	local_cmp = strcmp(str_local, str1_local);
	
	if (domain_cmp > 0 || (domain_cmp == 0 && local_cmp > 0)) {
		result = IS_TRUE;
	}
	
	return result;
}

static int is_email_sd(VarChar *email1, VarChar *email2) {
	
	char str_local[128],str_domain[128],str1_local[128],str1_domain[128]={0,};
	getLocal_Domain(str_local,str_domain,email1);
	getLocal_Domain(str1_local,str1_domain, email2);
	
	if (!strcmp(str_domain, str1_domain)) {
		return IS_TRUE;
	}
	return IS_FALSE;
}

static int is_email_lt(VarChar *email1, VarChar *email2) {
	
	char str_local[128],str_domain[128],str1_local[128],str1_domain[128]={0,};
	int result = IS_FALSE;
	int domain_cmp, local_cmp;
	
	getLocal_Domain(str_local,str_domain,email1);
	getLocal_Domain(str1_local,str1_domain, email2);
	
	domain_cmp = strcmp(str_domain, str1_domain);
	local_cmp = strcmp(str_local, str1_local);
	
	if (domain_cmp < 0 || (domain_cmp == 0 && (local_cmp < 0))) {
		result = IS_TRUE;
	}
	return result;
}


PG_FUNCTION_INFO_V1(email_hash);

Datum
email_hash(PG_FUNCTION_ARGS)
{
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	char *rt;
	int32 len,hash_val=0;

	len=GET_VARSIZE(str);
	rt=palloc(len+1);
	memcpy(rt,VARDATA(str),len);
	rt[len]='\0';
	hash_val=hash_any((unsigned char*)rt,len);
	pfree(rt);

	PG_RETURN_INT32(hash_val);
}




PG_FUNCTION_INFO_V1(email_eq);

Datum
email_eq(PG_FUNCTION_ARGS)
{

	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);
	
	PG_RETURN_BOOL(is_email_eq(str, str1));

	
}


PG_FUNCTION_INFO_V1(email_neq);

Datum
email_neq(PG_FUNCTION_ARGS)
{
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);

	PG_RETURN_BOOL(!is_email_eq(str, str1));

}


PG_FUNCTION_INFO_V1(email_gt);

Datum
email_gt(PG_FUNCTION_ARGS)
{
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);

	PG_RETURN_BOOL(is_email_gt(str, str1));
	
}


PG_FUNCTION_INFO_V1(email_sd);

Datum
email_sd(PG_FUNCTION_ARGS)
{
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);

	PG_RETURN_BOOL(is_email_sd(str, str1));		
	
}

PG_FUNCTION_INFO_V1(email_nsd);

Datum
email_nsd(PG_FUNCTION_ARGS)
{

	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);

	PG_RETURN_BOOL(!is_email_sd(str, str1));

}


PG_FUNCTION_INFO_V1(email_ge);

Datum
email_ge(PG_FUNCTION_ARGS)
{
	
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);
	
	PG_RETURN_BOOL(!is_email_lt(str, str1));
}


PG_FUNCTION_INFO_V1(email_lt);

Datum
email_lt(PG_FUNCTION_ARGS)
{
	
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);

	PG_RETURN_BOOL(is_email_lt(str, str1));
}


PG_FUNCTION_INFO_V1(email_le);

Datum
email_le(PG_FUNCTION_ARGS)
{

	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);

	PG_RETURN_BOOL(!is_email_gt(str, str1));
}

PG_FUNCTION_INFO_V1(email_cmp);

Datum
email_cmp(PG_FUNCTION_ARGS)
{
	VarChar *str=PG_GETARG_VARCHAR_P(0);
	VarChar *str1=PG_GETARG_VARCHAR_P(1);	
	
	int result_value = 0;
	
	if(is_email_lt(str, str1)==IS_TRUE)  //<
		result_value=-1;
	else if(is_email_gt(str, str1)==IS_TRUE)  //>
		result_value=1;

	PG_RETURN_INT32(result_value);
	
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


int strcpy_to_lowercase_len(char *tar,char *src,int len)//copy string and convert string into lowercase
{
	int i;
	for(i=0;i<len;i++)
	{
		if(*src>='A' && *src<='Z')
			*tar='a'+(*src-'A');
		else *tar=*src;
		src++;
		tar++;
	}

	return len?1:0;
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

//////////////////////////////////////////////////////////////////////////////////////
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

