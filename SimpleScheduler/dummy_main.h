#include "common.h"

int dummy_main(int argc, char **argv);

int main(int argc, char **argv) {
    //We are using "signal(SIGINT, SIG_IGN)" in scheduler.c instead of the below lines as that is more effecient thatn this and also doesn't require including additional files into the test cases
    // sigset_t mask1, mask2;
    // sigemptyset(&mask1);
    // sigaddset(&mask1, SIGINT);
    // if(sigprocmask(SIG_BLOCK, &mask1, &mask2) < 0){
    //     printf("Sigprocmask error!\n");
    //     exit(1);
    // }
    int ret = dummy_main(argc, argv);
    return ret;
}
#define main dummy_main