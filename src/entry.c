#include <stdio.h>

int main(int argc_, char** argv_) {
    (void)argc_;
    (void)argv_;
#ifdef RELEASE_BUILD
    fprintf(stdout, "Build mode: RELEASE.\n");
#endif
#ifdef DEBUG_BUILD
    fprintf(stdout, "Build mode: DEBUG.\n");
#endif
#ifdef TEST_BUILD
    fprintf(stdout, "Build mode: TEST.\n");
#endif

    return 0;
}
