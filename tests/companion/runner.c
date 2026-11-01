/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "companion_test.h"
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
    size_t selected = (size_t)COMPANION_CASE;
    UA_Boolean copy = true;
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "--case") && i + 1 < argc)
            selected = (size_t)strtoul(argv[++i], NULL, 10);
        else if(!strcmp(argv[i], "--inplace"))
            copy = false;
        else if(strcmp(argv[i], "--copy"))
            return EXIT_FAILURE;
    }
    return companionRun(selected, copy);
}
