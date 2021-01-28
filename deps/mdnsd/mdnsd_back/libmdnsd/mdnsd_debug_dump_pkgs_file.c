/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * This code is used to generate a binary file for every request type
 * which can be sent from a client to the server.
 * These files form the basic corpus for fuzzing the server.
 */

#ifndef MDNSD_DEBUG_DUMP_PKGS_FILE
#error MDNSD_DEBUG_DUMP_PKGS_FILE must be defined
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "mdnsd.h"


unsigned int mdnsd_dump_chunkCount = 0;

void mdnsd_debug_dumpCompleteChunk(mdns_daemon_t *d, const char* data, size_t len) {

    char fileName[250];
    struct timeval t;
    gettimeofday(&t, 0);
    snprintf(fileName, sizeof(fileName), "%s/%05u_%ld", MDNSD_CORPUS_OUTPUT_DIR, ++mdnsd_dump_chunkCount, t.tv_sec);

    char dumpOutputFile[266];
    snprintf(dumpOutputFile, 255, "%s.bin", fileName);
    // check if file exists and if yes create a counting filename to avoid overwriting
    unsigned cnt = 1;
    while ( access( dumpOutputFile, F_OK ) != -1 ) {
        snprintf(dumpOutputFile, 266, "%s_%u.bin", fileName, cnt);
        cnt++;
    }

    FILE *write_ptr = fopen(dumpOutputFile, "ab");
    fwrite(data, len, 1, write_ptr); // write 10 bytes from our buffer
    fclose(write_ptr);
}
