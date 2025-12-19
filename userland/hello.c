#include "userlib.h"

int main() {
    print("========================================\n");
    print("Hello from userspace!\n");
    print("This is a user-mode program running in Ring 3.\n");
    print("========================================\n");

    exit(0);
    return 0;
}
