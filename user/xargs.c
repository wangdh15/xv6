#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fs.h"

#define BUF_SIZE 512

int main(int argc, char* argv[]) {

    char* args[MAXARG];
    char* buf = (char*) malloc(BUF_SIZE + 1);;
    int str_len;
    int i, j, i_bak;
    int pid;
    for (i = 0; i < argc - 1; ++i) args[i] = argv[i+1];
    i_bak = i;
    while (1) {
        buf = gets(buf, BUF_SIZE + 1);
        str_len = strlen(buf);
        if (str_len == 0) break;
        else buf[str_len-1] = 0;
        str_len--;
        for (j = 0; j < str_len; ++j) {
            if (buf[j] == ' ') buf[j] = 0;
        }
        i = i_bak;
        for (j = 0; j < str_len; ++j) {
            if (buf[j] != 0 && (j == 0 || buf[j-1] == 0)) {
                args[i++] = buf + j;
            }
        }
        args[i] = 0;
        pid = fork();
        if (pid == 0) {
            exec(args[0], args);
        } else {
            wait(0);
        }
    }
    free(buf);
    exit(0);
}