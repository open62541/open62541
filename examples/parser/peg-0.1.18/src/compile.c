/* Copyright (c) 2007--2013 by Ian Piumarta
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the 'Software'),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software.  Acknowledgement
 * of the use of this Software in supporting documentation would be
 * appreciated but is not required.
 * 
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 * 
 * Last edited: 2016-07-22 09:43:05 by piumarta on zora.local
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
# undef inline
# define inline __inline
#endif

#include "version.h"
#include "tree.h"

static int yyl(void)
{
  static int prev= 0;
  return ++prev;
}

static void charClassSet  (unsigned char bits[], int c)	{ bits[c >> 3] |=  (1 << (c & 7)); }
static void charClassClear(unsigned char bits[], int c)	{ bits[c >> 3] &= ~(1 << (c & 7)); }

typedef void (*setter)(unsigned char bits[], int c);

static inline int oigit(int c)	{ return ('0' <= c && c <= '7'); }
static inline int higit(int c)	{ return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f'); }

static inline int hexval(int c)
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('A' <= c && c <= 'F') return 10 - 'A' + c;
    if ('a' <= c && c <= 'f') return 10 - 'a' + c;
    return 0;
}

static int cnext(unsigned char **ccp)
{
    unsigned char *cclass= *ccp;
    int c= *cclass++;
    if (c)
    {
	if ('\\' == c && *cclass)
	{
	    switch (c= *cclass++)
	    {
		case 'a':  c= '\a';   break;	/* bel */
		case 'b':  c= '\b';   break;	/* bs */
		case 'e':  c= '\033'; break;	/* esc */
		case 'f':  c= '\f';   break;	/* ff */
		case 'n':  c= '\n';   break;	/* nl */
		case 'r':  c= '\r';   break;	/* cr */
		case 't':  c= '\t';   break;	/* ht */
		case 'v':  c= '\v';   break;	/* vt */
		case 'x':
		    c= 0;
		    if (higit(*cclass)) c= (c << 4) + hexval(*cclass++);
		    if (higit(*cclass)) c= (c << 4) + hexval(*cclass++);
		    break;
		default:
		    if (oigit(c))
		    {
			c -= '0';
			if (oigit(*cclass)) c= (c << 3) + *cclass++ - '0';
			if (oigit(*cclass)) c= (c << 3) + *cclass++ - '0';
		    }
		    break;
	    }
	}
	*ccp= cclass;
    }
    return c;
}

static char *makeCharClass(unsigned char *cclass)
{
  unsigned char	 bits[32];
  setter	 set;
  int		 c, prev= -1;
  static char	 string[256];
  char		*ptr;

  if ('^' == *cclass)
    {
      memset(bits, 255, 32);
      set= charClassClear;
      ++cclass;
    }
  else
    {
      memset(bits, 0, 32);
      set= charClassSet;
    }

  while (*cclass)
    {
      if ('-' == *cclass && cclass[1] && prev >= 0)
	{
	  ++cclass;
	  for (c= cnext(&cclass);  prev <= c;  ++prev)
	    set(bits, prev);
	  prev= -1;
	}
      else
	{
	  c= cnext(&cclass);
	  set(bits, prev= c);
	}
    }

  ptr= string;
  for (c= 0;  c < 32;  ++c)
    ptr += sprintf(ptr, "\\%03o", bits[c]);

  return string;
}

static void begin(void)		{ fprintf(output, "\n  {"); }
static void end(void)		{ fprintf(output, "\n  }"); }
static void label(int n)	{ fprintf(output, "\n  l%d:;\t", n); }
static void jump(int n)		{ fprintf(output, "  goto l%d;", n); }
static void save(int n)		{ fprintf(output, "  int yypos%d= yy->__pos, yythunkpos%d= yy->__thunkpos;", n, n); }
static void restore(int n)	{ fprintf(output,     "  yy->__pos= yypos%d; yy->__thunkpos= yythunkpos%d;", n, n); }

static void Node_compile_c_ko(Node *node, int ko)
{
  assert(node);
  switch (node->type)
    {
    case Rule:
      fprintf(stderr, "\ninternal error #1 (%s)\n", node->rule.name);
      exit(1);
      break;

    case Dot:
      fprintf(output, "  if (!yymatchDot(yy)) goto l%d;", ko);
      break;

    case Name:
      fprintf(output, "  if (!yy_%s(yy)) goto l%d;", node->name.rule->rule.name, ko);
      if (node->name.variable)
	fprintf(output, "  yyDo(yy, yySet, %d, 0);", node->name.variable->variable.offset);
      break;

    case Character:
    case String:
      {
	int len= strlen(node->string.value);
	if (1 == len)
	  {
	    if ('\'' == node->string.value[0])
	      fprintf(output, "  if (!yymatchChar(yy, '\\'')) goto l%d;", ko);
	    else
	      fprintf(output, "  if (!yymatchChar(yy, '%s')) goto l%d;", node->string.value, ko);
	  }
	else
	  if (2 == len && '\\' == node->string.value[0])
	    fprintf(output, "  if (!yymatchChar(yy, '%s')) goto l%d;", node->string.value, ko);
	  else
	    fprintf(output, "  if (!yymatchString(yy, \"%s\")) goto l%d;", node->string.value, ko);
      }
      break;

    case Class:
      fprintf(output, "  if (!yymatchClass(yy, (unsigned char *)\"%s\")) goto l%d;", makeCharClass(node->cclass.value), ko);
      break;

    case Action:
      fprintf(output, "  yyDo(yy, yy%s, yy->__begin, yy->__end);", node->action.name);
      break;

    case Inline:
      fprintf(output, "  yyText(yy, yy->__begin, yy->__end);\n");
      fprintf(output, "#define yytext yy->__text\n");
      fprintf(output, "#define yyleng yy->__textlen\n");
      fprintf(output, "%s;\n", node->inLine.text);
      fprintf(output, "#undef yytext\n");
      fprintf(output, "#undef yyleng\n");
      break;

    case Predicate:
      fprintf(output, "  yyText(yy, yy->__begin, yy->__end);  {\n");
      fprintf(output, "#define yytext yy->__text\n");
      fprintf(output, "#define yyleng yy->__textlen\n");
      fprintf(output, "if (!(%s)) goto l%d;\n", node->predicate.text, ko);
      fprintf(output, "#undef yytext\n");
      fprintf(output, "#undef yyleng\n");
      fprintf(output, "  }");
      break;

    case Error:
      {
	int eok= yyl(), eko= yyl();
	Node_compile_c_ko(node->error.element, eko);
	jump(eok);
	label(eko);
	fprintf(output, "  yyText(yy, yy->__begin, yy->__end);  {\n");
	fprintf(output, "#define yytext yy->__text\n");
	fprintf(output, "#define yyleng yy->__textlen\n");
	fprintf(output, "  %s;\n", node->error.text);
	fprintf(output, "#undef yytext\n");
	fprintf(output, "#undef yyleng\n");
	fprintf(output, "  }");
	jump(ko);
	label(eok);
      }
      break;

    case Alternate:
      {
	int ok= yyl();
	begin();
	save(ok);
	for (node= node->alternate.first;  node;  node= node->alternate.next)
	  if (node->alternate.next)
	    {
	      int next= yyl();
	      Node_compile_c_ko(node, next);
	      jump(ok);
	      label(next);
	      restore(ok);
	    }
	  else
	    Node_compile_c_ko(node, ko);
	end();
	label(ok);
      }
      break;

    case Sequence:
      for (node= node->sequence.first;  node;  node= node->sequence.next)
	Node_compile_c_ko(node, ko);
      break;

    case PeekFor:
      {
	int ok= yyl();
	begin();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ko);
	restore(ok);
	end();
      }
      break;

    case PeekNot:
      {
	int ok= yyl();
	begin();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ok);
	jump(ko);
	label(ok);
	restore(ok);
	end();
      }
      break;

    case Query:
      {
	int qko= yyl(), qok= yyl();
	begin();
	save(qko);
	Node_compile_c_ko(node->query.element, qko);
	jump(qok);
	label(qko);
	restore(qko);
	end();
	label(qok);
      }
      break;

    case Star:
      {
	int again= yyl(), out= yyl();
	label(again);
	begin();
	save(out);
	Node_compile_c_ko(node->star.element, out);
	jump(again);
	label(out);
	restore(out);
	end();
      }
      break;

    case Plus:
      {
	int again= yyl(), out= yyl();
	Node_compile_c_ko(node->plus.element, ko);
	label(again);
	begin();
	save(out);
	Node_compile_c_ko(node->plus.element, out);
	jump(again);
	label(out);
	restore(out);
	end();
      }
      break;

    default:
      fprintf(stderr, "\nNode_compile_c_ko: illegal node type %d\n", node->type);
      exit(1);
    }
}


static int countVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      ++count;
      node= node->variable.next;
    }
  return count;
}

static void defineVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      fprintf(output, "#define %s yy->__val[%d]\n", node->variable.name, --count);
      node->variable.offset= count;
      node= node->variable.next;
    }
  fprintf(output, "#define __ yy->__\n");
  fprintf(output, "#define yypos yy->__pos\n");
  fprintf(output, "#define yythunkpos yy->__thunkpos\n");
}

static void undefineVariables(Node *node)
{
  fprintf(output, "#undef yythunkpos\n");
  fprintf(output, "#undef yypos\n");
  fprintf(output, "#undef yy\n");
  while (node)
    {
      fprintf(output, "#undef %s\n", node->variable.name);
      node= node->variable.next;
    }
}


static void Rule_compile_c2(Node *node)
{
  assert(node);
  assert(Rule == node->type);

  if (!node->rule.expression)
    fprintf(stderr, "rule '%s' used but not defined\n", node->rule.name);
  else
    {
      int ko= yyl(), safe;

      if ((!(RuleUsed & node->rule.flags)) && (node != start))
	fprintf(stderr, "rule '%s' defined but not used\n", node->rule.name);

      safe= ((Query == node->rule.expression->type) || (Star == node->rule.expression->type));

      fprintf(output, "\nYY_RULE(int) yy_%s(yycontext *yy)\n{", node->rule.name);
      if (!safe) save(0);
      if (node->rule.variables)
	fprintf(output, "  yyDo(yy, yyPush, %d, 0);", countVariables(node->rule.variables));
      fprintf(output, "\n  yyprintf((stderr, \"%%s\\n\", \"%s\"));", node->rule.name);
      Node_compile_c_ko(node->rule.expression, ko);
      fprintf(output, "\n  yyprintf((stderr, \"  ok   %%s @ %%s\\n\", \"%s\", yy->__buf+yy->__pos));", node->rule.name);
      if (node->rule.variables)
	fprintf(output, "  yyDo(yy, yyPop, %d, 0);", countVariables(node->rule.variables));
      fprintf(output, "\n  return 1;");
      if (!safe)
	{
	  label(ko);
	  restore(0);
	  fprintf(output, "\n  yyprintf((stderr, \"  fail %%s @ %%s\\n\", \"%s\", yy->__buf+yy->__pos));", node->rule.name);
	  fprintf(output, "\n  return 0;");
	}
      fprintf(output, "\n}");
    }

  if (node->rule.next)
    Rule_compile_c2(node->rule.next);
}

static char *header= "\
#include <stdio.h>\n\
#include <stdlib.h>\n\
#include <string.h>\n\
";

static char *preamble= "\
#ifndef YY_MALLOC\n\
#define YY_MALLOC(C, N)		malloc(N)\n\
#endif\n\
#ifndef YY_REALLOC\n\
#define YY_REALLOC(C, P, N)	realloc(P, N)\n\
#endif\n\
#ifndef YY_FREE\n\
#define YY_FREE(C, P)		free(P)\n\
#endif\n\
#ifndef YY_LOCAL\n\
#define YY_LOCAL(T)	static T\n\
#endif\n\
#ifndef YY_ACTION\n\
#define YY_ACTION(T)	static T\n\
#endif\n\
#ifndef YY_RULE\n\
#define YY_RULE(T)	static T\n\
#endif\n\
#ifndef YY_PARSE\n\
#define YY_PARSE(T)	T\n\
#endif\n\
#ifndef YYPARSE\n\
#define YYPARSE		yyparse\n\
#endif\n\
#ifndef YYPARSEFROM\n\
#define YYPARSEFROM	yyparsefrom\n\
#endif\n\
#ifndef YYRELEASE\n\
#define YYRELEASE	yyrelease\n\
#endif\n\
#ifndef YY_BEGIN\n\
#define YY_BEGIN	( yy->__begin= yy->__pos, 1)\n\
#endif\n\
#ifndef YY_END\n\
#define YY_END		( yy->__end= yy->__pos, 1)\n\
#endif\n\
#ifdef YY_DEBUG\n\
# define yyprintf(args)	fprintf args\n\
#else\n\
# define yyprintf(args)\n\
#endif\n\
#ifndef YYSTYPE\n\
#define YYSTYPE	int\n\
#endif\n\
#ifndef YY_STACK_SIZE\n\
#define YY_STACK_SIZE 128\n\
#endif\n\
\n\
#ifndef YY_BUFFER_SIZE\n\
#define YY_BUFFER_SIZE 1024\n\
#endif\n\
\n\
#ifndef YY_PART\n\
\n\
typedef struct _yycontext yycontext;\n\
typedef void (*yyaction)(yycontext *yy, char *yytext, int yyleng);\n\
typedef struct _yythunk { int begin, end;  yyaction  action;  struct _yythunk *next; } yythunk;\n\
\n\
struct _yycontext {\n\
  char     *__buf;\n\
  int       __buflen;\n\
  int       __pos;\n\
  int       __limit;\n\
  char     *__text;\n\
  int       __textlen;\n\
  int       __begin;\n\
  int       __end;\n\
  int       __textmax;\n\
  yythunk  *__thunks;\n\
  int       __thunkslen;\n\
  int       __thunkpos;\n\
  YYSTYPE   __;\n\
  YYSTYPE  *__val;\n\
  YYSTYPE  *__vals;\n\
  int       __valslen;\n\
#ifdef YY_CTX_MEMBERS\n\
  YY_CTX_MEMBERS\n\
#endif\n\
};\n\
\n\
#ifdef YY_CTX_LOCAL\n\
#define YY_CTX_PARAM_	yycontext *yyctx,\n\
#define YY_CTX_PARAM	yycontext *yyctx\n\
#define YY_CTX_ARG_	yyctx,\n\
#define YY_CTX_ARG	yyctx\n\
#ifndef YY_INPUT\n\
#define YY_INPUT(yy, buf, result, max_size)		\\\n\
  {							\\\n\
    int yyc= getchar();					\\\n\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\\\n\
    yyprintf((stderr, \"<%c>\", yyc));			\\\n\
  }\n\
#endif\n\
#else\n\
#define YY_CTX_PARAM_\n\
#define YY_CTX_PARAM\n\
#define YY_CTX_ARG_\n\
#define YY_CTX_ARG\n\
yycontext _yyctx= { 0, 0 };\n\
yycontext *yyctx= &_yyctx;\n\
#ifndef YY_INPUT\n\
#define YY_INPUT(buf, result, max_size)			\\\n\
  {							\\\n\
    int yyc= getchar();					\\\n\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\\\n\
    yyprintf((stderr, \"<%c>\", yyc));			\\\n\
  }\n\
#endif\n\
#endif\n\
\n\
YY_LOCAL(int) yyrefill(yycontext *yy)\n\
{\n\
  int yyn;\n\
  while (yy->__buflen - yy->__pos < 512)\n\
    {\n\
      yy->__buflen *= 2;\n\
      yy->__buf= (char *)YY_REALLOC(yy, yy->__buf, yy->__buflen);\n\
    }\n\
#ifdef YY_CTX_LOCAL\n\
  YY_INPUT(yy, (yy->__buf + yy->__pos), yyn, (yy->__buflen - yy->__pos));\n\
#else\n\
  YY_INPUT((yy->__buf + yy->__pos), yyn, (yy->__buflen - yy->__pos));\n\
#endif\n\
  if (!yyn) return 0;\n\
  yy->__limit += yyn;\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchDot(yycontext *yy)\n\
{\n\
  if (yy->__pos >= yy->__limit && !yyrefill(yy)) return 0;\n\
  ++yy->__pos;\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchChar(yycontext *yy, int c)\n\
{\n\
  if (yy->__pos >= yy->__limit && !yyrefill(yy)) return 0;\n\
  if ((unsigned char)yy->__buf[yy->__pos] == c)\n\
    {\n\
      ++yy->__pos;\n\
      yyprintf((stderr, \"  ok   yymatchChar(yy, %c) @ %s\\n\", c, yy->__buf+yy->__pos));\n\
      return 1;\n\
    }\n\
  yyprintf((stderr, \"  fail yymatchChar(yy, %c) @ %s\\n\", c, yy->__buf+yy->__pos));\n\
  return 0;\n\
}\n\
\n\
YY_LOCAL(int) yymatchString(yycontext *yy, const char *s)\n\
{\n\
  int yysav= yy->__pos;\n\
  while (*s)\n\
    {\n\
      if (yy->__pos >= yy->__limit && !yyrefill(yy)) return 0;\n\
      if (yy->__buf[yy->__pos] != *s)\n\
        {\n\
          yy->__pos= yysav;\n\
          return 0;\n\
        }\n\
      ++s;\n\
      ++yy->__pos;\n\
    }\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchClass(yycontext *yy, unsigned char *bits)\n\
{\n\
  int c;\n\
  if (yy->__pos >= yy->__limit && !yyrefill(yy)) return 0;\n\
  c= (unsigned char)yy->__buf[yy->__pos];\n\
  if (bits[c >> 3] & (1 << (c & 7)))\n\
    {\n\
      ++yy->__pos;\n\
      yyprintf((stderr, \"  ok   yymatchClass @ %s\\n\", yy->__buf+yy->__pos));\n\
      return 1;\n\
    }\n\
  yyprintf((stderr, \"  fail yymatchClass @ %s\\n\", yy->__buf+yy->__pos));\n\
  return 0;\n\
}\n\
\n\
YY_LOCAL(void) yyDo(yycontext *yy, yyaction action, int begin, int end)\n\
{\n\
  while (yy->__thunkpos >= yy->__thunkslen)\n\
    {\n\
      yy->__thunkslen *= 2;\n\
      yy->__thunks= (yythunk *)YY_REALLOC(yy, yy->__thunks, sizeof(yythunk) * yy->__thunkslen);\n\
    }\n\
  yy->__thunks[yy->__thunkpos].begin=  begin;\n\
  yy->__thunks[yy->__thunkpos].end=    end;\n\
  yy->__thunks[yy->__thunkpos].action= action;\n\
  ++yy->__thunkpos;\n\
}\n\
\n\
YY_LOCAL(int) yyText(yycontext *yy, int begin, int end)\n\
{\n\
  int yyleng= end - begin;\n\
  if (yyleng <= 0)\n\
    yyleng= 0;\n\
  else\n\
    {\n\
      while (yy->__textlen < (yyleng + 1))\n\
	{\n\
	  yy->__textlen *= 2;\n\
	  yy->__text= (char *)YY_REALLOC(yy, yy->__text, yy->__textlen);\n\
	}\n\
      memcpy(yy->__text, yy->__buf + begin, yyleng);\n\
    }\n\
  yy->__text[yyleng]= '\\0';\n\
  return yyleng;\n\
}\n\
\n\
YY_LOCAL(void) yyDone(yycontext *yy)\n\
{\n\
  int pos;\n\
  for (pos= 0;  pos < yy->__thunkpos;  ++pos)\n\
    {\n\
      yythunk *thunk= &yy->__thunks[pos];\n\
      int yyleng= thunk->end ? yyText(yy, thunk->begin, thunk->end) : thunk->begin;\n\
      yyprintf((stderr, \"DO [%d] %p %s\\n\", pos, thunk->action, yy->__text));\n\
      thunk->action(yy, yy->__text, yyleng);\n\
    }\n\
  yy->__thunkpos= 0;\n\
}\n\
\n\
YY_LOCAL(void) yyCommit(yycontext *yy)\n\
{\n\
  if ((yy->__limit -= yy->__pos))\n\
    {\n\
      memmove(yy->__buf, yy->__buf + yy->__pos, yy->__limit);\n\
    }\n\
  yy->__begin -= yy->__pos;\n\
  yy->__end -= yy->__pos;\n\
  yy->__pos= yy->__thunkpos= 0;\n\
}\n\
\n\
YY_LOCAL(int) yyAccept(yycontext *yy, int tp0)\n\
{\n\
  if (tp0)\n\
    {\n\
      fprintf(stderr, \"accept denied at %d\\n\", tp0);\n\
      return 0;\n\
    }\n\
  else\n\
    {\n\
      yyDone(yy);\n\
      yyCommit(yy);\n\
    }\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(void) yyPush(yycontext *yy, char *text, int count)\n\
{\n\
  yy->__val += count;\n\
  while (yy->__valslen <= yy->__val - yy->__vals)\n\
    {\n\
      long offset= yy->__val - yy->__vals;\n\
      yy->__valslen *= 2;\n\
      yy->__vals= (YYSTYPE *)YY_REALLOC(yy, yy->__vals, sizeof(YYSTYPE) * yy->__valslen);\n\
      yy->__val= yy->__vals + offset;\n\
    }\n\
}\n\
YY_LOCAL(void) yyPop(yycontext *yy, char *text, int count)   { yy->__val -= count; }\n\
YY_LOCAL(void) yySet(yycontext *yy, char *text, int count)   { yy->__val[count]= yy->__; }\n\
\n\
#endif /* YY_PART */\n\
\n\
#define	YYACCEPT	yyAccept(yy, yythunkpos0)\n\
\n\
";

static char *footer= "\n\
\n\
#ifndef YY_PART\n\
\n\
typedef int (*yyrule)(yycontext *yy);\n\
\n\
YY_PARSE(int) YYPARSEFROM(YY_CTX_PARAM_ yyrule yystart)\n\
{\n\
  int yyok;\n\
  if (!yyctx->__buflen)\n\
    {\n\
      yyctx->__buflen= YY_BUFFER_SIZE;\n\
      yyctx->__buf= (char *)YY_MALLOC(yyctx, yyctx->__buflen);\n\
      yyctx->__textlen= YY_BUFFER_SIZE;\n\
      yyctx->__text= (char *)YY_MALLOC(yyctx, yyctx->__textlen);\n\
      yyctx->__thunkslen= YY_STACK_SIZE;\n\
      yyctx->__thunks= (yythunk *)YY_MALLOC(yyctx, sizeof(yythunk) * yyctx->__thunkslen);\n\
      yyctx->__valslen= YY_STACK_SIZE;\n\
      yyctx->__vals= (YYSTYPE *)YY_MALLOC(yyctx, sizeof(YYSTYPE) * yyctx->__valslen);\n\
      yyctx->__begin= yyctx->__end= yyctx->__pos= yyctx->__limit= yyctx->__thunkpos= 0;\n\
    }\n\
  yyctx->__begin= yyctx->__end= yyctx->__pos;\n\
  yyctx->__thunkpos= 0;\n\
  yyctx->__val= yyctx->__vals;\n\
  yyok= yystart(yyctx);\n\
  if (yyok) yyDone(yyctx);\n\
  yyCommit(yyctx);\n\
  return yyok;\n\
}\n\
\n\
YY_PARSE(int) YYPARSE(YY_CTX_PARAM)\n\
{\n\
  return YYPARSEFROM(YY_CTX_ARG_ yy_%s);\n\
}\n\
\n\
YY_PARSE(yycontext *) YYRELEASE(yycontext *yyctx)\n\
{\n\
  if (yyctx->__buflen)\n\
    {\n\
      yyctx->__buflen= 0;\n\
      YY_FREE(yyctx, yyctx->__buf);\n\
      YY_FREE(yyctx, yyctx->__text);\n\
      YY_FREE(yyctx, yyctx->__thunks);\n\
      YY_FREE(yyctx, yyctx->__vals);\n\
    }\n\
  return yyctx;\n\
}\n\
\n\
#endif\n\
";

void Rule_compile_c_header(void)
{
  fprintf(output, "/* A recursive-descent parser generated by peg %d.%d.%d */\n", PEG_MAJOR, PEG_MINOR, PEG_LEVEL);
  fprintf(output, "\n");
  fprintf(output, "%s", header);
  fprintf(output, "#define YYRULECOUNT %d\n", ruleCount);
}

int consumesInput(Node *node)
{
  if (!node) return 0;

  switch (node->type)
    {
    case Rule:
      {
	int result= 0;
	if (RuleReached & node->rule.flags)
	  fprintf(stderr, "possible infinite left recursion in rule '%s'\n", node->rule.name);
	else
	  {
	    node->rule.flags |= RuleReached;
	    result= consumesInput(node->rule.expression);
	    node->rule.flags &= ~RuleReached;
	  }
	return result;
      }
      break;

    case Dot:		return 1;
    case Name:		return consumesInput(node->name.rule);
    case Character:
    case String:	return strlen(node->string.value) > 0;
    case Class:		return 1;
    case Action:	return 0;
    case Inline:	return 0;
    case Predicate:	return 0;
    case Error:		return consumesInput(node->error.element);

    case Alternate:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (!consumesInput(n))
	    return 0;
      }
      return 1;

    case Sequence:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (consumesInput(n))
	    return 1;
      }
      return 0;

    case PeekFor:	return 0;
    case PeekNot:	return 0;
    case Query:		return 0;
    case Star:		return 0;
    case Plus:		return consumesInput(node->plus.element);

    default:
      fprintf(stderr, "\nconsumesInput: illegal node type %d\n", node->type);
      exit(1);
    }
  return 0;
}


void Rule_compile_c(Node *node, int nolines)
{
  Node *n;

  for (n= rules;  n;  n= n->rule.next)
    consumesInput(n);

  fprintf(output, "%s", preamble);
  for (n= node;  n;  n= n->rule.next)
    fprintf(output, "YY_RULE(int) yy_%s(yycontext *yy); /* %d */\n", n->rule.name, n->rule.id);
  fprintf(output, "\n");
  for (n= actions;  n;  n= n->action.list)
    {
      fprintf(output, "YY_ACTION(void) yy%s(yycontext *yy, char *yytext, int yyleng)\n{\n", n->action.name);
      defineVariables(n->action.rule->rule.variables);
      fprintf(output, "  yyprintf((stderr, \"do yy%s\\n\"));\n", n->action.name);
      fprintf(output, "  {\n");
      if (!nolines)
	fprintf(output, "#line %i\n", n->action.line);
      fprintf(output, "  %s;\n", n->action.text);
      fprintf(output, "  }\n");
      undefineVariables(n->action.rule->rule.variables);
      fprintf(output, "}\n");
    }
  Rule_compile_c2(node);
  fprintf(output, footer, start->rule.name);
}
