#include <stdio.h>

#define YY_INPUT(buf, result, max)			\
{							\
  int c= getchar();					\
  result= (EOF == c) ? 0 : (*(buf)= c, 1);		\
  if (EOF != c) printf("<%c>\n", c);			\
}

#include "left.peg.c"

int main()
{
  printf(yyparse() ? "success\n" : "failure\n");

  return 0;
}
