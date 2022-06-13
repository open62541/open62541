//
// Created by flo47663 on 05.10.2022.
//
#include <stdio.h>
#include <stdlib.h>

#include "parser/min_examples_query/SAO/soa.peg.c"

int main()
{
    while (yyparse());

    return 0;
}
