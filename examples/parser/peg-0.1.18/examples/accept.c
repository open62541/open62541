#include <stdio.h>
#include <stdlib.h>

#include "accept.peg.c"

int main()
{
  while (yyparse());

  return 0;
}
