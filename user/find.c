#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  buf[strlen(p)] = 0;
  return buf;
}


void find(char* path, char* target) {
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512], *p;

    fd = open(path, 0);
    if (fd < 0) {
        printf("Open Dir Fail: %s\n", path);
        exit(1);
    }

    if (fstat(fd, &st) < 0) {
        printf("Get File Stat Fail: %s\n", path);
        close(fd);
        exit(1);
    }

    // 递归终点
    if (st.type == T_FILE) {
        // fprintf(1, "%s | %s\n", fmtname(path), target);
        if (strcmp(fmtname(path), target) == 0) {
            printf("%s\n", path);
        }
        close(fd);
        return;
    } else if (st.type != T_DIR) {
        close(fd);
        return;
    }

    if (strlen(buf) + 1 + DIRSIZ + 1 > sizeof buf) {
        printf("The Path is too long.\n");
        close(fd);
        exit(1);
    }

    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof de) == sizeof(de)) {
        if (de.inum == 0) continue;
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        find(buf, target);
    }
    close(fd);
    return;
}

int
main(int argc, char *argv[])
{
  if(argc != 3){
    printf("Useage: find <path> <fine_name>\n");
    exit(0);
  }
  find(argv[1], argv[2]);
  exit(0);
}
