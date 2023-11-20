#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>

// Additional headers as needed


// Register process with kernel module
void register_process(unsigned int pid)
{
    
     // Insert your code here ...
    FILE* file = fopen("/proc/kmlab/status", "w");
    fprintf(file, "%u", pid);
    fclose(file);
}

int main(int argc, char* argv[])
{
    int __expire = 10;
    time_t start_time = time(NULL);

    if (argc == 2) {
        __expire = atoi(argv[1]);
    }

    register_process(getpid());

    // Terminate user application if the time is expired
    while (1) {
        if ((int)(time(NULL) - start_time) > __expire) {
            break;
        }
    }

	return 0;
}
