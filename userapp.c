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

// print out proc file
int print_procfs(){
    printf("\nUser program prints procfs: \n");
    FILE* file = fopen("/proc/kmlab/status", "r");
    char c;
    if (file == NULL) 
    { 
        printf("Cannot open file \n"); 
        return 1;
    } 
  
    // Read contents from file 
    c = fgetc(file); 
    while (c != EOF) 
    { 
        printf ("%c", c); 
        c = fgetc(file); 
    } 
    fclose(file);
    return 0;
}

// a slow fibbonachi function
int slow_fibbonacci(int n) {
   if(n == 0){
      return 0;
   } else if(n == 1) {
      return 1;
   } else {
      return (slow_fibbonacci(n-1) + slow_fibbonacci(n-2));
   }
}

int main(int argc, char* argv[])
{
    int __expire = 10;
    time_t start_time = time(NULL);

    if (argc == 2) {
        __expire = atoi(argv[1]);
    }

    register_process(getpid());
    // print_procfs();
    
    // Terminate user application if the time is expired
    while (1) {
        if ((int)(time(NULL) - start_time) > __expire) {
            print_procfs();
            printf("Process %d is finished", (int)getpid());
            break;
        }
        slow_fibbonacci(5);
    }

	return 0;
}
