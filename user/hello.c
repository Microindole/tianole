// user/hello.c - Our first user program!

// We can't use kernel headers, so we define what we need.
// This function will be provided by a system call.
void print(const char* str); 

int main() {
    print("Hello from User Space!");
    
    // An infinite loop to prevent the program from returning
    // since we don't have an exit() syscall for user mode yet.
    for(;;);

    return 0; 
}

// A simple syscall wrapper for printing
void print(const char* str) {
    asm volatile (
        "int $0x80"      // Trigger syscall interrupt
        :                // No output operands
        : "a" (3),       // Syscall number 3 for 'print'
          "b" (str)      // Pass the string address in EBX
    );
}