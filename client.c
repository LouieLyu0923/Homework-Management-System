#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#define IP "114.132.158.94"//10.0.8.17
#define PORT 7000

void print_err(char* str, int line, int err_no) {
    printf("%d, %s :%s\n", line, str, strerror(err_no));
    _exit(-1);
}

// 显示菜单
void displayMenu() {
    printf("\n=== Welcome to happy studying! ===\n");
    printf("1. login\n");
    printf("2. exit\n");
    printf("Please choose the number: ");
}

// 管理员菜单
void displayAdminMenu() {
    printf("\n=== Welcome to admin system! ===\n");
    printf("1. Create new user\n");
    printf("2. List all files under directory\n");
    printf("3. Add teaching courses\n");
    printf("4. Delete teaching courses\n");
    printf("5. Grant Course Access To Student\n");
    printf("6. Revoke Course Access From Student\n");
    printf("7. Grant Teaching Permissions\n");
    printf("8. Revoke Teaching Permissions\n");
    printf("9. Read from files\n");
    printf("10. Write to files\n");
    printf("11. Backup system\n");
    printf("12. Restore system\n");
    printf("13. Exit\n");
    printf("Please choose the number: ");
}
// 教师菜单
void displayTeacherMenu() {
    printf("\n=== Welcome to teacher system! ===\n");
    printf("1. Assign homework\n"); // 课程文件夹下创建子目录“chapter 1”
    printf("2. Check student_homework & Readme\n");
    printf("3. Mark student_homework & Readme\n"); //对应文件夹下，在子目录下创建文件“Readme”；写文件。 直接写文件（学生提交的文件上），your score is xxx
    printf("4. Exit\n");
    printf("Please choose the number: ");
}
// 学生菜单
void displayStudentMenu() {
    printf("\n=== Welcome to student system! ===\n");
    printf("1. view assignment Readme\n"); // 输入 /course/chapter1（服务器记录地址）后加/assignment_instruction ;读Readme文件
    printf("2. submit assignment\n"); // 先输入/course/chapter1（服务器记录地址）;创建文件student1_homework;写文件
    printf("3. view assignment and its score\n"); // 先输入/course（服务器记录地址）; 读文件/chapter1/xxx; 2021xxxxxxxx.txt
    printf("4. exit\n");
    printf("Please choose the number: ");
}

int main() {
    int skfd = -1, ret = -1;
    skfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == skfd) {
        print_err("socket failed", __LINE__, errno);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;            // 设置tcp协议族
    addr.sin_port = htons(PORT);          // 设置端口号
    addr.sin_addr.s_addr = inet_addr(IP); // 设置ip地址

    // 主动发送连接请求
    ret = connect(skfd, (struct sockaddr*)&addr, sizeof(addr));
    if (-1 == ret) print_err("connect failed", __LINE__, errno);

    char buf[256] = { 0 };
    char id[256] = { 0 };
    char password[256] = { 0 };
    int choice;
    int choice1;
    int choice2;
    int choice3;

    while (1) {
        // 显示菜单
        displayMenu();

        // 获取用户选择
        scanf("%d", &choice);

        // 清空输入缓冲区
        while (getchar() != '\n');

        // 处理用户选择
        switch (choice) {
        case 1:
            // 用户选择登录
            ret = send(skfd, "login", sizeof("login"), 0);
            if (-1 == ret) {
                print_err("send failed", __LINE__, errno);
            }
            // 输入用户名
            printf("Please enter your id: ");
            scanf("%s", id);
            ret = send(skfd, id, sizeof(id), 0);
            if (-1 == ret) {
                print_err("send failed", __LINE__, errno);
            }
            // 输入密码
            printf("Please enter your password: ");
            scanf("%s", password);

            ret = send(skfd, password, sizeof(password), 0);
            if (-1 == ret) {
                print_err("send failed", __LINE__, errno);
            }
           
            char response1[256] = { 0 };
            ret = recv(skfd, response1, sizeof(response1), 0);
            //sleep(3);
            if (-1 == ret) {
                print_err("recv failed", __LINE__, errno);
            }

            if (strcmp(response1,"admin")==0 || strcmp(response1,"teacher")==0 || strcmp(response1,"student")==0) {
                printf("Received from server: %s successfully login\n",response1);
                // 管理员端
                if (strcmp(response1,"admin")==0) {
                    int flag = 1;
                    char response2[256] = { 0 };
                    while (flag) {
                        displayAdminMenu();
                        // 获取用户选择
                        scanf("%d", &choice1);
                        // 清空输入缓冲区
                        while (getchar() != '\n');
                        switch (choice1) {
                        case 1:
                            // 用户选择创建用户
                            ret = send(skfd, "create_user", sizeof("create_user"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            // 输入新用户名
                            printf("Please enter your new id: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            // 输入新密码
                            printf("Please enter your new password: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            // 输入新用户角色
                            printf("Please enter your new role: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;

                        case 2:
                            ret = send(skfd, "list_by_dir", sizeof("list_by_dir"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the directory with '/': ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 3:
                            // 用户选择创建目录
                            ret = send(skfd, "add_course", sizeof("add_course"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the class name: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 4:
                            ret = send(skfd, "delete_course", sizeof("delete_course"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the class name: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 5:
                            ret = send(skfd, "grant", sizeof("grant"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course name for which you want to grant access: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Now, enter the student's ID or name to whom you want to grant access to this course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Finally, enter the type of permission you want to grant:\n1. Read\n2. Write\n3. Read & Write\nEnter the permission type (1, 2, or 3):");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            
                            ret = recv(skfd, response2, sizeof(response2), 0);
                        
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 6:
                            ret = send(skfd, "revoke", sizeof("revoke"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course name for which you want to revoke access: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Now, enter the student's ID or name to whom you want to revoke access to this course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 7:
                            ret = send(skfd, "grant", sizeof("grant"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course name for which you want to grant access: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Now, enter the teacher's ID or name to whom you want to grant access to this course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Finally, enter the type of permission you want to grant:\n1. Read\n2. Write\n3. Read & Write\nEnter the permission type (1, 2, or 3):");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 8:
                            ret = send(skfd, "revoke", sizeof("revoke"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course name for which you want to revoke access: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Now, enter the teacher's ID or name to whom you want to revoke access to this course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 9:
                            ret = send(skfd, "Read_File_by_dir", sizeof("Read_File_by_dir"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the directory with '/': ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 10:
                            ret = send(skfd, "Write_File_by_dir", sizeof("Write_File_by_dir"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the directory with '/': ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("You want to append or overwrite file (append/overwrite): ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please enter the content you want to mark: ");
                            scanf(" %[^\n]", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 11:
                            ret = send(skfd, "backup", sizeof("backup"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 12:
                            ret = send(skfd, "restore", sizeof("restore"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 13:
                            // 用户选择退出登录
                            flag = 0;
                            break;
                        default:
                            // 处理无效选择
                            printf("please choose again \n");
                            break;
                        }
                    }
                }
                // 教师端
                if (strcmp(response1, "teacher") == 0) {
                    char response2[256] = { 0 };
                    int flag = 1;
                    while (flag) {
                        displayTeacherMenu();
                        // 获取用户选择
                        scanf("%d", &choice2);
                        // 清空输入缓冲区
                        while (getchar() != '\n');
                        switch (choice2) {
                        case 1:
                            ret = send(skfd, "Assign", sizeof("Assign"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            // 输入用户名
                            printf("Please enter your id: ");
                            scanf("%s", id);

                            ret = send(skfd, id, sizeof(id), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please enter your course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please name the homework: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please write Readme file: ");
                            scanf(" %[^\n]", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);

                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 2:
                            ret = send(skfd, "teacherCheck", sizeof("teacherCheck"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            // 输入用户名
                            printf("Please enter your id: ");
                            scanf("%s", id);
                            ret = send(skfd, id, sizeof(id), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please enter your course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);

                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            
                            // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Sorry", 5) != 0){
				ret = send(skfd, "accepted", sizeof("accepted"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                            }
			    else{
				ret = send(skfd, "failed", sizeof("failed"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                                break;
			    }

                            printf("Please enter which homework to see: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }

                            printf("Please enter the name of the file or directory you want to access: ");
                            scanf("%s", buf);//对应submitName
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                             // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Readme", 6) == 0){
                                break;
                            }
			                else{
                                printf("Please enter the name of the file you want to access: ");
                                scanf("%s", buf);//对应fileNameInIdDir
                                ret = send(skfd, buf, sizeof(buf), 0);
                                if (-1 == ret) {
                                    print_err("send failed", __LINE__, errno);
                                }

                                ret = recv(skfd, response2, sizeof(response2), 0);
                                if (-1 == ret) {
                                    print_err("recv failed", __LINE__, errno);
                                }
                                else if (ret > 0) {
                                    printf("Received from server:%s\n", response2);
                                }
                            }
                            break;
                        case 3:
                            ret = send(skfd, "teacherMark", sizeof("teacherMark"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            // 输入用户名
                            printf("Please enter your id: ");
                            scanf("%s", id);
                            ret = send(skfd, id, sizeof(id), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please enter your course: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);

                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }

                            // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Sorry", 5) != 0){
				ret = send(skfd, "accepted", sizeof("accepted"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                            }
			    else{
				ret = send(skfd, "failed", sizeof("failed"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                                break;
			    }

                            printf("Please enter the homework you want to check: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);

                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }

                            printf("Please enter the Readme or which student directory you want to mark: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);//fileName
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            if(strncmp(buf, "Readme", 6) == 0){
                            printf("You want to append or overwrite file (append/overwrite): ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please enter the content you want to mark: ");
                            scanf(" %[^\n]", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            ret = recv(skfd, response2, sizeof(response2), 0);
                                if (-1 == ret) {
                                    print_err("recv failed", __LINE__, errno);
                                }
                                else if (ret > 0) {
                                    printf("Received from server:%s\n", response2);
                                }
                                break;
                            }else{
                                ret = recv(skfd, response2, sizeof(response2), 0);
                                if (-1 == ret) {
                                    print_err("recv failed", __LINE__, errno);
                                }
                                else if (ret > 0) {
                                    printf("Received from server:%s\n", response2);
                                }

                                printf("Please enter the name of the file you want to mark: ");
                                scanf("%s", buf);//对应fileNameInIdDir
                                ret = send(skfd, buf, sizeof(buf), 0);
                                if (-1 == ret) {
                                    print_err("send failed", __LINE__, errno);
                                }
                                printf("You want to append or overwrite file (append/overwrite): ");
                                scanf("%s", buf);
                                ret = send(skfd, buf, sizeof(buf), 0);
                                if (-1 == ret) {
                                    print_err("send failed", __LINE__, errno);
                                }
                                printf("Please enter the content you want to mark: ");
                                scanf(" %[^\n]",buf);
                                ret = send(skfd, buf, sizeof(buf), 0);
                                if (-1 == ret) {
                                    print_err("send failed", __LINE__, errno);
                                }
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            }
                            break;
                        case 4:
                            // 用户选择退出登录
                            flag = 0;
                            break;
                        default:
                            // 处理无效选择
                            printf("please choose again \n");
                            break;
                        }
                    }
                }
                // 学生端
                if (strcmp(response1, "student") == 0) {
                    char response2[256] = { 0 };
                    int flag = 1;
                    while (flag) {
                        displayStudentMenu();
                        // 获取用户选择
                        scanf("%d", &choice3);
                        // 清空输入缓冲区
                        while (getchar() != '\n');
                        switch (choice3) {
                        case 1:
                            ret = send(skfd, "ViewReadme", sizeof("ViewReadme"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            // 输入用户名
                            printf("Please enter your id: ");
                            scanf("%s", id);
                            ret = send(skfd, id, sizeof(id), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course you want to see: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            
                            // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Sorry", 5) != 0){
				ret = send(skfd, "accepted", sizeof("accepted"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                            }
			    else{
				ret = send(skfd, "failed", sizeof("failed"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                                break;
			    }
                            printf("Please enter which homework's Readme to see: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                        
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 2:
                            ret = send(skfd, "studentSubmit", sizeof("studentSubmit"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            // 输入用户名
                            printf("Please enter your id: ");
                            scanf("%s", id);                            
                            ret = send(skfd, id, sizeof(id), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course you want to submit: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            
                            // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Sorry", 5) != 0){
				ret = send(skfd, "accepted", sizeof("accepted"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                            }
			    else{
				ret = send(skfd, "failed", sizeof("failed"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                                break;
			    }

                            printf("Please enter which homework you want to submit: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

			    ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            
                            // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Sorry", 5) != 0){
				ret = send(skfd, "accepted", sizeof("accepted"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                            }
			    else{
				ret = send(skfd, "failed", sizeof("failed"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	}
                                break;
			    }

                            printf("Notification: Automatically create your own directory according to your id\n\n");

                            printf("Please enter your homework name: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            printf("Please enter your homework content: ");
                            scanf(" %[^\n]", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                        
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            break;
                        case 3:
                            ret = send(skfd, "ViewHomework", sizeof("ViewHomework"), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            // 输入用户名
                            printf("Please enter your id: ");
                            scanf("%s", id);
                            ret = send(skfd, id, sizeof(id), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                            printf("Please enter the course you want to see: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                            
                            // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Sorry", 5) != 0){
				                ret = send(skfd, "accepted", sizeof("accepted"), 0);
                                if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	    }
                            }
			                else{
				                ret = send(skfd, "failed", sizeof("failed"), 0);
                            	if (-1 == ret) {
                            	    print_err("send failed", __LINE__, errno);
                          	    }
                                break;
			                }


                            printf("Please enter which homework to see: ");
                            scanf("%s", buf);
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }
                        
                            ret = recv(skfd, response2, sizeof(response2), 0);
                            //sleep(3);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }

                            printf("Please enter the name of the file or directory you want to access: ");
                            scanf("%s", buf);//对应submitName
                            ret = send(skfd, buf, sizeof(buf), 0);
                            if (-1 == ret) {
                                print_err("send failed", __LINE__, errno);
                            }

                            ret = recv(skfd, response2, sizeof(response2), 0);
                            if (-1 == ret) {
                                print_err("recv failed", __LINE__, errno);
                            }
                            else if (ret > 0) {
                                printf("Received from server:%s\n", response2);
                            }
                             // Check if the first five characters of response2 are "Sorry"
                            if(strncmp(response2, "Readme", 6) == 0){
                                break;
                            }
			                else{
                                printf("Please enter the name of the file you want to access: ");
                                scanf("%s", buf);//对应fileNameInIdDir
                                ret = send(skfd, buf, sizeof(buf), 0);
                                if (-1 == ret) {
                                    print_err("send failed", __LINE__, errno);
                                }

                                ret = recv(skfd, response2, sizeof(response2), 0);
                                if (-1 == ret) {
                                    print_err("recv failed", __LINE__, errno);
                                }
                                else if (ret > 0) {
                                    printf("Received from server:%s\n", response2);
                                }
			                }

                            break;
                        case 4:
                            // 用户选择退出登录
                            flag = 0;
                            break;
                        default:
                            // 处理无效选择
                            printf("please choose again \n");
                            break;
                        }
                    }
                }
            }
            break;

        case 2:
            // 用户选择退出
            printf("exit...\n");
            close(skfd);
            exit(0);

        default:
            // 处理无效选择
            printf("please choose again \n");
            break;
        }
        //sleep(3);
    }

    return 0;
}


