#include <stdio.h>
#include "test.peg.c"

int main()
{
  while (yyparse());
  return 0;
}
