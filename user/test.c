#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    int count = getprocs();
    printf("目前有%d个活跃进程\n", count);
    exit(0);
}

// #include "kernel/types.h"
// #include "user/user.h"

// int main(int argc, char *argv[])
// {
//   my_heap();
//   return 0;
// }

