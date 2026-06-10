/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <open62541/server.h>

#include "../common.h"

#include <check.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <dirent.h>
# include <limits.h>
# include <sys/stat.h>
# include <unistd.h>
#endif

/* Create a temporary directory. Caller must free the returned string. */
static char *
createTempDir(void) {
#if defined(_WIN32)
    char dirname[MAX_PATH + 1];
    for(int i = 0; i < 100; i++) {
        snprintf(dirname, sizeof(dirname), "ua_pki_%lu_%d",
                 GetTickCount(), i);
        if(CreateDirectoryA(dirname, NULL))
            return _strdup(dirname);
    }
    return NULL;
#else
    char tmpdir[] = "/tmp/ua_pki_test_XXXXXX";
    const char *pkiDir = mkdtemp(tmpdir);
    if(!pkiDir)
        return NULL;
    return strdup(pkiDir);
#endif
}

/* Recursively remove a directory */
static void
removeDir(const char *path) {
#if defined(_WIN32)
    char search[MAX_PATH + 3];
    snprintf(search, sizeof(search), "%s\\*", path);
    WIN32_FIND_DATAA ffd;
    HANDLE hFind = FindFirstFileA(search, &ffd);
    if(hFind == INVALID_HANDLE_VALUE)
        return;
    do {
        if(strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
            continue;
        char full[MAX_PATH + 1];
        snprintf(full, sizeof(full), "%s\\%s", path, ffd.cFileName);
        if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            removeDir(full);
        else
            DeleteFileA(full);
    } while(FindNextFileA(hFind, &ffd) != 0);
    FindClose(hFind);
    RemoveDirectoryA(path);
#else
    DIR *dir = opendir(path);
    if(!dir)
        return;

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full[PATH_MAX];
        int n = snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        if(n < 0 || (size_t)n >= sizeof(full))
            continue;

        struct stat st;
        if(stat(full, &st) != 0)
            continue;

        if(S_ISDIR(st.st_mode)) {
            removeDir(full);
            continue;
        }

        unlink(full);
    }

    closedir(dir);
    rmdir(path);
#endif
}

/* Test: pkiFolder in JSON config actually configures the filestore
 * certificate group, rather than leaving the AcceptAll default in place.
 *
 * Regression test for #8057 */
START_TEST(server_from_json_with_pkifolder) {
    UA_StatusCode retval;

    /* Build a JSON config with pkiFolder pointing to a real temp dir */
    char *pkiDir = createTempDir();
    ck_assert_ptr_ne(pkiDir, NULL);

    char *json = NULL;
    const size_t bufsize = 2048;
    json = (char *)UA_malloc(bufsize);
    ck_assert_ptr_ne(json, NULL);
    int n = snprintf(json, bufsize,
        "{\n"
        "  \"serverUrls\": [\"opc.tcp://localhost:4840\"],\n"
        "  \"pkiFolder\": \"%s\"\n"
        "}\n",
        pkiDir);
    ck_assert_int_gt(n, 0);
    ck_assert_int_lt((size_t)n, bufsize);

    UA_ByteString jsonBytes;
    jsonBytes.length = strlen(json);
    jsonBytes.data = (UA_Byte *)UA_malloc(jsonBytes.length);
    ck_assert_ptr_ne(jsonBytes.data, NULL);
    memcpy(jsonBytes.data, json, jsonBytes.length);
    UA_free(json);

    /* Create server from config */
    UA_Server *server = UA_Server_newFromFile(jsonBytes);
    UA_ByteString_clear(&jsonBytes);

    ck_assert_ptr_ne(server, NULL);

    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* The bug: secureChannelPKI and sessionPKI should have a filestore context
     * (non-NULL) because pkiFolder was set. With AcceptAll, context remains NULL. */
    ck_assert_ptr_ne(config->secureChannelPKI.context, NULL);
    ck_assert_ptr_ne(config->sessionPKI.context, NULL);

    /* Simple verification: the PKI store should be a filestore, not AcceptAll */
    UA_TrustListDataType trustList;
    memset(&trustList, 0, sizeof(UA_TrustListDataType));
    retval = config->secureChannelPKI.getTrustList(&config->secureChannelPKI, &trustList);
    ck_assert_uint_eq(retval, UA_STATUSCODE_GOOD);
    /* The trust list from a fresh filestore should be empty (not an error) */

    UA_Server_delete(server);

    /* Clean up temp dir */
    if(pkiDir) {
        removeDir(pkiDir);
        UA_free(pkiDir);
    }
}
END_TEST

static Suite *testSuite_pki_from_json(void) {
    Suite *s = suite_create("Load server from json config with pkiFolder");
    TCase *tc_new = tcase_create("pki_regression");
    tcase_add_test(tc_new, server_from_json_with_pkifolder);
    suite_add_tcase(s, tc_new);
    return s;
}

int main(void) {
    int      number_failed = 0;
    Suite   *s;
    SRunner *sr;

    /* Only run if encryption is compiled in */
#if defined(UA_ENABLE_ENCRYPTION)
    s  = testSuite_pki_from_json();
    sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
#else
    (void)number_failed;
#endif

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
