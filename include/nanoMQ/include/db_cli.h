#ifndef DB_CLI_H
#define DB_CLI_H

#include "fdb_version.h"
#include <pthread.h>
#include <stdlib.h>

#include <foundationdb/fdb_c.h>
#include <foundationdb/fdb_c_options.g.h>

FDBDatabase* openDatabase(pthread_t* netThread);
#endif