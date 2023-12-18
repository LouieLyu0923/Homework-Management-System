#ifndef INODE_FILE_SYSTEM_H
#define INODE_FILE_SYSTEM_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define BLOCK_SIZE 512
#define MAX_FILENAME_LENGTH 12
#define ENTRY_NUMBER 32
#define BLOCK_NUMBER 128
#define INODE_NUMBER 128
#define NUM_USERS 100  // 假设每种类型的用户都有100个
#define MAX_USERS 100
#define MAX_CONTENT_SIZE 1024  // Define a maximum size for the content

// Permission bits for each role
#define PERM_READ  (1 << 0)//001=1
#define PERM_WRITE (1 << 1)//010=2


typedef struct {
    char id[256];
    char password[256];
    char role[256];
} User;
extern User users[MAX_USERS];
extern int userCount;


typedef struct {
    char id[NUM_USERS];
    int permission;
} student_p;

typedef struct {
    char id[NUM_USERS];
    int permission;
} teacher_p;

struct Permission {
    unsigned int root_perm; //0是无权限，1是读权限，2是写权限，3是读写权限
    teacher_p teacher_perm[NUM_USERS];   
    student_p student_perm[NUM_USERS];   
};
struct Inode {
    int inodeNumber;  // 索引节点号
    int blockID;  // 数据块号
    int fileType;  // 文件类型：0表示目录，1表示普通文件
    struct Permission permissions;   // Permissions for the inode
};

struct DirectoryBlock {
    char fileName[ENTRY_NUMBER][MAX_FILENAME_LENGTH];  // 文件名数组
    int inodeID[ENTRY_NUMBER];  // 索引节点号数组
    //必须保证inodeMem[inodeID[i]]
};

struct FileBlock {
    char content[BLOCK_SIZE];  // 文件内容
};

struct Inode inodeMem[INODE_NUMBER];  // 索引节点内存
struct FileBlock blockMem[BLOCK_NUMBER];  // 数据块内存
char blockBitmap[BLOCK_NUMBER / 8];  // 数据块位图

void init_inode(struct Inode *inode);
struct Inode* find_inode_by_path(const char* inputPath);
bool has_read_Authority(const char* inputPath, const char* userID);
bool has_write_Authority(const char* inputPath, const char* userID);
bool changePermission(int InodeID, const char* userId, int newPermission);
// Update the declaration in the header file (if any) and the definition to include the 'startChanging' parameter
bool recursiveChange(int InodeID, const char* userId, int newPermission);
bool changeAuthority(char* inputPath, const char* userId, int newPermission);
bool createDirectory(char *inputPath, const char* userID);
void deleteFileByInode(int inodeID);
void recursiveDelete(int inodeID);
bool deleteDirectory(char *inputPath);
char* listFiles(char *inputPath);
bool createFile(char *inputPath, const char* userID);
void deleteFile();
char* readFile(char *inputPath);
void writeFile(char *inputPath, const char* content, bool append);
bool saveSnapshot(const char* snapshotFile);
bool restoreFromSnapshot(const char* snapshotFile);
void add_(char *dir, char *name);
void delete_(char *dir, char *name);
bool judgeIn(char *inputPath);
bool isFile(char *inputPath);
void syncPermissions(int parentInodeID, int newInodeID);
#endif