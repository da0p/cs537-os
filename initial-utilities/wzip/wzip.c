#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {

    FILE *fp = NULL;
    int nbytes;
    int same = 1, j = 0;
    size_t n = 0;
    char prev = '\0', cur = '\0';

    if (argc < 2) {
        printf("wzip: file1 [file2 ...]\n");
        return 1;
    }
   
    for (int i = 1; i < argc; i++) { 
       
        fp = fopen(argv[i], "r");

        if (fp == NULL) {
            printf("wzip: cannot open file\n");
            return 1;
        }
        while ((cur = fgetc(fp)) != EOF) {
            if (prev != '\0') { 
                if (prev == cur) 
                    same++;
                else { 
                    fwrite(&same, sizeof(int), 1, stdout);
                    fwrite(&prev, 1, 1, stdout);
                    same = 1;
                }
            }
            prev = cur;
        }

        if (i == argc - 1) {
            fwrite(&same, sizeof(int), 1, stdout);
            fwrite(&prev, 1, 1, stdout);
        }
        fclose(fp);
    }

    return 0;
}

