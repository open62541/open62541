#include <stdio.h>
#include <stdlib.h>

int stack[1024];
int stackp= -1;
int var= 0;
int vars[26];

static int push(int n)	{ return stack[++stackp]= n; }
static int pop(void)	{ return stack[stackp--]; }
static int top(void)	{ return stack[stackp]; }

#include "dcv.peg.c"

int main()
{
  while (yyparse());

  return 0;
}
