#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include "inode_file_system.h"

#define IP "10.0.8.17"
#define PORT 7000

void print_err(char* str, int line, int err_no) {
    printf("%d, %s: %s\n", line, str, strerror(err_no));
    _exit(-1);
}
pthread_rwlock_t rwlocks[INODE_NUMBER];//rwlock

pthread_mutex_t create_mutex;

// Function declarations
bool authenticateUser(char* id, char* password, char* role);
bool createUser(char* id, char* password, char* role);
void handleLogin(intptr_t cfd, char* id, char* password);
void handleCreateUser(intptr_t cfd, char* newid, char* newPassword, char* newRole);
void handleListByPath(intptr_t cfd, char* path);
void handleReadFileByDir(intptr_t cfd, char* path);
void handleWriteFileByDir(intptr_t cfd, char* path, char* type, char* content);
void update_inode_permissions_for_new_user(const char* id, const char* role);
void handleCreateDirectory(intptr_t cfd,char* courseName, char* id);
void handleDeleteDirectory(intptr_t cfd,char* courseName);
void handleGrant(intptr_t cfd,char* courseName, char* id, int permission);
void handleRevoke(intptr_t cfd,char* courseName, char* id, int permission);
void handleSaveSnapshot(intptr_t cfd);
void handleRestoreFromSnapshot(intptr_t cfd);
void handleAssign(intptr_t cfd, char* id, char* courseName,char* homework, char* Readme);
void handleCheckCourseDir(intptr_t cfd, char* id, char* courseName);
void handleCheckHomeworkDir(intptr_t cfd, char* id, char* courseName, char* homeworkName);
void handleCheckfileNameInIdDir(intptr_t cfd, char* id,char* courseName,char* homeworkName);
void handleCheckfileNameInIdDirForTeacher(intptr_t cfd, char* courseName, char* homeworkName, char* submitName);
void handleCheckFile(intptr_t cfd, char* id,char* courseName,char* homeworkName, char* fileName);
void handleCheckReadme(intptr_t cfd,char* id,char* courseName,char* homeworkName);
void handleCheckFileInIdDir(intptr_t cfd,char* id, char* courseName, char* homeworkName, char* fileNameInIdDir);
void handleCheckFileInIdDirForTeacher(intptr_t cfd,char* courseName,char* homeworkName,char* submitName,char* fileNameInIdDir);
void handleMarkFile(intptr_t cfd, char* id,char* courseName, char* homeworkName,char* fileName,char* content,char* appendOrOverwrite);
void handleMarkReadme(intptr_t cfd,char* id,char* courseName,char* homeworkName,char* content,char* appendOrOverwrite);
void handleMarkFileInIdDirForTeacher(intptr_t cfd,char* courseName,char* homeworkName,char* fileName,char* fileNameInIdDir,char* content,char* appendOrOverwrite);
void handleSubmitHomework(intptr_t cfd,char* id,char* courseName,char* homeworkName,char* submitName,char* content);
void* handleClient(void* pth_arg);
int getlock (char* path);

int getlock (char* inputPath){
    char path[256];
    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0'; // Ensure null-termination

    char* directory, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem; // Start from root inode
    struct DirectoryBlock* block;

    // Split the path and traverse each component
    parent = NULL;
    directory = strtok(path, delimiter);
    while (directory != NULL) {
        // Check each directory in the path
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            int found = 0;
            for (int i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    found = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (!found) {
                return -1; // Path component not found
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            return block->inodeID[i]; // 
        }
    }

    return -1; // 
}

bool authenticateUser(char* id, char* password, char* role) {
    for (int i = 0; i < userCount; ++i) {
        if (strcmp(users[i].id, id) == 0 &&
            strcmp(users[i].password, password) == 0) {
            strcpy(role, users[i].role);
            return true; // Authentication successful
        }
    }
    return false; // Authentication failed
}

void update_inode_permissions_for_new_user(const char* id, const char* role) {
    for (int i = 0; i < INODE_NUMBER; i++) {
        if (strcmp(role, "teacher") == 0) {
            for (int j = 0; j < NUM_USERS; j++) {
                if (inodeMem[i].permissions.teacher_perm[j].id[0] == '\0') { // Empty slot
                    strcpy(inodeMem[i].permissions.teacher_perm[j].id, id);
                    inodeMem[i].permissions.teacher_perm[j].permission = 0; // Default permission
                    break;
                }
            }
        } else if (strcmp(role, "student") == 0) {
            for (int j = 0; j < NUM_USERS; j++) {
                if (inodeMem[i].permissions.student_perm[j].id[0] == '\0') { // Empty slot
                    strcpy(inodeMem[i].permissions.student_perm[j].id, id);
                    inodeMem[i].permissions.student_perm[j].permission = 0; // Default permission
                    break;
                }
            }
        }
    }
}

bool createUser(char* id, char* password, char* role) {
    if (userCount >= MAX_USERS) {
        printf("Unable to create a new user, reached the maximum number of users\n");
        return false;
    }

    for (int i = 0; i < userCount; ++i) {
        if (strcmp(users[i].id, id) == 0) {
            printf("Unable to create a new user, the id already exists\n");
            return false;
        }
    }

    strcpy(users[userCount].id, id);
    strcpy(users[userCount].password, password);
    strcpy(users[userCount].role, role);
    userCount++;

    // Now create a directory for the user based on their role
    char path[256];
    if (strcmp(role, "teacher") == 0) {
        
        sprintf(path, "/root/Teacher/%s", id); // Constructs the directory path for the teacher
        pthread_mutex_lock(&create_mutex);
        createDirectory(path, "admin"); // You may want to check for success or failure
        pthread_mutex_unlock(&create_mutex);
    } else if (strcmp(role, "student") == 0) {
        sprintf(path, "/root/Student/%s", id); // Constructs the directory path for the student
        pthread_mutex_lock(&create_mutex);
        createDirectory(path, "admin"); // You may want to check for success or failure
        pthread_mutex_unlock(&create_mutex);
    }
    update_inode_permissions_for_new_user(id, role);
    
    printf("Successfully created a new user\n");
    return true;
}

void handleLogin(intptr_t cfd, char* id, char* password) {
    char result[256];
    if (authenticateUser(id, password, result)) {
        // Authentication successful
        printf("User '%s' authenticated successfully\n", id);
    }
    else {
        // Authentication failed
        printf("User '%s' authentication failed\n", id);
        strcpy(result, "Login failed. Please check your credentials.");
    }

    // Send the authentication result to the client
    send(cfd, result, sizeof(result), 0);
}

void handleListByPath(intptr_t cfd, char* path){
    char* result = listFiles(path);

    // Send the file listing result to the client
    send(cfd, result, strlen(result) + 1, 0); // +1 for null-terminator
}

void handleReadFileByDir(intptr_t cfd, char* path){
    char result[256]; // Buffer to hold the copy of the file content
    if(judgeIn(path)){
        if(isFile(path)){
            int lock_id = getlock(path);
            pthread_rwlock_rdlock(&rwlocks[lock_id]);
            char* temp = readFile(path); // Read the file
            pthread_rwlock_unlock(&rwlocks[lock_id]);
            if (temp != NULL) {
                strncpy(result, temp, sizeof(result)); // Safe copy
                result[sizeof(result) - 1] = '\0'; // Null-terminate to avoid overflow
            }
        }else{
            printf("admin enter a directory path, not a file path.\n");
            strcpy(result, "Sorry, you enter the directory path. Please enter a file path");
        }
    }
    else{
        printf("A wrong path is entered.\n");
        strcpy(result, "Sorry, you enter the wrong path. Please enter again");
    }
    send(cfd, result, sizeof(result), 0);
}
void handleWriteFileByDir(intptr_t cfd, char* path, char* type, char* content){
    bool shouldAppend;
    shouldAppend = true;
    char result[256]; // Buffer to hold the copy of the file content
    if(judgeIn(path)){
        if(isFile(path)){
                        int lock_id = getlock(path);
            printf("WriteFileByDir: %d\n",lock_id);
            pthread_rwlock_wrlock(&rwlocks[lock_id]);
            sleep(60);
            if(strcmp(type, "append") == 0){
                writeFile(path, content, shouldAppend);
            }else if(strcmp(type, "overwrite") == 0){
                shouldAppend = false;
                writeFile(path,content,shouldAppend);
            }else{
                printf("admin enter neither append or overwrite\n");
                strcpy(result, "Sorry, please enter either append or overwrite.");
            }
            pthread_rwlock_unlock(&rwlocks[lock_id]);
        }else{
            printf("admin enter a directory path, not a file path.\n");
            strcpy(result, "Sorry, you enter the directory path. Please enter a file path");
        }
    }
    else{
        printf("A wrong path is entered.\n");
        strcpy(result, "Sorry, you enter the wrong path. Please enter again");
    }
    send(cfd, result, sizeof(result), 0);
}
void handleCreateUser(intptr_t cfd, char* newid, char* newPassword, char* newRole) {
    char result[256];
    if (createUser(newid, newPassword, newRole)) {
        // User creation successful
        printf("Successfully created user '%s'\n", newid);
        strcpy(result, "User created successfully!");
    }
    else {
        // User creation failed
        printf("Failed to create user '%s'\n", newid);
        strcpy(result, "User creation failed. Please check the id and try again.");
    }

    // Send the user creation result to the client
    send(cfd, result, sizeof(result), 0);
}
// void createDirectory(char* inputPath, const char* role)
void handleCreateDirectory(intptr_t cfd,char* courseName, char* id){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s", courseName);
    pthread_mutex_lock(&create_mutex);
    if(createDirectory(path,id)){
        printf("Successfully created directory '%s'\n", courseName);
        strcpy(result, "Directory created successfully!");
    }
    else{
        //Directory creation falied
        printf("Failed to create directory '%s'\n", courseName);
        strcpy(result, "Directory creation failed. Please check the path and try again.");
    }
    pthread_mutex_unlock(&create_mutex);
    // Send the Directory creation result to the client
    send(cfd, result, sizeof(result), 0);
}

void handleDeleteDirectory(intptr_t cfd,char* courseName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s", courseName);
    int lock_id = getlock(path);
    pthread_rwlock_wrlock(&rwlocks[lock_id]);
    if(deleteDirectory(path)){
        printf("Successfully deleted directory '%s'\n", courseName);
        strcpy(result, "Directory deleted successfully!");
    }
    else{
        printf("Failed to deleted directory '%s'\n", courseName);
        strcpy(result, "Directory deletion failed. Please check the path and try again.");
    }
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    // Send the Directory creation result to the client
    send(cfd, result, sizeof(result), 0);
}

void handleGrant(intptr_t cfd,char* courseName, char* id, int permission){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s", courseName);
    if(changeAuthority(path,id,permission)){
        printf("Successfully grant course '%s' access to '%s'\n", courseName, id);
        strcpy(result, "Course Access granted successfully!");
    }
    else{
        printf("Failed to grant course '%s' access to '%s'\n", courseName, id);
        strcpy(result, "Course Access granting failed. Please check the path and try again.");
    }
    // Send the Directory creation result to the client
    send(cfd, result, sizeof(result), 0);
}
void handleRevoke(intptr_t cfd,char* courseName, char* id, int permission){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s", courseName);
    if(changeAuthority(path,id,permission)){
        printf("Successfully revoke course '%s' access to '%s'\n", courseName, id);
        strcpy(result, "Course Access revoked successfully!");
    }
    else{
        printf("Failed to grant course '%s' access to '%s'\n", courseName, id);
        strcpy(result, "Course Access revoking failed. Please check the path and try again.");
    }
    // Send the Directory creation result to the client
    send(cfd, result, sizeof(result), 0);
}
void handleSaveSnapshot(intptr_t cfd){
    char result[256];
    for (int i =0;i<INODE_NUMBER;i++)
    {
        pthread_rwlock_wrlock(&rwlocks[i]);
    }
    if(saveSnapshot("filesystem_snapshot.bin")){
        printf("Successfully saved on server!");
        strcpy(result, "Successfully saved on server!");
    }
    else{
        printf("Fail to save on server!");
        strcpy(result, "Fail to save on server!");
    }
    for (int i =0;i<INODE_NUMBER;i++)
    {
        pthread_rwlock_unlock(&rwlocks[i]);
    }
    send(cfd, result, sizeof(result), 0);
}
void handleRestoreFromSnapshot(intptr_t cfd){
    char result[256];
    for (int i =0;i<INODE_NUMBER;i++)
    {
        pthread_rwlock_wrlock(&rwlocks[i]);
    }
    if(restoreFromSnapshot("filesystem_snapshot.bin")){
        printf("Successfully restored on server!");
        strcpy(result, "Successfully restored on server!");
    }
    else{
        printf("Fail to restore on server!");
        strcpy(result, "Fail to restore on server!");
    }
    for (int i =0;i<INODE_NUMBER;i++)
    {
        pthread_rwlock_unlock(&rwlocks[i]);
    }
    send(cfd, result, sizeof(result), 0);
}
void handleAssign(intptr_t cfd, char* id, char* courseName,char* homework, char* Readme){
    char result[256];
    char coursePath[256];
    char path[256];
    char filePath[256];
    sprintf(coursePath, "/root/Course/%s", courseName);
    sprintf(path, "/root/Course/%s/%s", courseName, homework);
    sprintf(filePath, "/root/Course/%s/%s/Readme", courseName, homework);
    if(has_read_Authority(coursePath, id)){
        if(has_write_Authority(coursePath, id)){
            pthread_mutex_lock(&create_mutex);
            if(createDirectory(path, id)){
                createFile(filePath, id);
                writeFile(filePath, Readme, false);
                printf("Successfully assign '%s' in '%s' by '%s'\n",homework, courseName, id);
                strcpy(result, "Assign Homework successfully!");
            }
            else{
                printf("Failed to assign '%s' in '%s'\n", homework, courseName);
                strcpy(result, "Assign Homework failed.");
            }
            pthread_mutex_unlock(&create_mutex);
        }
        else{
            printf("'%s' do not have write permission for '%s'\n", id, courseName);
            strcpy(result, "Sorry, you do not have write permission for this course. Please contact the administrator to grant you permission.");
        }
    }
    else{
        printf("'%s' do not have read permission for '%s'\n", id, courseName);
        strcpy(result, "Sorry, you do not have read permission for this course. Please contact the administrator to grant you permission.");
    }
    send(cfd, result, sizeof(result), 0);
}
void handleCheckCourseDir(intptr_t cfd, char* id, char* courseName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s", courseName);
    int lock_id = getlock(path);
    // printf("CheckCourseDir: %d\n",lock_id);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    if(judgeIn(path)){
        if(has_read_Authority(path, id)){
            printf("'%s' has read permission to '%s'\n",id, courseName);
            strcpy(result, listFiles(path));
        }
        else{
            printf("'%s' do not have read permission to '%s'\n",id, courseName);
            strcpy(result, "Sorry, you do not have read permission for this course. Please contact the administrator to grant you permission.");
        }
    }
    else{
        printf("'%s' enter the wrong CourseName.\n",id);
        strcpy(result, "Sorry, you enter the wrong CourseName. Please enter again");
    }
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}
void handleCheckHomeworkDir(intptr_t cfd, char* id, char* courseName, char* homeworkName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s", courseName,homeworkName);
    int lock_id = getlock(path);
    printf("CheckHomeworkDir: %d\n",lock_id);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    if(judgeIn(path)){
        if(has_read_Authority(path, id)){
            printf("'%s' has read permission to '%s'\n",id, courseName);
            strcpy(result, listFiles(path));
        }
        else{
            printf("'%s' do not have read permission to '%s' in '%s'\n",id, homeworkName,courseName);
            strcpy(result, "Sorry, you do not have read permission for this course. Please contact the administrator to grant you permission.");
        }
    }
    else{
        printf("'%s' enter the wrong path.\n",id);
        strcpy(result, "Sorry, you enter the wrong path. Please enter again");
    }
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}
void handleCheckfileNameInIdDir(intptr_t cfd, char* id,char* courseName,char* homeworkName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/%s", courseName,homeworkName,id);
    int lock_id = getlock(path);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    
        strcpy(result, listFiles(path));
    
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}
void handleCheckfileNameInIdDirForTeacher(intptr_t cfd, char* courseName, char* homeworkName, char* submitName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/%s", courseName,homeworkName,submitName);
    int lock_id = getlock(path);
    // printf("CheckFile: %d\n",lock_id);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    char* dirContent = listFiles(path);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    if (dirContent != NULL) {
        sprintf(result, "%s", dirContent);
    } else {
        // Handle the case where readFile returns NULL (file not found or read error)
        strcpy(result, "Error reading directory");
    }
    

    send(cfd, result, sizeof(result), 0);
}
void handleCheckFile(intptr_t cfd, char* id,char* courseName,char* homeworkName, char* fileName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/%s", courseName,homeworkName,fileName);
    int lock_id = getlock(path);
    // printf("CheckFile: %d\n",lock_id);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    if(has_read_Authority(path, id)){
        printf("'%s' has read permission to '%s' in '%s' in '%s'\n",id, fileName,homeworkName,courseName);
        strcpy(result, readFile(path));
    }
    else{
        printf("'%s' do not have read permission to '%s'  in '%s' in '%s'\n",id, fileName,homeworkName,courseName);
        strcpy(result, "Sorry, you do not have read permission for this File. Please contact the administrator to grant you permission.");
    }
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}
void handleCheckReadme(intptr_t cfd,char* id,char* courseName,char* homeworkName){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/Readme", courseName,homeworkName);
    int lock_id = getlock(path);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    if(has_read_Authority(path, id)){
        printf("'%s' has read permission to Readme in '%s' in '%s'\n",id,homeworkName,courseName);
        // Assuming readFile(path) returns a pointer to a string that contains the file content
        char* fileContent = readFile(path);
        if (fileContent != NULL) {
            sprintf(result, "Readme: %s", fileContent);
        } else {
            // Handle the case where readFile returns NULL (file not found or read error)
            strcpy(result, "Readme: Error reading file");
        }
    }
    else{
        printf("'%s' do not have read permission to Readme  in '%s' in '%s'\n",id,homeworkName,courseName);
        strcpy(result, "Sorry, you do not have read permission for this File. Please contact the administrator to grant you permission.");
    }
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}
void handleCheckFileInIdDir(intptr_t cfd,char* id, char* courseName, char* homeworkName, char* fileNameInIdDir){
    char result[256];
    char path[256];
    int lock_id = getlock(path);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
    sprintf(path, "/root/Course/%s/%s/%s/%s", courseName,homeworkName,id,fileNameInIdDir);
        strcpy(result, readFile(path));
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}
void handleCheckFileInIdDirForTeacher(intptr_t cfd,char* courseName,char* homeworkName,char* submitName,char* fileNameInIdDir){
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/%s/%s", courseName,homeworkName,submitName,fileNameInIdDir);
    int lock_id = getlock(path);
    pthread_rwlock_rdlock(&rwlocks[lock_id]);
        strcpy(result, readFile(path));
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}

void handleMarkReadme(intptr_t cfd,char* id,char* courseName,char* homeworkName,char* content,char* appendOrOverwrite){
    bool shouldAppend;
    // Initialize to a default value
    shouldAppend = false;

    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/Readme", courseName,homeworkName);
    if(has_write_Authority(path, id)){
        printf("'%s' has write permission to Readme in '%s' in '%s'\n",id,homeworkName,courseName);
        int lock_id = getlock(path);
        pthread_rwlock_wrlock(&rwlocks[lock_id]);
        printf("'%s' has write permission to Readme in '%s' in '%s'\n",id,homeworkName,courseName);
            // Check the received value
        if (strcmp(appendOrOverwrite, "append") == 0) {
            shouldAppend = true;
            writeFile(path,content,shouldAppend);
            printf("'%s' have write permission to Readme in '%s' in '%s' and choose to append\n",id,homeworkName,courseName);
            strcpy(result, "Successfully append the content in Readme");
        } else if (strcmp(appendOrOverwrite, "overwrite") == 0) {
            shouldAppend = false;
            writeFile(path,content,shouldAppend);
            printf("'%s' have write permission to Readme in '%s' in '%s' and choose to overwrite\n",id,homeworkName,courseName);
            strcpy(result, "Successfully overwrite the content in Readme");
        }else{
            printf("'%s' enter neither append or overwrite\n",id);
            strcpy(result, "Sorry, please enter either append or overwrite.");
        }
        pthread_rwlock_unlock(&rwlocks[lock_id]);
    }
    else{
        printf("'%s' do not have write permission to Readme in '%s' in '%s'\n",id,homeworkName,courseName);
        strcpy(result, "Sorry, you do not have write permission for this File. Please contact the administrator to grant you permission.");
    }
    send(cfd, result, sizeof(result), 0);
}
void handleMarkFileInIdDirForTeacher(intptr_t cfd,char* courseName,char* homeworkName,char* fileName,char* fileNameInIdDir,char* content,char* appendOrOverwrite ){
    bool shouldAppend;
    // Initialize to a default value
    shouldAppend = false;
    char result[256];
    char path[256];
    sprintf(path, "/root/Course/%s/%s/%s/%s", courseName,homeworkName,fileName,fileNameInIdDir);
        // Check the received value
    int lock_id = getlock(path);
    pthread_rwlock_wrlock(&rwlocks[lock_id]);
    if (strcmp(appendOrOverwrite, "append") == 0) {
        shouldAppend = true;
        writeFile(path,content,shouldAppend);
        sprintf(result, "Successfully append the content in '%s'", fileNameInIdDir);
    } else if (strcmp(appendOrOverwrite, "overwrite") == 0) {
        shouldAppend = false;
        writeFile(path,content,shouldAppend);
        sprintf(result, "Successfully overwrite the content in '%s'", fileNameInIdDir);
    }else{
        printf("A teacher enter neither append or overwrite\n");
        strcpy(result, "Sorry, please enter either append or overwrite.");
    }
    pthread_rwlock_unlock(&rwlocks[lock_id]);
    send(cfd, result, sizeof(result), 0);
}

void handleSubmitHomework(intptr_t cfd, char* id, char* courseName, char* homeworkName, char* submitName, char* content) {
    char result[256];
    char studentFolderPath[256];
    char filePath[256];
    char coursePath[256];
    sprintf(coursePath, "/root/Course/%s", courseName); // 课程文件夹路径
    sprintf(studentFolderPath, "/root/Course/%s/%s/%s", courseName, homeworkName, id); // 学生个人文件夹路径
    sprintf(filePath, "/root/Course/%s/%s/%s/%s", courseName, homeworkName, id, submitName); // 提交作业文件路径

    // 检查学生是否有读权限来提交作业
    if (has_read_Authority(coursePath, id)) {
        // 如果学生个人文件夹不存在，则创建
        if (!judgeIn(studentFolderPath)) {
            pthread_mutex_lock(&create_mutex);
            if (!createDirectory(studentFolderPath, id)) {
                printf("Failed to create personal folder for '%s'\n", id);
                strcpy(result, "Failed to create personal folder. Please contact the administrator.");
                send(cfd, result, sizeof(result), 0);
                return;
            }
            pthread_mutex_unlock(&create_mutex);
        }

        // 创建或覆盖学生的作业文件
        pthread_mutex_lock(&create_mutex);
        if (createFile(filePath, id)) {
            // 文件已成功创建，现在写入内容
            int lock_id = getlock(filePath);
            pthread_rwlock_wrlock(&rwlocks[lock_id]);
            writeFile(filePath, content, false); // 假设此函数覆盖文件内容
            pthread_rwlock_unlock(&rwlocks[lock_id]);
            printf("'%s' created file '%s' successfully\n", id, submitName);
            strcpy(result, "Submission succeeded.");
        } else {
            printf("'%s' failed to create file '%s'\n", id, submitName);
            strcpy(result, "Sorry, failed to create the submission file.");
        }
        pthread_mutex_unlock(&create_mutex);
    } else {
        printf("'%s' does not have permission to submit homework '%s' in course '%s'\n", id, homeworkName, courseName);
        strcpy(result, "Sorry, you do not have permission to submit this homework. Please contact the administrator.");
    }

    // 发送操作结果回客户端
    send(cfd, result, sizeof(result), 0);
}


void* handleClient(void* pth_arg) {
    intptr_t cfd = (intptr_t)pth_arg;
    int ret = -1;
    char buf[256] = { 0 };
    char id[256] = { 0 };
    while (1) {
        // Receive client requests
        ret = recv(cfd, buf, sizeof(buf), 0);
        if (-1 == ret) {
            print_err("recv failed", __LINE__, errno);
        }
        else if (ret == 0) {
            // Client closed the connection
            printf("Client has closed the connection\n");
            close(cfd);
            pthread_exit(NULL);
        }
        else {
            // Call the appropriate handling function based on the client's request
            if (strcmp(buf, "login") == 0) {
                // Receive id and password
                char password[256] = { 0 };
                ret = recv(cfd, id, sizeof(id), 0);
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd, password, sizeof(password), 0);
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                // Handle login logic
                handleLogin(cfd, id, password);
            }
            else if (strcmp(buf, "list_by_dir") ==0) {
                char path[256] = { 0 };
                ret = recv(cfd, path, sizeof(path), 0);
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }
                handleListByPath(cfd, path);
            }
            else if (strcmp(buf, "create_user") == 0) {
                // Handle create user request
                // Receive new user information
                char newid[256] = { 0 };
                char newPassword[256] = { 0 };
                char newRole[256] = { 0 };
                ret = recv(cfd, newid, sizeof(newid), 0);
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd, newPassword, sizeof(newPassword), 0);
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd, newRole, sizeof(newRole), 0);
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                // Handle create user logic
                handleCreateUser(cfd, newid, newPassword, newRole);
            }
            else if(strcmp(buf,"add_course")==0){
                char courseName[256] = { 0 };
                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                // Handle create directory logic
                handleCreateDirectory(cfd,courseName,id);
            }
            else if(strcmp(buf,"delete_course")==0){
                char courseName[256] = { 0 };
                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }


                handleDeleteDirectory(cfd,courseName);
            }
            else if(strcmp(buf,"grant")==0){
                char courseName[256] = { 0 };
                char ID[256] = { 0 };
                char permissionStr[256] = {0}; // Store permission as a string
                int permission; // Variable to store the integer value of permission
                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,ID,sizeof(ID),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd, permissionStr, sizeof(permissionStr), 0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                } else {
                permission = atoi(permissionStr); // Convert to integer
                handleGrant(cfd, courseName, ID, permission);
                }
            }
            else if(strcmp(buf,"revoke")==0){
                char courseName[256] = { 0 };
                char ID[256] = { 0 };
                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,ID,sizeof(ID),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }
                handleRevoke(cfd, courseName, ID, 0);
            }
            else if (strcmp(buf, "backup") == 0) {
                handleSaveSnapshot(cfd);
            }
            else if(strcmp(buf, "restore") == 0) {
                handleRestoreFromSnapshot(cfd);
            }
            else if(strcmp(buf,"Assign") == 0){
                char id[256] = { 0 };
                char courseName[256] = { 0 };
                char homework[256] = { 0 };
                char Readme[256] = { 0 };
		        char signal[256] = { 0 };

                ret = recv(cfd,id,sizeof(id),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

		        ret = recv(cfd,homework,sizeof(homework),0); 
	            if (-1 == ret) {
	                print_err("recv failed", __LINE__, errno);
		        }

		        ret = recv(cfd,Readme,sizeof(Readme),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
                }
                handleAssign(cfd, id, courseName, homework, Readme);
            }
            else if(strcmp(buf,"Read_File_by_dir") == 0){
                char path[256] = { 0 };

                ret = recv(cfd,path,sizeof(path),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                handleReadFileByDir(cfd, path);
            }
            else if(strcmp(buf,"Write_File_by_dir") == 0){
                char path[256] = { 0 };
                char type[256] = { 0 };
                char content[256] = { 0 };

                ret = recv(cfd,path,sizeof(path),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }
                ret = recv(cfd,type,sizeof(type),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }
                ret = recv(cfd,content,sizeof(content),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                handleWriteFileByDir(cfd, path, type, content);
            }

            else if(strcmp(buf,"teacherCheck") == 0){
                char id[256] = { 0 };
                char courseName[256] = { 0 };
                char homeworkName[256] = { 0 };
                char submitName[256] = { 0 };
                char fileNameInIdDir[256] = { 0 };
		        char signal[256] = { 0 };
                ret = recv(cfd,id,sizeof(id),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                handleCheckCourseDir(cfd, id, courseName);

		        ret = recv(cfd,signal,sizeof(signal),0); 
		        if(strcmp(signal,"failed") != 0){

		        ret = recv(cfd,homeworkName,sizeof(homeworkName),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
		        }

		        handleCheckHomeworkDir(cfd,  id,courseName,  homeworkName);

		        ret = recv(cfd,submitName,sizeof(submitName),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
		        }

                if(strcmp(submitName, "Readme") == 0) {
                        handleCheckReadme(cfd, id, courseName, homeworkName);
                    } else {
                        handleCheckfileNameInIdDirForTeacher(cfd, courseName, homeworkName, submitName);
                        ret = recv(cfd,fileNameInIdDir,sizeof(fileNameInIdDir),0); 
                        if (-1 == ret) {
                        print_err("recv failed", __LINE__, errno);
                        }
                        handleCheckFileInIdDirForTeacher(cfd, courseName, homeworkName, submitName, fileNameInIdDir);
                    }
		    }
            }
            else if(strcmp(buf,"teacherMark") == 0){
                char id[256] = { 0 };
                char courseName[256] = { 0 };
                char homeworkName[256] = { 0 };
                char fileName[256] = { 0 };
                char fileNameInIdDir[256] = { 0 };
                char appendOrOverwrite[256] = { 0 };
                char content[256] = { 0 };
		        char signal[256] = { 0 };

                ret = recv(cfd,id,sizeof(id),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                handleCheckCourseDir(cfd, id, courseName);
		
		        ret = recv(cfd,signal,sizeof(signal),0); 
		        if(strcmp(signal,"failed") != 0){

		        ret = recv(cfd,homeworkName,sizeof(homeworkName),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
		        }

		        handleCheckHomeworkDir(cfd,  id,courseName,  homeworkName);

		        ret = recv(cfd,fileName,sizeof(fileName),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
		        }

		
                if(strcmp(fileName, "Readme") == 0) {
                    	ret = recv(cfd,appendOrOverwrite,sizeof(appendOrOverwrite),0); 
                        if (-1 == ret) {
                            print_err("recv failed", __LINE__, errno);
                        }

                        ret = recv(cfd,content,sizeof(content),0); 
                        if (-1 == ret) {
                            print_err("recv failed", __LINE__, errno);
                        }

                        handleMarkReadme(cfd, id, courseName, homeworkName,content,appendOrOverwrite);
                    } else {
                        handleCheckfileNameInIdDirForTeacher(cfd, courseName, homeworkName, fileName);
                        ret = recv(cfd,fileNameInIdDir,sizeof(fileNameInIdDir),0); 
                        if (-1 == ret) {
                        print_err("recv failed", __LINE__, errno);
                        }
                        ret = recv(cfd,appendOrOverwrite,sizeof(appendOrOverwrite),0); 
                        if (-1 == ret) {
                            print_err("recv failed", __LINE__, errno);
                        }

                        ret = recv(cfd,content,sizeof(content),0); 
                        if (-1 == ret) {
                            print_err("recv failed", __LINE__, errno);
                        }
                        handleMarkFileInIdDirForTeacher(cfd, courseName, homeworkName, fileName, fileNameInIdDir,content,appendOrOverwrite);
                    }
        
        
        
        
        }
            }
            else if(strcmp(buf,"ViewReadme") == 0){
                char id[256] = { 0 };
                char courseName[256] = { 0 };
                char homeworkName[256] = { 0 };
                char signal[256] = { 0 };

                ret = recv(cfd,id,sizeof(id),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }
                handleCheckCourseDir(cfd, id, courseName);

		        ret = recv(cfd,signal,sizeof(signal),0); 
                

                if(strcmp(signal,"failed") != 0){

                    ret = recv(cfd,homeworkName,sizeof(homeworkName),0); 
                    if (-1 == ret) {
                        print_err("recv failed", __LINE__, errno);
                    }
                    handleCheckFile(cfd, id, courseName,homeworkName,"Readme");   
                }

            }
            else if(strcmp(buf,"studentSubmit") == 0){
                char id[256] = { 0 };
                char courseName[256] = { 0 };
                char homeworkName[256] = { 0 };
                char submitName[256] = { 0 };
                char content[256] = { 0 };
		char signal1[256] = { 0 };
		char signal2[256] = { 0 };

                ret = recv(cfd,id,sizeof(id),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                handleCheckCourseDir(cfd, id, courseName);
		
		ret = recv(cfd,signal1,sizeof(signal1),0); 

		if(strcmp(signal1,"failed") != 0){
                    ret = recv(cfd,homeworkName,sizeof(homeworkName),0); 
                    if (-1 == ret) {
                        print_err("recv failed", __LINE__, errno);
                    }

               	    handleCheckHomeworkDir(cfd,  id,courseName,  homeworkName); 
		    
		    ret = recv(cfd,signal2,sizeof(signal2),0); 

		    if(strcmp(signal2,"failed") != 0){
		        ret = recv(cfd,submitName,sizeof(submitName),0); 
                        if (-1 == ret) {
                            print_err("recv failed", __LINE__, errno);
                        }
                        ret = recv(cfd,content,sizeof(content),0); 
                        if (-1 == ret) {
                            print_err("recv failed", __LINE__, errno);
                        }
                        handleSubmitHomework(cfd, id, courseName,homeworkName,submitName,content);
		    }
		}

            }
            else if(strcmp(buf,"ViewHomework") == 0){



                char id[256] = { 0 };
                char courseName[256] = { 0 };
                char homeworkName[256] = { 0 };
                char submitName[256] = { 0 };
                char fileNameInIdDir[256] = { 0 };
		        char signal[256] = { 0 };

                ret = recv(cfd,id,sizeof(id),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                ret = recv(cfd,courseName,sizeof(courseName),0); 
                if (-1 == ret) {
                    print_err("recv failed", __LINE__, errno);
                }

                handleCheckCourseDir(cfd, id, courseName);
		
		        ret = recv(cfd,signal,sizeof(signal),0); 

		        if(strcmp(signal,"failed") != 0){

		        ret = recv(cfd,homeworkName,sizeof(homeworkName),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
		        }

		        handleCheckHomeworkDir(cfd, id, courseName,homeworkName);

		        ret = recv(cfd,submitName,sizeof(submitName),0); 
		        if (-1 == ret) {
		            print_err("recv failed", __LINE__, errno);
		        }

                if(strcmp(submitName, "Readme") == 0) {
                        handleCheckReadme(cfd, id, courseName, homeworkName);
                    } else {
                        handleCheckfileNameInIdDir(cfd, id, courseName, homeworkName);
                        ret = recv(cfd,fileNameInIdDir,sizeof(fileNameInIdDir),0); 
                        if (-1 == ret) {
                        print_err("recv failed", __LINE__, errno);
                        }
                        handleCheckFileInIdDir(cfd, id, courseName, homeworkName, fileNameInIdDir);
                    }
		    }




            }
            else {
                // Unknown request, handle as needed
                printf("Unknown request: %s\n", buf);
            }
        }
    }
}

int main() {
    int i;
    for (i = 0; i < INODE_NUMBER; i++) {
        inodeMem[i].inodeNumber = i;
        inodeMem[i].blockID = -1;
        inodeMem[i].fileType = 0; // Assuming 0 is the default for uninitialized
        // Initialize all permissions to no access
        init_inode(&inodeMem[i]);
        pthread_rwlock_init(&rwlocks[i],NULL);//init rwlocks
    }
    pthread_mutex_init(&create_mutex,NULL);
    inodeMem[0].blockID = 0;
    inodeMem[0].fileType = 0; // Directory
    blockBitmap[0] |= 1 << 0; // Root directory block is used
    init_inode(&inodeMem[0]);
    blockBitmap[0] |= 1;
    struct DirectoryBlock* root = (struct DirectoryBlock*)&blockMem[0];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        root->inodeID[i] = -1;// This doesn't mean that inodeNumber is -1; rather, it means that the directory entries are initially empty or not pointing to any Inode. 
    }
    
    //增加原始数据
    createDirectory("/root", "admin");
    createDirectory("/root/Teacher", "admin");
    createDirectory("/root/Student", "admin");
    createDirectory("/root/Teacher/teacher1", "admin");
    update_inode_permissions_for_new_user("teacher1", "teacher");
    createDirectory("/root/Teacher/teacher2", "admin");
    update_inode_permissions_for_new_user("teacher2", "teacher");
    createDirectory("/root/Student/student1", "admin");
    update_inode_permissions_for_new_user("student1", "student");
    createDirectory("/root/Course", "admin");
    createDirectory("/root/Course/Calculus", "admin");
    createDirectory("/root/Course/Python", "admin");
    createDirectory("/root/Course/Java", "admin");
    changeAuthority("/root/Course/Calculus", "teacher1", PERM_READ|PERM_WRITE);
    if(has_write_Authority("/root/Course/Calculus","teacher1")){createDirectory("/root/Course/Calculus/Homework1", "teacher1");}
    //else{printf("teacher1 fail to create%d\n",1);}
    if(has_write_Authority("/root/Course/Calculus","teacher2")){createDirectory("/root/Course/Calculus/Homework2", "teacher2");}
    //else{printf("teacher2 fail%d\n",1);}
    createFile("/root/Course/Calculus/Homework1/Readme", "teacher1");
    char contentToWrite[] = "Hello World";
    writeFile("/root/Course/Calculus/Homework1/Readme", contentToWrite, true);

    int skfd = -1, ret = -1;
    skfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == skfd) {
        print_err("socket failed", __LINE__, errno);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;            // Set the TCP protocol family
    addr.sin_port = htons(PORT);          // Set the port number
    addr.sin_addr.s_addr = inet_addr(IP); // Set the IP address

    ret = bind(skfd, (struct sockaddr*)&addr, sizeof(addr));
    if (-1 == ret) {
        print_err("bind failed", __LINE__, errno);
    }

    ret = listen(skfd, 3);
    if (-1 == ret) {
        print_err("listen failed", __LINE__, errno);
    }

    intptr_t cfd = -1;
    while (1) {
        
        struct sockaddr_in caddr = { 0 };
        int csize = sizeof(caddr);
        cfd = accept(skfd, (struct sockaddr*)&caddr, &csize);
        if (-1 == cfd) {
            print_err("accept failed", __LINE__, errno);
        }
        pthread_t id;
        // Create a child thread to handle client requests
        int ret = pthread_create(&id, NULL, handleClient, (void*)cfd);
        if (-1 == ret) print_err("pthread_create failed", __LINE__, errno);
    }
    // The main thread can perform other tasks or wait for the child thread to finish

    return 0;
}
