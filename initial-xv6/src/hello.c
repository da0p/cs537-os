#include "types.h"
#include "user.h"

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf(1, "%s name\n", argv[0]);
        exit();
    }
    
    printf(1, "hello from %s\n", argv[1]);
    exit();
}
