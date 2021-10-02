#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


void child_process(int *p) {
    int pid;
    int my_prime;
    int cur_num;
    int n;
    int p2[2] = {-1, 1};

    close(p[1]);
    // 读取传入的第一个素数
    n = read(p[0], &my_prime, 4);
    fprintf(1, "prime %d\n", my_prime);

    while (1) {
        n = read(p[0], (char*)&cur_num, 4);
        if (n == 0) {  // 父进行关闭了pipe，相当于通知子进程终止
            if (p2[1] != -1) close(p2[1]);  // 通知自己的孩子进程关闭
            close(p[0]);  // 关闭从父进程哪里拿到的pipe的读端
            break;
        } else {
            if (cur_num  % my_prime == 0) continue;
            if (p2[0] == -1) {  // 如果自己还没有孩子的话，就创建一个子进程，并将管道建立好
                pipe(p2);
                pid = fork();
                if (pid == 0) {
                    child_process(p2);
                }
                close(p2[0]);
            }
            // 向自己的孩子进程发送数据
            write(p2[1], &cur_num, 4);
        }
    }
    wait(&pid);
    exit(0);
}


int main() {

    int pid;
    int p[2] = {-1, -1};
    int i;

    for (i = 2; i <= 35; ++i) {
        if (p[0] == -1) {
            pipe(p);
            pid = fork();
            if (pid == 0) {
                child_process(p);
            }
            close(p[0]);
        }
        write(p[1], &i, 4);
    }
    close(p[1]);
    wait(&pid);
    exit(0);
}