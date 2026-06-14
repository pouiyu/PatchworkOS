#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fs.h>
#include <sys/proc.h>

#define INIT_PID 1

static void print_message(const char* msg)
{
    writes(STDOUT_FILENO, msg);
}

static void print_banner(void)
{
    print_message("\n========================================\n");
    print_message("           SYSTEM SHUTDOWN              \n");
    print_message("========================================\n\n");
}

static void kill_all_processes(void)
{
    print_message("Terminating all processes...\n");
    
    fd_t proc_fd = open("/proc");
    if (proc_fd == (fd_t)ERR) {
        print_message("  - Failed to open /proc\n");
        return;
    }
    
    dirent_t* entries = NULL;
    uint64_t count = 0;
    if (readdir(proc_fd, &entries, &count) == (size_t)ERR || entries == NULL) {
        print_message("  - Failed to read /proc\n");
        close(proc_fd);
        return;
    }
    
    int killed = 0;
    pid_t current_pid = getpid();
    
    for (uint64_t i = 0; i < count; i++) {
        const char* name = strrchr(entries[i].path, '/');
        if (name != NULL) {
            name++;
            bool is_pid = true;
            for (const char* p = name; *p; p++) {
                if (*p < '0' || *p > '9') {
                    is_pid = false;
                    break;
                }
            }
            
            if (is_pid) {
                pid_t pid = (pid_t)atoi(name);
                if (pid > INIT_PID && pid != current_pid) {
                    if (kill(pid) == 0) {
                        killed++;
                    }
                }
            }
        }
    }
    
    free(entries);
    close(proc_fd);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "  - Killed %d process(es)\n", killed);
    print_message(msg);
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    
    print_banner();
    kill_all_processes();
    
    print_message("\n========================================\n");
    print_message("Shutdown sequence completed.\n");
    print_message("Please power off the system manually.\n");
    print_message("========================================\n\n");
    
    return EXIT_SUCCESS;
}
