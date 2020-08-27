/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2019 (c) basysKom GmbH <opensource@basyskom.com> (Author: Frank Meerkötter)
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/types.h>

#include "server/ua_server_internal.h"
#include "server/ua_services.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "check.h"

#include <execinfo.h>

#define FAILURE_POINTS_FILE "allocationFailurePoints"
#define TESTED_FAILURE_POINTS_FILE "testedAllocationFailurePoints"
#define STACK_DEPTH 20

struct BreakingInfo {
  uintptr_t addressToBreak;
  const char* lineToBreak;
  bool alreadyBroken;

};

static struct BreakingInfo currentInfo = { 0, "", false };

static void* stackBuffer[STACK_DEPTH];
static FILE* memoryFile;
static int numberOfBreaks = 0;
static int totalFailurePoints = 0;

static FILE* testedPointsFile;

static UA_Server* server = NULL;

//static void timedCallbackHandler(UA_Server* s, void* data) {
//  *((UA_Boolean*) data) = false; // stop the server via a timedCallback
//}


static void functionToTest(void) {
  server = UA_Server_new();
  if(!server)
    return;

  if(UA_STATUSCODE_GOOD != UA_ServerConfig_setDefault(UA_Server_getConfig(server))) {
    UA_Server_delete(server);
    return;
  }

  //UA_Boolean running = true;
  // 0 is in the past so the server will terminate on the first iteration
  //UA_Server_addTimedCallback(server, &timedCallbackHandler, &running, 0, NULL);

  UA_Server_delete(server);
}

#define CHECK_STACK_AND_BREAK \
  if (currentInfo.alreadyBroken) return NULL; \
  int nptrs = backtrace(stackBuffer, STACK_DEPTH); \
    for(int i = 0; i < nptrs; i++) { \
      if(stackBuffer[i] == (void*) currentInfo.addressToBreak) { \
        printf("BREAKING at address %" PRIxPTR ": %s", currentInfo.addressToBreak, currentInfo.lineToBreak); \
        numberOfBreaks++; \
        currentInfo.alreadyBroken = !0; \
        return NULL; \
      } \
    }



void* mallocWrapper(size_t size);
void* callocWrapper(size_t nelem, size_t elsize);
void* reallocWrapper(void* ptr, size_t size);

void* mallocWrapper(size_t size) {
  CHECK_STACK_AND_BREAK
  return malloc(size);
}

void* callocWrapper(size_t nelem, size_t elsize) {
  CHECK_STACK_AND_BREAK
  return calloc(nelem, elsize);
}

void* reallocWrapper(void* ptr, size_t size) {
  CHECK_STACK_AND_BREAK
  return realloc(ptr, size);
}

START_TEST( checkAllocation) {
  ck_assert(memoryFile != NULL);
  if(memoryFile) {
    char lineBuffer[200];
    while(fgets(lineBuffer, 200, memoryFile)) {
      char* startOfText = strchr(lineBuffer, ',');
      if(startOfText == 0) {
        printf("Line %s doesn't have source file description\n", lineBuffer);
        currentInfo.lineToBreak = "";
      } else {
        currentInfo.lineToBreak = startOfText + 1;
        *startOfText = '\0';
      }

      currentInfo.alreadyBroken = 0;
      currentInfo.addressToBreak = atoi(lineBuffer);
      printf("Line to break is %" PRIxPTR ": %s", currentInfo.addressToBreak, currentInfo.lineToBreak);
      totalFailurePoints++;

      functionToTest();

      if (currentInfo.alreadyBroken) {
        *startOfText = ',';
        if(fputs(lineBuffer, testedPointsFile) < 0) {
          printf("Error writing line %s to file\n", lineBuffer);
        }
      }
    }

    printf("No more lines to break. %d of %d failure points were tested\n", numberOfBreaks, totalFailurePoints);

  }
}
END_TEST

static void setup(void) {
  //open file
  memoryFile = fopen(FAILURE_POINTS_FILE, "r");
  if(!memoryFile) {
    printf("ERROR: Couldn't find file %s\n", FAILURE_POINTS_FILE);
  }

   testedPointsFile = fopen(TESTED_FAILURE_POINTS_FILE, "w");
   if(!memoryFile) {
     printf("ERROR: Couldn't find file %s\n", TESTED_FAILURE_POINTS_FILE);
   }

  UA_mallocSingleton = mallocWrapper;
  UA_callocSingleton = callocWrapper;
  UA_reallocSingleton = reallocWrapper;

}

static void teardown(void) {
  if(memoryFile) {
    fclose(memoryFile);
  }
  if(testedPointsFile) {
    fclose(testedPointsFile);
  }
}

int main(void) {
  Suite* s = suite_create("Allocation Failure");

  TCase* tc_call = tcase_create("Allocation failure points");
  tcase_add_checked_fixture(tc_call, setup, teardown);
  tcase_add_test(tc_call, checkAllocation);
  suite_add_tcase(s, tc_call);

  SRunner* sr = srunner_create(s);
  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_NORMAL);
  int number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
