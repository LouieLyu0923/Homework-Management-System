#include "inode_file_system.h"

//关于各个结构体的关系
//总之，inodeID 用于识别和访问特定的 Inode 结构体，
//而 blockID 是 Inode 结构体中的字段，用于指向包含文件内容或目录列表的实际数据块。
//通过 inodeID 可以找到其对应的 Inode，
//然后通过 Inode 的 blockID 可以找到存储数据的 FileBlock 或 DirectoryBlock。
//这是一个两级查找过程，首先通过 inodeID 定位索引节点，然后通过索引节点中的 blockID 定位数据。

//inodeID[i] 表示的是在某个目录中第 i 个条目关联的 inode 的编号（也就是这个 inode 的int标识符），它用于在 inodeMem 数组中查找对应的 Inode 结构体。
//也就是     x=inodeID[i]     Inode结构体=inodeMem[x]
//在一些简单的文件系统实现中（比如我们的文件系统 //main初始化inodeMem[i].inodeNumber = i;），inodeMem 数组的索引 i 通常会与存储在该位置的 Inode 结构体的 inodeNumber 字段的值相同，这样做是为了简化设计。也就是说，inodeMem[i] 的 inodeNumber 通常等于 i。
//但是，这不是文件系统的要求，只是一种常见的简化实践。在更复杂或者更高级的文件系统中，inodeNumber 可能会是一个全局唯一的标识符，它不一定与 inode 存储在 inodeMem 数组中的索引位置相同。

User users[MAX_USERS] = {
    {"admin", "adminpass", "admin"},
    {"teacher1", "teacherpass", "teacher"},
    {"student1", "studentpass", "student"},
    {"teacher2", "teacherpass", "teacher"}
};
// Number of users
int userCount = 4;

void init_inode(struct Inode *inode) {
    inode->permissions.root_perm = PERM_READ|PERM_WRITE; // 管理员权限

    // 重置所有权限为无
    for (int i = 0; i < NUM_USERS; i++) {
        strcpy(inode->permissions.teacher_perm[i].id, "");
        inode->permissions.teacher_perm[i].permission = 0;
        strcpy(inode->permissions.student_perm[i].id, "");
        inode->permissions.student_perm[i].permission = 0;
    }

    // 遍历用户数组，设置权限
    for (int i = 0; i < MAX_USERS; i++) {
        if (strcmp(users[i].role, "teacher") == 0) {
            // 如果是教师，设置相应的权限
            for (int j = 0; j < NUM_USERS; j++) {
                if (inode->permissions.teacher_perm[j].id[0] == '\0') {
                    strcpy(inode->permissions.teacher_perm[j].id, users[i].id);
                    // 设置默认教师权限
                    inode->permissions.teacher_perm[j].permission = 0;
                    break;
                }
            }
        } else if (strcmp(users[i].role, "student") == 0) {
            // 如果是学生，设置相应的权限
            for (int j = 0; j < NUM_USERS; j++) {
                if (inode->permissions.student_perm[j].id[0] == '\0') {
                    strcpy(inode->permissions.student_perm[j].id, users[i].id);
                    // 设置默认学生权限
                    inode->permissions.student_perm[j].permission = 0;
                    break;
                }
            }
        }
    }
}


// 查找路径对应的inode
struct Inode* find_inode_by_path(const char* inputPath) {
    char path[256];
    strcpy(path, inputPath);  // 复制路径以便操作

    struct Inode* current_inode = &inodeMem[0]; // 假设0是根目录的inode
    char* part = strtok(path, "/");

    while (part != NULL) {
        if (current_inode->fileType != 0) {  // 检查是否是目录
            printf("Error: '%s' is not a directory.\n", part);
            return NULL;
        }

        struct DirectoryBlock* block = (struct DirectoryBlock*)&blockMem[current_inode->blockID];
        int found = 0;

        for (int i = 0; i < ENTRY_NUMBER; i++) {
            if (strcmp(block->fileName[i], part) == 0) {
                current_inode = &inodeMem[block->inodeID[i]];
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("Error: '%s' not found in path.\n", part);
            return NULL;
        }

        part = strtok(NULL, "/");
    }

    return current_inode;
}

// Function to check if a user has read authority on a file or directory at inputPath
bool has_read_Authority(const char* inputPath, const char* userID) {
    struct Inode* inode = find_inode_by_path(inputPath);
    if (!inode) {
        printf("Error: Unable to find inode for the given path.\n");
        return 0; // Return 0 if inode is not found
    }


    // Check for teacher permissions
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(inode->permissions.teacher_perm[i].id, userID) == 0 &&
            inode->permissions.teacher_perm[i].permission & PERM_READ) {
            return 1; // Teacher has read access
        }
    }

    // Check for student permissions
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(inode->permissions.student_perm[i].id, userID) == 0 &&
            inode->permissions.student_perm[i].permission & PERM_READ) {
            return 1; // Student has read access
        }
    }

    return 0; // No read access
}

// Function to check if a user has write authority on a file or directory at inputPath
bool has_write_Authority(const char* inputPath, const char* userID) {
    struct Inode* inode = find_inode_by_path(inputPath);
    if (!inode) {
        printf("Error: Unable to find inode for the given path.\n");
        return 0; // Return 0 if inode is not found
    }

    // Check for teacher permissions
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(inode->permissions.teacher_perm[i].id, userID) == 0 &&
            inode->permissions.teacher_perm[i].permission & PERM_WRITE) {
            return 1; // Teacher has write access
        }
    }

    // Check for student permissions
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(inode->permissions.student_perm[i].id, userID) == 0 &&
            inode->permissions.student_perm[i].permission & PERM_WRITE) {
            return 1; // Student has write access
        }
    }

    return 0; // No write access
}






bool changePermission(int InodeID, const char* userId, int newPermission) {
    for (int i = 0; i < NUM_USERS; i++) {
        if (strcmp(inodeMem[InodeID].permissions.teacher_perm[i].id, userId) == 0) {
            inodeMem[InodeID].permissions.teacher_perm[i].permission = newPermission;
            printf("Permission for teacher '%s' changed to %d.\n", userId, newPermission);
            return true;
        }
        if (strcmp(inodeMem[InodeID].permissions.student_perm[i].id, userId) == 0) {
            inodeMem[InodeID].permissions.student_perm[i].permission = newPermission;
            printf("Permission for student '%s' changed to %d.\n", userId, newPermission);
            return true;
        }
    }
    printf("User ID '%s' not found in permissions.\n", userId);
    return false;
}

bool recursiveChange(int inodeID, const char* userId, int newPermission) {
    // 修改当前节点的权限
    if (!changePermission(inodeID, userId, newPermission)) {
        printf("Failed to change permission for inode %d.\n", inodeID);
        return false;
    }

    // 如果是目录，递归修改其子目录和文件的权限
    if (inodeMem[inodeID].fileType == 0) { // 0 表示目录
        struct DirectoryBlock* block = (struct DirectoryBlock*)&blockMem[inodeMem[inodeID].blockID];
        for (int i = 0; i < ENTRY_NUMBER; i++) {
            if (block->inodeID[i] != -1) {
                if (!recursiveChange(block->inodeID[i], userId, newPermission)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool changeAuthority(char *inputPath, const char* userId, int newPermission) {
    char path[256];
    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0';

    char* directory, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    // 遍历目录路径
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory and cannot be changed!\n");
        return false;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            int flag = 0;
            for (int i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                printf("The path does not exist or is not a directory!\n");
                return false;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    //追下一层
    int entry = -1;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            entry = i;
            break;
        }
    }
    if (entry < 0) {
        printf("The directory does not exist!\n");
        return false;
    }
    int inodeID = block->inodeID[entry];
    pointer=&inodeMem[inodeID];

    // 从指定目录开始递归地修改权限
    return recursiveChange(pointer->inodeNumber, userId, newPermission);
}








bool createDirectory(char *inputPath, const char* userID) {//传入userID匹配后，修改student_perm，成creater可读可写(这样可以实现老师能够在init_inode(管理员权限3，老师学生权限0)后，依然能够读写发布的作业)
    char path[256]; // you should ensure that the string passed to strtok is modifiable. Typically, this means using a character array that is not a string literal
    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0'; // Null-terminate to avoid overflow
    
    int i, j, flag;
    char* directory, * parent, * target;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    int parentInodeID = -1; // Initialize parent inode ID
    int newInodeID = -1;    // Initialize new inode ID

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory!\n");
        return false;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist!\n");
                return false;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // At this point, 'pointer' points to the parent inode
    parentInodeID = pointer->inodeNumber;

    // 创建目标目录
    int entry = -1, n_block = -1, n_inode = -1;
    target = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], target) == 0) {
            printf("The directory already exists!\n");
            return false;
        }
        if (block->inodeID[i] == -1) {//找到空的block可以用来创建目录
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        // 找到一个未使用的数据块
        for (i = 0; i < BLOCK_NUMBER / 8; i++) {
            for (j = 0; j < 8; j++) {
                if ((blockBitmap[i] & (1 << j)) == 0) {
                    n_block = i * 8 + j;
                    break;
                }
            }
            if (n_block != -1) {
                break;
            }
        }
        if (n_block == -1) {
            printf("The block is full!\n");
            return false;
        }

        // 找到一个未使用的索引节点
        flag = 0;
        for (i = 0; i < INODE_NUMBER; i++) {
            if (inodeMem[i].blockID == -1) {
                flag = 1;
                inodeMem[i].blockID = n_block;
                inodeMem[i].fileType = 0;
                n_inode = i;
                
                init_inode(&inodeMem[n_inode]);

                // 为创建者设置读写权限
                for (int j = 0; j < NUM_USERS; j++) {
                    if (strcmp(inodeMem[n_inode].permissions.teacher_perm[j].id, userID) == 0) {
                        inodeMem[n_inode].permissions.teacher_perm[j].permission = PERM_READ | PERM_WRITE;
                        break;
                    }
                    else if (strcmp(inodeMem[n_inode].permissions.student_perm[j].id, userID) == 0) {
                        inodeMem[n_inode].permissions.student_perm[j].permission = PERM_READ | PERM_WRITE;
                        break;
                    }
                }

                break;
            }
        }
        if (n_inode == -1) {
            printf("The inode is full!\n");
            return false;
        }

        // 初始化新目录文件
        block->inodeID[entry] = n_inode;
        strcpy(block->fileName[entry], target);
        blockBitmap[n_block / 8] |= 1 << (n_block % 8);
        block = (struct DirectoryBlock*)&blockMem[n_block];
        for (i = 0; i < ENTRY_NUMBER; i++) {
            block->inodeID[i] = -1;
        }
    }
    else {
        printf("The directory is full!\n");
        return false;
    }

    // After creating new directory, get its inode ID
    newInodeID = n_inode; // Assuming 'n_inode' is the inode number of the new directory
    // After creating a directory, sync permissions
    syncPermissions(parentInodeID, newInodeID); // Use appropriate inode IDs

    return true;
}

//deleteDirectory函数的两个辅助函数
void deleteFileByInode(int inodeID) {
    // 清除文件项和释放索引节点和数据块
    int blockID = inodeMem[inodeID].blockID;
    inodeMem[inodeID].blockID = -1;
    inodeMem[inodeID].fileType = -1;

    memset(&blockMem[blockID], 0, sizeof(struct FileBlock));
    blockBitmap[blockID / 8] &= ~(1 << (blockID % 8));
}

void recursiveDelete(int inodeID) {
    struct DirectoryBlock* block = (struct DirectoryBlock*)&blockMem[inodeMem[inodeID].blockID];

    // 递归删除子目录和文件
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (block->inodeID[i] != -1) {
            if (inodeMem[block->inodeID[i]].fileType == 1) {
                // 如果是文件，删除文件
                deleteFileByInode(block->inodeID[i]);
            }
            else {
                // 如果是目录，递归删除目录
                recursiveDelete(block->inodeID[i]);
            }
        }
    }

    // 清除目录项和释放索引节点和数据块
    int blockID = inodeMem[inodeID].blockID;
    inodeMem[inodeID].blockID = -1;
    inodeMem[inodeID].fileType = -1;

    memset(&blockMem[blockID], 0, sizeof(struct FileBlock));
    blockBitmap[blockID / 8] &= ~(1 << (blockID % 8));
}

bool deleteDirectory(char *inputPath) {
    char path[256]; // you should ensure that the string passed to strtok is modifiable. Typically, this means using a character array that is not a string literal
    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0'; // Null-terminate to avoid overflow

    char* directory, * parent, * target;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("It is the root directory and cannot be deleted!\n");
        return false;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            int flag = 0;
            for (int i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                printf("The path does not exist or is not a directory!\n");
                return false;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 删除目标目录及其内容
    int entry = -1;
    target = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], target) == 0) {
            entry = i;
            break;
        }
    }
    if (entry < 0) {
        printf("The directory does not exist!\n");
        return false;
    }

    int inodeID = block->inodeID[entry];

    if (inodeMem[inodeID].fileType == 1) {
        // 如果是文件，删除文件
        deleteFileByInode(inodeID);
    }
    else {
        // 如果是目录，递归删除目录
        recursiveDelete(inodeID);
    }

    // 清除目录项
    block->inodeID[entry] = -1;
    strcpy(block->fileName[entry], "");

    printf("Directory '%s' has been deleted!\n", target);
    return true;
}


char* listFiles(char *inputPath) {
    static char fileListing[256];  // Increased size for buffer
    char path[256];
    if (inputPath == NULL || strlen(inputPath) == 0) {
        strcpy(fileListing, "Invalid input path!\n");
        return fileListing;
    }
    strncpy(path, inputPath, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    int i, flag;
    char* directory;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    strcpy(fileListing, "The directory includes following files:\n");

    directory = strtok(path, delimiter);
    while (directory != NULL) {
        block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
        flag = 0;
        for (i = 0; i < ENTRY_NUMBER; i++) {
            if (strcmp(block->fileName[i], directory) == 0) {
                flag = 1;
                pointer = &inodeMem[block->inodeID[i]];
                break;
            }
        }
        if (flag == 0 || pointer->fileType == 1) {
            strcpy(fileListing, "The path does not exist or it is not a directory!\n");
            return fileListing;
        }
        directory = strtok(NULL, delimiter);
    }

    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (block->inodeID[i] != -1) {
            struct Inode *fileInode = &inodeMem[block->inodeID[i]];
            char line[128];
            int line_length = snprintf(line, sizeof(line), "%d\t%d\t%s\n", 
                                       fileInode->inodeNumber, fileInode->fileType, block->fileName[i]);
            if (line_length > 0 && strlen(fileListing) + line_length < sizeof(fileListing)) {
                strcat(fileListing, line);
            } else {
                // Handle buffer overflow or snprintf error
                strcat(fileListing, "Error: Buffer overflow or line formatting error\n");
                break;
            }
        }
    }

    return fileListing;
}



bool createFile(char *inputPath, const char* userID) {
    char path[256]; // you should ensure that the string passed to strtok is modifiable. Typically, this means using a character array that is not a string literal
    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0'; // Null-terminate to avoid overflow

    int i, flag;
    char* directory, * filename, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    int parentInodeID = -1; // 用于存储父目录的 inode 编号
    int newInodeID = -1;    // 用于存储新文件的 inode 编号

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory does not allow file creation!\n");
        return false;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist or is not a directory!\n");
                return false;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }
    // 在创建新文件之前，找到父目录的 inode 编号
    parentInodeID = pointer->inodeNumber;
    // 创建新文件
    int entry = -1, n_inode = -1, n_block = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            printf("The file already exists!\n");
            return false;
        }
        if (block->inodeID[i] == -1) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        // 找到一个未使用的索引节点
        for (i = 0; i < INODE_NUMBER; i++) {
            if (inodeMem[i].blockID == -1) {
                // 找到一个未使用的数据块
                for (int j = 0; j < BLOCK_NUMBER; j++) {
                    int byteIndex = j / 8;
                    int bitIndex = j % 8;
                    if (!(blockBitmap[byteIndex] & (1 << bitIndex))) {
                        n_block = j;
                        blockBitmap[byteIndex] |= (1 << bitIndex); // 标记数据块为已使用
                        break;
                    }
                }
                if (n_block == -1) {
                    printf("No blocks available.\n");
                    return false;
                }
                inodeMem[i].fileType = 1; // 设置文件类型为普通文件
                inodeMem[i].blockID = n_block; // 分配数据块
                n_inode = i; // 记录索引节点号
                init_inode(&inodeMem[n_inode]);

                // 为创建者设置读写权限
                for (int j = 0; j < NUM_USERS; j++) {
                    if (strcmp(inodeMem[n_inode].permissions.teacher_perm[j].id, userID) == 0) {
                        inodeMem[n_inode].permissions.teacher_perm[j].permission = PERM_READ | PERM_WRITE;
                        break;
                    }
                    else if (strcmp(inodeMem[n_inode].permissions.student_perm[j].id, userID) == 0) {
                        inodeMem[n_inode].permissions.student_perm[j].permission = PERM_READ | PERM_WRITE;
                        break;
                    }
                }

                break;
            }
        }
        if (n_inode == -1) {
            printf("The inode is full!\n");
            return false;
        }

        // 初始化新文件项
        block->inodeID[entry] = n_inode;
        strcpy(block->fileName[entry], filename);
    }
    else {
        printf("The directory is full!\n");
        return false;
    }
    
    // 确定新文件的 inode 编号
    newInodeID = n_inode;

    // 在这里，我们需要确保新文件继承父目录的权限
    if (newInodeID != -1 && parentInodeID != -1) {
        syncPermissions(parentInodeID, newInodeID);
    }
    return true;
}


void deleteFile() {
    int i;
    char path[256];
    char* directory, * filename, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    printf("Input the path of the file to be deleted:\n");
    scanf("%s", path);

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory cannot be deleted!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            int flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 删除目标文件
    int entry = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        int inodeID = block->inodeID[entry];
        int blockID = inodeMem[inodeID].blockID;

        if (inodeMem[inodeID].fileType != 1) {
            printf("The path is not a file!\n");
            return;
        }

        // 清除文件内容
        memset(&blockMem[blockID], 0, sizeof(struct FileBlock));

        // 更新位图以表示数据块现在是空闲的
        int byteIndex = blockID / 8;
        int bitOffset = blockID % 8;
        blockBitmap[byteIndex] &= ~(1 << bitOffset);

        // 清除目录项
        block->inodeID[entry] = -1;
        memset(block->fileName[entry], 0, MAX_FILENAME_LENGTH);

        // 重置Inode
        inodeMem[inodeID].blockID = -1;
        inodeMem[inodeID].fileType = -1; // -1 或其他特定值表示这个inode未被使用

        printf("File '%s' has been deleted.\n", filename);
    }
    else {
        printf("The file does not exist!\n");
    }
}


char* readFile(char *inputPath) {
    static char fileContent[1024];  // Increase size if needed
    char path[256];
    strcpy(path, inputPath);
    path[sizeof(path) - 1] = '\0';

    int i, flag;
    char* filename, * directory, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory cannot be read!\n");
        return NULL;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist or is not a directory!\n");
                return NULL;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 读取目标文件
    int entry = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];

    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            entry = i;
            break;
        }
    }

    if (entry >= 0) {
        int inodeID = block->inodeID[entry];
        if (inodeMem[inodeID].fileType == 0) {
            strcpy(fileContent, "The specified path is not a file!\n");
            return fileContent;
        }
        struct FileBlock* fileBlock = (struct FileBlock*)&blockMem[inodeMem[inodeID].blockID];
        strcpy(fileContent, fileBlock->content);
    } else {
        strcpy(fileContent, "The file does not exist!\n");
    }

    return fileContent;
}

void writeFile(char *inputPath, const char* content, bool append) {
    char path[256]; // you should ensure that the string passed to strtok is modifiable. Typically, this means using a character array that is not a string literal
    strncpy(path, inputPath, sizeof(path));
    path[sizeof(path) - 1] = '\0'; // Null-terminate to avoid overflow


    int i, flag;
    char* filename, * directory, * parent;
    const char delimiter[2] = "/";
    struct Inode* pointer = inodeMem;
    struct DirectoryBlock* block;

    // 递归访问父目录的指针
    parent = NULL;
    directory = strtok(path, delimiter);
    if (directory == NULL) {
        printf("The root directory cannot be written!\n");
        return;
    }
    while (directory != NULL) {
        if (parent != NULL) {
            block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
            flag = 0;
            for (i = 0; i < ENTRY_NUMBER; i++) {
                if (strcmp(block->fileName[i], parent) == 0) {
                    flag = 1;
                    pointer = &inodeMem[block->inodeID[i]];
                    break;
                }
            }
            if (flag == 0 || pointer->fileType == 1) {
                // 如果父目录不存在或不是目录文件
                printf("The path does not exist or is not a directory!\n");
                return;
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // 写入目标文件
    int entry = -1;
    filename = parent;
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], filename) == 0) {
            entry = i;
            break;
        }
    }
    if (entry >= 0) {
        int inodeID = block->inodeID[entry];
        if (inodeMem[inodeID].fileType == 0) {
            printf("The specified path is not a file!\n");
            return;
        }
        struct FileBlock* fileBlock = (struct FileBlock*)&blockMem[inodeMem[inodeID].blockID];
        // printf("Enter the content to be written (up to %d characters):\n", BLOCK_SIZE);
        // scanf(" %[^\n]s", fileBlock->content);

        if (append) {
            char newContent[strlen(content) + 2];
            newContent[0] = '\n';
            strcpy(&newContent[1],content);
            // Append mode
            strncat(fileBlock->content, content, BLOCK_SIZE - strlen(fileBlock->content) - 1);
        } else {
            // Overwrite mode
            strncpy(fileBlock->content, content, BLOCK_SIZE);
            fileBlock->content[BLOCK_SIZE - 1] = '\0';
        }
        printf("File content has been updated.\n");
    } else {
        printf("The file does not exist!\n");
    }
}


// 保存快照到文件
bool saveSnapshot(const char* snapshotFile) {
    FILE* file = fopen(snapshotFile, "wb");
    if (file == NULL) {
        printf("Failed to open snapshot file for writing.\n");
        return false;
    }

    // 写入inode数组
    fwrite(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);

    // 写入文件/目录块数组
    fwrite(blockMem, sizeof(struct FileBlock), BLOCK_NUMBER, file);

    // 写入数据块位图
    fwrite(blockBitmap, sizeof(blockBitmap), 1, file);

    // Restore the user array
    fwrite(users, sizeof(User), MAX_USERS, file);

    // Restore the user count
    fwrite(&userCount, sizeof(userCount), 1, file);

    fclose(file);
    printf("Snapshot saved to '%s'\n", snapshotFile);
    return true;
}

// 从快照文件恢复
bool restoreFromSnapshot(const char* snapshotFile) {
    FILE* file = fopen(snapshotFile, "rb");
    if (file == NULL) {
        printf("Failed to open snapshot file for reading.\n");
        return false;
    }

    // 读取inode数组
    fread(inodeMem, sizeof(struct Inode), INODE_NUMBER, file);

    // 读取文件/目录块数组
    fread(blockMem, sizeof(struct FileBlock), BLOCK_NUMBER, file);

    // 读取数据块位图
    fread(blockBitmap, sizeof(blockBitmap), 1, file);

    // Restore the user array
    fread(users, sizeof(User), MAX_USERS, file);

    // Restore the user count
    fread(&userCount, sizeof(userCount), 1, file);

    fclose(file);
    printf("File system restored from '%s'\n", snapshotFile);
    return true;
}


//增 辅助函数
void add_(char *dir, char *name){
    char path[256];
    // Use sprintf to concatenate "/root/Course/" with the courseName
    sprintf(path, "/root/%s/%s", dir, name);
    createDirectory(path, "admin");
}
//删 辅助函数
void delete_(char *dir, char *name){
    char path[256];
    // Use sprintf to concatenate "/root/Course/" with the courseName
    sprintf(path, "/root/%s/%s", dir, name);
    deleteDirectory(path);
}
//判断这个path是否存在
bool judgeIn(char *inputPath) {
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
            if (!found || pointer->fileType != 0) { // fileType 0 for directories
                return false; // Path component not found or not a directory
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // Check if the final component exists
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            return true; // Final component found
        }
    }

    return false; // Final component not found
}
//判断是否File
bool isFile(char *inputPath) {
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
                return false; // Path component not found
            }
        }
        parent = directory;
        directory = strtok(NULL, delimiter);
    }

    // At the end of the path, check if the final component is a file
    block = (struct DirectoryBlock*)&blockMem[pointer->blockID];
    for (int i = 0; i < ENTRY_NUMBER; i++) {
        if (strcmp(block->fileName[i], parent) == 0) {
            return inodeMem[block->inodeID[i]].fileType == 1; // Check if the fileType indicates a file
        }
    }

    return false; // Final component not found or not a file
}

void syncPermissions(int parentInodeID, int newInodeID) {
    struct Inode* parentInode = &inodeMem[parentInodeID];
    struct Inode* newInode = &inodeMem[newInodeID];

    // Copy teacher permissions
    for (int i = 0; i < NUM_USERS; i++) {
        strcpy(newInode->permissions.teacher_perm[i].id, parentInode->permissions.teacher_perm[i].id);
        newInode->permissions.teacher_perm[i].permission = parentInode->permissions.teacher_perm[i].permission;
    }

    // Copy student permissions
    for (int i = 0; i < NUM_USERS; i++) {
        strcpy(newInode->permissions.student_perm[i].id, parentInode->permissions.student_perm[i].id);
        newInode->permissions.student_perm[i].permission = parentInode->permissions.student_perm[i].permission;
    }
}