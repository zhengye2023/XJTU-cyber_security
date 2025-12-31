// chat_client_async_decor.c - async client with advanced features
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <errno.h>
#include <time.h>

#define DEVICE "/dev/chat_simple"
#define BUFSZ 2048
#define NAME_SZ 32
#define CHAT_HISTORY_FILE "chat_history.txt"

static int fd = -1;
static char username[NAME_SZ];
static int cleaned = 0;

// 聊天记录文件（只用于持久化操作时的临时变量；不在全局常驻）
static FILE *history_file = NULL;

// 打印装饰线
void print_line(char c, int len) {
    for (int i = 0; i < len; i++) putchar(c);
    putchar('\n');
}

// 清理操作
void cleanup(int sig) {
    if (cleaned) return; // 防止重复执行
    cleaned = 1;

    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    // history_file 在本程序中不长期保持打开（我们在使用后总会 fclose）
    if (history_file) {
        fclose(history_file);
        history_file = NULL;
    }

    printf("\n");
    print_line('=', 50);
    printf("            Thank you for chatting!\n");
    printf("                Goodbye, %s!\n", username);
    print_line('=', 50);
    printf("\n");

    exit(0);
}

// 获取当前时间戳
void get_current_time(char *buffer, size_t size) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// 读取并显示聊天历史记录（使用局部 FILE*，关闭后不留悬指针）
void display_chat_history() {
    char line[BUFSZ];
    FILE *fh = fopen(CHAT_HISTORY_FILE, "r");
    if (fh == NULL) {
        printf("No chat history available.\n");
        return;
    }··

    print_line('-', 50);
    printf("Chat History:\n");
    while (fgets(line, sizeof(line), fh)) {
        printf("%s", line);
    }
    print_line('-', 50);
    fclose(fh);
    // 不改变全局 history_file（我们不在全局维持打开状态）
}

// 写入聊天记录到文件（使用局部 FILE*）
void save_chat_history(const char *message) {
    FILE *fh = fopen(CHAT_HISTORY_FILE, "a");
    if (fh) {
        fprintf(fh, "%s\n", message);
        fclose(fh);
    }
}

int main() {
    char buf[BUFSZ];
    char sendbuf[BUFSZ];
    ssize_t r;

    signal(SIGINT, cleanup); // 捕捉 Ctrl+C 信号
    signal(SIGTERM, cleanup); // 捕捉终止信号

    // 装饰欢迎界面
    print_line('=', 50);
    printf("            Welcome to Your Chat Room!\n");
    print_line('=', 50);

    // 输入用户名
    printf("Enter your name: ");
    if (!fgets(username, sizeof(username), stdin)) return 1;
    size_t l = strlen(username);
    if (l && username[l - 1] == '\n') username[l - 1] = '\0';
    if (strlen(username) == 0) strcpy(username, "Anonymous");

    // 显示聊天记录
    display_chat_history();
0.
    print_line('-', 50);
    printf("Hello, %s! \n", username);
    printf("Type '/quit'  to exit.\n", username);
    printf("Type '/users' to check number of users.\n");
    printf("Type '/name'  to change your name.\n");
    print_line('-', 50);

    fd = open(DEVICE, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Chat client is ready. Start typing your messages...\n\n");

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO; // 标准输入
    fds[0].events = POLLIN;q
    fds[1].fd = fd;           // 设备文件
    fds[1].events = POLLIN;

    while (1) {
        int ret = poll(fds, 2, -1); // 无限等待
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // 标准输入可读
        if (fds[0].revents & POLLIN) { //表示有事件并且是刻毒事件
            if (!fgets(buf, sizeof(buf), stdin)) break;

            size_t l = strlen(buf);
            if (l && buf[l-1] == '\n') buf[l-1] = '\0';
            if (strncmp(buf, "/name", 5) == 0) {
        char *newname = buf + 5;

        while (*newname == ' ') newname++;

        if (strlen(newname) == 0) {
            printf("Usage: /name <new_name>\n");
            continue;
        }

        if (strlen(newname) >= NAME_SZ) {
            printf("Name too long (max %d chars)\n", NAME_SZ - 1);
            continue;
        }

        strcpy(username, newname);
        printf("Your name has been changed to %s.\n", username);
        continue;
    }
            // === 新增 /users 指令 ===
            if (strcmp(buf, "/users") == 0) {
                if (write(fd, "/users", strlen("/users")) < 0) {
                    perror("write /users");
                    continue;
                }

                ssize_t r = read(fd, buf, sizeof(buf) - 1);
                if (r > 0) {
                    buf[r] = '\0';
                    printf("-----------------Current %s-----------------\n", buf);
                } else if (r == 0) {
                    printf("No response from device.\n");
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("No data available (non-blocking read).\n");
                    } else {
                        perror("read /users");
                    }
                }
                continue;
            }

            if (strcmp(buf, "/quit") == 0) break;

            // 拼接用户名并格式化消息
            char timestamp[20];
            get_current_time(timestamp, sizeof(timestamp));
            snprintf(sendbuf, sizeof(sendbuf), "[%s] %s: %s", timestamp, username, buf);

            // 保存聊天记录
            save_chat_history(sendbuf);

            if (write(fd, sendbuf, strlen(sendbuf)) < 0) {
                perror("write");
                break;
            }
        }

        // 设备文件可读
        if (fds[1].revents & POLLIN) {
            r = read(fd, buf, sizeof(buf) - 1);
            if (r < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                perror("read");
                break;
            } else if (r == 0) {
                printf("device closed\n");
                break;
            }
            buf[r] = '\0';
            printf("[recv] %s\n", buf);
        }
    }

    cleanup(0);
    return 0;
}
