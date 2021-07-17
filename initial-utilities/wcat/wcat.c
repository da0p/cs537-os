#include <stdio.h>

#define BUF_SZ 1024

int main (int argc, char *argv[]) {

    FILE *fp = NULL;
    char s[BUF_SZ] = {0};

    if (argc < 2)
        return 0;

    for (int i = 1; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wcat: cannot open file\n");
            return 1;
        }

        while (fgets(s, BUF_SZ, fp) != NULL) {
            printf("%s", s);
        }
    }

    return 0;
}
