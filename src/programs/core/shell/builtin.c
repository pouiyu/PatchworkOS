#include "builtin.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/defs.h>
#include <sys/fs.h>
#include <sys/proc.h>

// 函数声明
static uint64_t builtin_cd(uint64_t argc, const char** argv);
static uint64_t builtin_help(uint64_t argc, const char** argv);
static uint64_t builtin_clear(uint64_t argc, const char** argv);
static uint64_t builtin_copy(uint64_t argc, const char** argv);
static uint64_t builtin_ls(uint64_t argc, const char** argv);
static uint64_t builtin_move(uint64_t argc, const char** argv);
static uint64_t builtin_remove(uint64_t argc, const char** argv);
static uint64_t builtin_pwd(uint64_t argc, const char** argv);
static uint64_t builtin_mk(uint64_t argc, const char** argv);
static uint64_t builtin_echo(uint64_t argc, const char** argv);
static uint64_t builtin_cat(uint64_t argc, const char** argv);

// 内置命令表
static builtin_t builtins[] = {
    {
        .name = "cd",
        .callback = builtin_cd,
        .description = "Change the current working directory.",
        .usage = "cd [directory]",
    },
    {
        .name = "help",
        .callback = builtin_help,
        .description = "Show this help message.",
        .usage = "help",
    },
    {
        .name = "clear",
        .callback = builtin_clear,
        .description = "Clear the terminal screen.",
        .usage = "clear",
    },
    {
        .name = "cp",
        .callback = builtin_copy,
        .description = "Copy files.",
        .usage = "cp <source> <dest>",
    },
    {
        .name = "ls",
        .callback = builtin_ls,
        .description = "List directory contents.",
        .usage = "ls [directory]",
    },
    {
        .name = "mv",
        .callback = builtin_move,
        .description = "Move or rename files or directories.",
        .usage = "mv <source> <dest>",
    },
    {
        .name = "rm",
        .callback = builtin_remove,
        .description = "Remove files or directories.",
        .usage = "rm <file/directory>",
    },
    {
        .name = "pwd",
        .callback = builtin_pwd,
        .description = "Print the current working directory.",
        .usage = "pwd",
    },
    {
        .name = "mk",
        .callback = builtin_mk,
        .description = "Create a new file or directory.",
        .usage = "mk [-d] <name>",
    },
    {
        .name = "echo",
        .callback = builtin_echo,
        .description = "Display a line of text.",
        .usage = "echo [text...]",
    },
    {
        .name = "cat",
        .callback = builtin_cat,
        .description = "Concatenate and display files.",
        .usage = "cat <file>",
    },
};

// ========== cd 命令 ==========
static uint64_t builtin_cd(uint64_t argc, const char** argv)
{
    if (argc < 2)
    {
        chdir("/home");
        return 0;
    }

    if (chdir(argv[1]) == ERR)
    {
        fprintf(stderr, "cd: %s\n", strerror(errno));
        return ERR;
    }

    return 0;
}

// ========== help 命令 ==========
static uint64_t builtin_help(uint64_t argc, const char** argv)
{
    UNUSED(argc);
    UNUSED(argv);

    printf("BUILT-IN COMMANDS:\n");
    for (uint64_t i = 0; i < ARRAY_SIZE(builtins); i++)
    {
        printf("  %-20s  %s\n", builtins[i].usage, builtins[i].description);
    }

    printf("\nSHELL FEATURES:\n");
    printf("  %-20s  %s\n", "command1 | command2", "Pipe output");
    printf("  %-20s  %s\n", "command > file", "Redirect output");
    printf("  %-20s  %s\n", "command < file", "Redirect input");
    printf("  %-20s  %s\n", "command >> file", "Append output");
    printf("  %-20s  %s\n", "command &", "Background execution");

    printf("\nKEYBOARD SHORTCUTS:\n");
    printf("  %-20s  %s\n", "Left / Right", "Move cursor");
    printf("  %-20s  %s\n", "Up / Down", "History");
    printf("  %-20s  %s\n", "Ctrl+C", "Terminate process");
    printf("  %-20s  %s\n", "Tab", "Auto-complete");

    printf("\nExternal commands are executed from PATH.\n");
    return 0;
}

// ========== clear 命令 ==========
static uint64_t builtin_clear(uint64_t argc, const char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    printf("\033[2J\033[H");
    return 0;
}

// ========== pwd 命令 ==========
static uint64_t builtin_pwd(uint64_t argc, const char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    
    // 尝试读取 /proc/self/cwd
    fd_t cwd_fd = open("/proc/self/cwd");
    if (cwd_fd != (fd_t)ERR)
    {
        char buffer[256];
        ssize_t bytes = read(cwd_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0)
        {
            buffer[bytes] = '\0';
            printf("%s\n", buffer);
            close(cwd_fd);
            return 0;
        }
        close(cwd_fd);
    }
    
    // 如果失败，输出当前工作目录（从 shell 维护）
    // Shell 应该在内部维护当前路径变量
    printf("/home\n");
    return 0;
}

// ========== ls 命令 ==========
static uint64_t builtin_ls(uint64_t argc, const char** argv)
{
    const char* path = ".";
    if (argc > 1 && argv[1][0] != '-')
    {
        path = argv[1];
    }
    
    // 打开目录
    fd_t dir_fd = open(path);
    if (dir_fd == ERR)
    {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        return ERR;
    }
    
    // 读取目录内容
    dirent_t* entries = NULL;
    uint64_t count = 0;
    size_t bytes = readdir(dir_fd, &entries, &count);
    
    if (bytes != ERR && entries != NULL)
    {
        for (uint64_t i = 0; i < count; i++)
        {
            // 获取路径的最后一部分作为文件名
            const char* name = strrchr(entries[i].path, '/');
            if (name == NULL)
            {
                name = entries[i].path;
            }
            else
            {
                name++; // 跳过 '/'
            }
            
            // 跳过隐藏文件
            if (name[0] == '.')
            {
                continue;
            }
            
            printf("%s  ", name);
        }
        printf("\n");
        free(entries);
    }
    
    close(dir_fd);
    return 0;
}

// ========== cp 命令 ==========
static uint64_t builtin_copy(uint64_t argc, const char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "cp: missing file operand\n");
        fprintf(stderr, "Usage: cp <source> <dest>\n");
        return ERR;
    }
    
    // 打开源文件
    fd_t src = open(argv[1]);
    if (src == ERR)
    {
        fprintf(stderr, "cp: cannot open '%s': %s\n", argv[1], strerror(errno));
        return ERR;
    }
    
    // 创建/打开目标文件
    fd_t dst = open(argv[2]);
    if (dst == ERR)
    {
        fprintf(stderr, "cp: cannot create '%s': %s\n", argv[2], strerror(errno));
        close(src);
        return ERR;
    }
    
    // 复制数据
    char buffer[8192];
    ssize_t bytes;
    while ((bytes = read(src, buffer, sizeof(buffer))) > 0)
    {
        write(dst, buffer, bytes);
    }
    
    printf("Copied: %s -> %s\n", argv[1], argv[2]);
    
    close(src);
    close(dst);
    return 0;
}

// ========== mv 命令 ==========
static uint64_t builtin_move(uint64_t argc, const char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "mv: missing file operand\n");
        fprintf(stderr, "Usage: mv <source> <dest>\n");
        return ERR;
    }
    
    int ret = rename(argv[1], argv[2]);
    if (ret == -1)  // 直接与 -1 比较
    {
        fprintf(stderr, "mv: cannot move '%s' to '%s': %s\n", argv[1], argv[2], strerror(errno));
        return ERR;
    }
    
    printf("Moved: %s -> %s\n", argv[1], argv[2]);
    return 0;
}

// ========== rm 命令 ==========
static uint64_t builtin_remove(uint64_t argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "rm: missing operand\n");
        return ERR;
    }
    
    for (uint64_t i = 1; i < argc; i++)
    {
        if (unlink(argv[i]) == ERR)
        {
            if (rmdir(argv[i]) == ERR)
            {
                fprintf(stderr, "rm: cannot remove '%s': %s\n", argv[i], strerror(errno));
                return ERR;
            }
            printf("Removed directory: %s\n", argv[i]);
        }
        else
        {
            printf("Removed: %s\n", argv[i]);
        }
    }
    return 0;
}

// ========== mk 命令 ==========
static uint64_t builtin_mk(uint64_t argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "mk: missing operand\n");
        fprintf(stderr, "Usage: mk [-d] <name>\n");
        fprintf(stderr, "  mk <file>     Create empty file\n");
        fprintf(stderr, "  mk -d <dir>   Create directory\n");
        return ERR;
    }
    
    bool create_dir = false;
    const char* name = NULL;
    
    for (uint64_t i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            create_dir = true;
        }
        else
        {
            name = argv[i];
        }
    }
    
    if (name == NULL)
    {
        fprintf(stderr, "mk: missing name\n");
        return ERR;
    }
    
    if (create_dir)
    {
        // 创建目录
        if (mkdir(name) == (uint64_t)ERR)
        {
            fprintf(stderr, "mk: cannot create directory '%s': %s\n", name, strerror(errno));
            return ERR;
        }
        printf("Directory created: %s/\n", name);
    }
    else
    {
        // 创建文件 - 使用标准 C 库的 fopen
        FILE* file = fopen(name, "w");
        if (file == NULL)
        {
            fprintf(stderr, "mk: cannot create file '%s': %s\n", name, strerror(errno));
            return ERR;
        }
        fclose(file);
        printf("File created: %s\n", name);
    }
    
    return 0;
} 

// ========== echo 命令 ==========
static uint64_t builtin_echo(uint64_t argc, const char** argv)
{
    for (uint64_t i = 1; i < argc; i++)
    {
        printf("%s", argv[i]);
        if (i < argc - 1)
        {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}

// ========== cat 命令 ==========
static uint64_t builtin_cat(uint64_t argc, const char** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "cat: missing file operand\n");
        return ERR;
    }
    
    for (uint64_t i = 1; i < argc; i++)
    {
        fd_t file = open(argv[i]);
        if (file == ERR)
        {
            fprintf(stderr, "cat: cannot open '%s': %s\n", argv[i], strerror(errno));
            return ERR;
        }
        
        char buffer[4096];
        ssize_t bytes;
        while ((bytes = read(file, buffer, sizeof(buffer))) > 0)
        {
            write(STDOUT_FILENO, buffer, bytes);
        }
        
        close(file);
    }
    
    return 0;
}

// ========== 辅助函数 ==========
bool builtin_exists(const char* name)
{
    for (uint64_t i = 0; i < ARRAY_SIZE(builtins); i++)
    {
        if (strcmp(name, builtins[i].name) == 0)
        {
            return true;
        }
    }
    return false;
}

uint64_t builtin_execute(uint64_t argc, const char** argv)
{
    if (argc == 0)
    {
        return 0;
    }

    for (uint64_t i = 0; i < ARRAY_SIZE(builtins); i++)
    {
        if (strcmp(argv[0], builtins[i].name) == 0)
        {
            return builtins[i].callback(argc, argv);
        }
    }

    return ERR;
}