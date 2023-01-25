#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

typedef struct {
    size_t length; /* The length of the string */
    uint8_t *data; /* The content (not null-terminated) */
} string;

// structure for user Data Queue
typedef struct user_data_buffer {
    bool userAvailable;
    string username;
    string password;
    string role;
    uint64_t GroupID;
    uint64_t userID;
} userDatabase;

int checkUser (userDatabase* userData){
    // reading line by line, max 256 bytes
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];
    char *filename = "/opt/userDatabase.txt";

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        printf("Error: could not open file %s", filename);
        return 1;
    }

    int databaseIndex = 0;
    memset(buffer, 0, MAX_LENGTH);
    while (fgets(buffer, MAX_LENGTH, fp))
    {
        char *ptr;
        ptr = strtok (buffer, ",");
        while (ptr != NULL) {
            if (databaseIndex == 0){
                if (strncmp(userData->username.data, ptr, userData->username.length) == 0)
                    printf("User found Succesfully\n");
                else{
                    userData->role.length = 0;
                    userData->GroupID = 0;
                    userData->userID = 0;
                    break;
                }
            }
            else if (databaseIndex == 1)
            {
                if (strncmp(userData->password.data, ptr, userData->password.length) == 0){
                    printf("Password correct\n");
                    userData->userAvailable = true;
                }
                else{
                    userData->role.length = 0;
                    userData->GroupID = 0;
                    userData->userID = 0;
                    break;
                }
            }
            else if ((databaseIndex == 2) && (userData->userAvailable == true)){
                userData->role.data = (uint8_t*)malloc(strlen(ptr));
                strncpy(userData->role.data, ptr, strlen(ptr));
                userData->role.length = strlen(ptr);
            }
            else if ((databaseIndex == 3) && (userData->userAvailable == true)){
                int groupID = atoi(ptr);
                userData->GroupID = groupID;
            }
            else if ((databaseIndex == 4) && (userData->userAvailable == true)){
                int userID = atoi(ptr);
                userData->userID = userID;
            }

            ptr = strtok (NULL, ",");
            databaseIndex++;
        }

        databaseIndex = 0;
        if (userData->userAvailable == true)
            break;
    }
    // close the file
    fclose(fp);

    return 0;
}