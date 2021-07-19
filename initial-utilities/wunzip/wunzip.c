#include <stdio.h>

#define BUF_SZ   5 * 512

int main (int argc, char *argv[]) {

    FILE *fp = NULL;
    size_t nbytes;
    char buf[BUF_SZ];
    size_t k;
    
    if (argc < 2) {
        printf("wunzip: file1 [file2 ...]\n");
        return 1;
    }

    for (int f = 1; f < argc; f++) {
        fp = fopen(argv[f], "r");
        if (fp == NULL) {
            printf("wunzip: cannot open file\n");
            return 1;
        }
        while ((nbytes = fread(buf, 1, BUF_SZ, fp)) != 0) {
            for (int i = 0; i < nbytes; i += 5) {
                k = (int)buf[i] + ((int)buf[i + 1] << 8) + 
                    ((int)buf[i + 2] << 16) + ((int)buf[i + 3] << 24);   
                for (int j = 0; j < k; j++) fputc(buf[i + 4], stdout);
            }
        }
        fclose(fp);
    }

    return 0;
}
