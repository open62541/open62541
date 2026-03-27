#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <directoryArch/common/fileSystemOperations_common.h>

#if defined(UA_ARCHITECTURE_POSIX)
#include <dirent.h>
#include <unistd.h>

static void deleteRecursive(const char *path) {
    struct stat st;
    if(stat(path, &st) != 0)
        return;

    if(S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if(!dir)
            return;

        struct dirent *entry;
        char buf[1024];

        while((entry = readdir(dir)) != NULL) {
            if(strcmp(entry->d_name, ".") == 0 ||
               strcmp(entry->d_name, "..") == 0)
                continue;

            snprintf(buf, sizeof(buf), "%s/%s", path, entry->d_name);
            deleteRecursive(buf);
        }

        closedir(dir);
        rmdir(path);
    } else {
        unlink(path);
    }
}

#endif

#if defined(UA_ARCHITECTURE_WIN32)
#include <windows.h>
#include <io.h>
static void deleteRecursive(const char *path) {
    DWORD attrs = GetFileAttributesA(path);
    if(attrs == INVALID_FILE_ATTRIBUTES)
        return;

    if(attrs & FILE_ATTRIBUTE_DIRECTORY) {
        char searchPath[MAX_PATH];
        snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(searchPath, &fd);
        if(h == INVALID_HANDLE_VALUE)
            return;

        do {
            if(strcmp(fd.cFileName, ".") == 0 ||
               strcmp(fd.cFileName, "..") == 0)
                continue;

            char child[MAX_PATH];
            snprintf(child, sizeof(child), "%s\\%s", path, fd.cFileName);
            deleteRecursive(child);

        } while(FindNextFileA(h, &fd));

        FindClose(h);
        RemoveDirectoryA(path);
    } else {
        DeleteFileA(path);
    }
}

#endif

#if defined(UA_FILESYSTEM)

struct stat st;

/* ------------------------------------------------------------------------- */
/* Directory Operations                                                      */
/* ------------------------------------------------------------------------- */
START_TEST(make_directory)
{
    UA_StatusCode status = makeDirectory("./TestDir");
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    ck_assert(stat("./TestDir", &st) == 0);
    ck_assert(S_ISDIR(st.st_mode));
}END_TEST

START_TEST(make_file)
{
    UA_StatusCode status = makeFile("./TestDir/TestFile.txt", false, NULL);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    
    ck_assert(stat("./TestDir/TestFile.txt", &st) == 0);
    ck_assert(S_ISREG(st.st_mode));
}END_TEST

START_TEST(is_directory)
{
    bool isDir = isDirectory("./TestDir");
    ck_assert(isDir == true);

    bool isNotDir = isDirectory("./TestDir/TestFile.txt");
    ck_assert(isNotDir == false);
}END_TEST

START_TEST(copy_item)
{
    UA_StatusCode status = moveOrCopyItem("./TestDir/TestFile.txt", "./TestFile.txt", true);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    
    ck_assert(stat("./TestFile.txt", &st) == 0);
    ck_assert(S_ISREG(st.st_mode));

    ck_assert(stat("./TestDir/TestFile.txt", &st) == 0);
    ck_assert(S_ISREG(st.st_mode));

    status = moveOrCopyItem("./TestDir", "./TestDir2", true);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    
    ck_assert(stat("./TestDir2", &st) == 0);
    ck_assert(S_ISDIR(st.st_mode));

    ck_assert(stat("./TestDir", &st) == 0);
    ck_assert(S_ISDIR(st.st_mode));
}END_TEST

START_TEST(move_item)
{
    UA_StatusCode status = moveOrCopyItem("./TestFile.txt", "./TestDir/TestFile2.txt", false);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    ck_assert(stat("./TestFile.txt", &st) != 0);
    ck_assert(stat("./TestDir/TestFile2.txt", &st) == 0);
    ck_assert(S_ISREG(st.st_mode));

    status = moveOrCopyItem("./TestDir2", "./TestDir/TestDir2", false);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    
    ck_assert(stat("./TestDir2", &st) != 0);
    ck_assert(stat("./TestDir/TestDir2", &st) == 0);
    ck_assert(S_ISDIR(st.st_mode));


}END_TEST

START_TEST(delete_item)
{
    UA_StatusCode status = deleteDirOrFile("./TestDir/TestFile2.txt");
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    status = deleteDirOrFile("./TestDir");
    ck_assert_int_eq(status, UA_STATUSCODE_BADINTERNALERROR);

    status = deleteDirOrFile("./TestDir/TestDir2/TestFile.txt");
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    status = deleteDirOrFile("./TestDir/TestDir2");
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    status = deleteDirOrFile("./TestDir/TestFile.txt");
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    status = deleteDirOrFile("./TestDir");
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
}END_TEST

/* ------------------------------------------------------------------------- */
/* File Operations                                                           */
/* ------------------------------------------------------------------------- */
START_TEST(open_file_test)
{
    UA_Int32 *handle = NULL;
    UA_StatusCode status = openFile("./TestDir/TestFile.txt", 0x01, &handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(handle, NULL);

    status = closeFile(handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(close_file_test)
{
    UA_Int32 *handle = NULL;
    UA_StatusCode status = openFile("./TestDir/TestFile.txt", 'r', &handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    ck_assert_ptr_ne(handle, NULL);

    status = closeFile(handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    /* Optional: double-close should fail or be no-op depending on your API contract.
       If it should fail, you can assert that here. */
}
END_TEST

START_TEST(write_file_test)
{
    UA_Int32 *handle = NULL;
    UA_StatusCode status = openFile("./TestDir/TestFile.txt", 'w', &handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    char *msg = "HelloWorld";
    UA_ByteString data;
    data.length = strlen(msg);
    data.data = (UA_Byte*)msg;

    status = writeFile(handle, &data);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    status = closeFile(handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
}
END_TEST

START_TEST(read_file_test)
{
    UA_Int32 *handle = NULL;
    UA_StatusCode status = openFile("./TestDir/TestFile.txt", 'r', &handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    UA_ByteString data;
    data.length = 10; // read "HelloWorld"
    data.data = (UA_Byte *)UA_calloc(10, sizeof(UA_Byte));

    status = readFile(handle, 10, &data);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    ck_assert(memcmp(data.data, "HelloWorld", 10) == 0);

    free(data.data);
    closeFile(handle);
}
END_TEST

START_TEST(seek_and_position_test)
{
    UA_Int32 *handle = NULL;
    UA_StatusCode status = openFile("./TestDir/TestFile.txt", 'r', &handle);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    status = seekFile(handle, 5);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    UA_UInt64 pos = 0;
    status = getFilePosition(handle, &pos);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);
    ck_assert_uint_eq(pos, 5);

    closeFile(handle);
}
END_TEST

START_TEST(get_file_size_test)
{
    UA_UInt64 size = 0;
    UA_StatusCode status = getFileSize("./TestDir/TestFile.txt", &size);
    ck_assert_int_eq(status, UA_STATUSCODE_GOOD);

    // "HelloWorld" = 10 bytes
    ck_assert_uint_eq(size, 10);
}
END_TEST
/* ------------------------------------------------------------------------- */
/* FIXTURES                                                                  */
/* ------------------------------------------------------------------------- */

static void setup(void) {
    deleteRecursive("./TestDir");
    deleteRecursive("./TestDir2");
    deleteRecursive("./TestFile.txt");
}

static void teardown(void) {
    deleteRecursive("./TestDir");
    deleteRecursive("./TestDir2");
    deleteRecursive("./TestFile.txt");
}

/* ------------------------------------------------------------------------- */
/* MAIN                                                                      */
/* ------------------------------------------------------------------------- */
int main(void) {
    setup();
    Suite *s = suite_create("Filesystem Operations");

    TCase *tc_core = tcase_create("Core");
    // tcase_add_checked_fixture(tc_core, setup, teardown);

    tcase_add_test(tc_core, make_directory);
    tcase_add_test(tc_core, make_file);
    tcase_add_test(tc_core, is_directory);
    tcase_add_test(tc_core, copy_item);
    tcase_add_test(tc_core, move_item);

    // FILE OP TESTS
    tcase_add_test(tc_core, open_file_test);
    tcase_add_test(tc_core, close_file_test);
    tcase_add_test(tc_core, write_file_test);
    tcase_add_test(tc_core, read_file_test);
    tcase_add_test(tc_core, seek_and_position_test);
    tcase_add_test(tc_core, get_file_size_test);

    tcase_add_test(tc_core, delete_item);
    teardown();

    // tcase_add_test(tc_core, test_full_sequence);
    suite_add_tcase(s, tc_core);

    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);

    int number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

#endif /* UA_FILESYSTEM */
