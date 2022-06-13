#include <stdio.h>
#include <stdlib.h>

#include "rule.peg.c"

int main()
{
  while (yyparse());

  return 0;
}
