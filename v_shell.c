#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>
#include <termios.h>
#include <sys/prctl.h>

#define VSH_MAX_INPUT 1024

// Define ANSI escape codes for colors
#define PROMPT_COLOR  "\033[36m" // Cyan for prompt
#define RESET_COLOR   "\033[0m"
#define USER_COLOR    "\033[32m" // Green
#define HOST_COLOR    "\033[34m" // Blue
#define DIR_COLOR     "\033[34m" // Blue for directories
#define FILE_COLOR    "\033[37m" // White for regular files
#define EXEC_COLOR    "\033[32m" // Green for executables
#define C_FILE_COLOR  "\033[31m" // Red for C files (.c)
#define CPP_FILE_COLOR "\033[33m" // Yellow for C++ files (.cpp)
#define PY_FILE_COLOR "\033[35m" // Magenta for Python files (.py)
#define JAVA_FILE_COLOR "\033[36m" // Cyan for Java files (.java)
#define SHELL_FILE_COLOR "\033[34m" // Blue for Shell scripts (.sh)
#define WEB_FILE_COLOR "\033[36m" // Cyan for HTML/CSS/JS files
#define RUBY_FILE_COLOR "\033[35m" // Pink for Ruby files (.rb)
#define PHP_FILE_COLOR "\033[32m" // Green for PHP files (.php)
#define GO_FILE_COLOR "\033[32m" // Light Green for Go files (.go)
#define XML_JSON_FILE_COLOR "\033[37m" // Light Cyan for XML/JSON files
#define SQL_FILE_COLOR "\033[33m" // Orange for SQL files
#define DOCKER_FILE_COLOR "\033[35m" // Magenta for Docker files
#define IMAGE_FILE_COLOR "\033[96m" // Light Blue for image files
#define MEDIA_FILE_COLOR "\033[93m" // Light Yellow for media files
#define ARCHIVE_FILE_COLOR "\033[38;5;94m" // Brown for archive files
#define BACKUP_FILE_COLOR "\033[91m" // Red for backup/temporary files
#define CONFIG_FILE_COLOR "\033[35m" // Light Purple for config files
#define MARKDOWN_FILE_COLOR "\033[37m" // White for markdown files

// Global variables for job control
struct termios shell_tmodes;
pid_t shell_pgid;

// Built-in commands structure
struct builtin {
    char *name;
    void (*func)(char **args, int arg_count);
};

// Function prototypes
void display_prompt(void);
void colorize_ls_output(void);
const char* get_file_extension(const char *filename);
void execute_command(char *command);
char **split_command(char *command, int *arg_count);
void free_args(char **args, int count);
void vsh_help(char **args, int arg_count);
void vsh_cd(char **args, int arg_count);
void vsh_exit(char **args, int arg_count);
void vsh_version(char **args, int arg_count);
void initialize_shell(void);
void initialize_job_control(void);
void set_shell_env(void);
int is_builtin(char *cmd);
void signal_handler(int signo);

// Built-in command functions
void vsh_help(char **args, int arg_count) {
    printf("VSH - V Shell, version 1.0\n");
    printf("Built-in commands:\n");
    printf("  cd [dir]     - Change directory\n");
    printf("  exit         - Exit the shell\n");
    printf("  help         - Show this help\n");
    printf("  version      - Show shell version\n");
    printf("  ls          - List files and directories\n");
    printf("and many other Bash commands...\n");
    printf("tip: don't run 'neofetch' or any other command like that\n");
}

void vsh_exit(char **args, int arg_count) {
    printf("Goodbye!\n");
    exit(0);
}

void vsh_cd(char **args, int arg_count) {
    if (arg_count < 2) {
        char *home = getenv("HOME");
        if (home != NULL) {
            if (chdir(home) != 0) {
                perror("vsh");
            }
        }
    } else {
        if (chdir(args[1]) != 0) {
            perror("vsh");
        }
    }
}

void vsh_version(char **args, int arg_count) {
    printf("VSH v1.0\n");
}

// Array of built-in commands
struct builtin builtins[] = {
    {"cd", vsh_cd},
    {"help", vsh_help},
    {"version", vsh_version},
    {"exit", vsh_exit},
    {NULL, NULL}
};

// Initialize shell environment
void set_shell_env() {
    setenv("SHELL", "/usr/local/bin/vsh", 1);
    setenv("VSH_VERSION", "1.0", 1);
    prctl(PR_SET_NAME, "vsh", 0, 0, 0);
}

void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("\n");
        display_prompt();
        fflush(stdout);
    }
}

void display_prompt() {
    char hostname[HOST_NAME_MAX];
    char cwd[PATH_MAX];
    char *username = getenv("USER");

    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        strcpy(hostname, "unknown");
    }

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        strcpy(cwd, "unknown");
    }

    printf("\n" PROMPT_COLOR "vsh." USER_COLOR "%s" RESET_COLOR "@" HOST_COLOR "%s" RESET_COLOR " in " DIR_COLOR "%s" RESET_COLOR "> ",
           username ? username : "unknown", hostname, cwd);
}

void initialize_shell() {
    printf("\033[H\033[J");

    printf(PROMPT_COLOR "Welcome to VSH - V Shell v1.0\n" RESET_COLOR);
    printf(PROMPT_COLOR "Type 'help' for commands\n\n" RESET_COLOR);

    set_shell_env();

    signal(SIGINT, signal_handler);
    signal(SIGTSTP, SIG_IGN);
}

void initialize_job_control() {
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("Couldn't put the shell in its own process group");
        exit(1);
    }

    tcsetpgrp(STDIN_FILENO, shell_pgid);

    tcgetattr(STDIN_FILENO, &shell_tmodes);
}

const char* get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

void colorize_ls_output() {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    struct stat file_stat;

    while ((entry = readdir(dir)) != NULL) {
        if (stat(entry->d_name, &file_stat) == -1) {
            perror("stat");
            continue;
        }

        const char *extension = get_file_extension(entry->d_name);

        if (entry->d_type == DT_DIR || S_ISDIR(file_stat.st_mode)) {
            // Directory: Blue
            printf(DIR_COLOR "%s" RESET_COLOR "\n", entry->d_name);
        } else if (S_ISREG(file_stat.st_mode)) {
            // Regular file
            if (strcmp(extension, "c") == 0) {
                printf(C_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "cpp") == 0) {
                printf(CPP_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "py") == 0) {
                printf(PY_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "java") == 0) {
                printf(JAVA_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "sh") == 0 || strcmp(extension, "bash") == 0 || strcmp(extension, "zsh") == 0) {
                printf(SHELL_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "html") == 0 || strcmp(extension, "css") == 0 || strcmp(extension, "js") == 0) {
                printf(WEB_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "rb") == 0) {
                printf(RUBY_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "php") == 0) {
                printf(PHP_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "go") == 0) {
                printf(GO_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "xml") == 0 || strcmp(extension, "json") == 0 || strcmp(extension, "yaml") == 0) {
                printf(XML_JSON_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "sql") == 0) {
                printf(SQL_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "dockerfile") == 0) {
                printf(DOCKER_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "png") == 0 || strcmp(extension, "jpg") == 0 || strcmp(extension, "jpeg") == 0) {
                printf(IMAGE_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "mp3") == 0 || strcmp(extension, "wav") == 0 || strcmp(extension, "mp4") == 0) {
                printf(MEDIA_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else if (strcmp(extension, "tar") == 0 || strcmp(extension, "gz") == 0 || strcmp(extension, "zip") == 0) {
                printf(ARCHIVE_FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            } else {
                printf(FILE_COLOR "%s" RESET_COLOR "\n", entry->d_name);
            }
        }
    }
    closedir(dir);
}

void execute_command(char *command) {
    int arg_count = 0;
    char **args = split_command(command, &arg_count);

    if (args[0] == NULL) {
        free_args(args, arg_count);
        return;
    }

    if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        printf("Goodbye!\n");
        free_args(args, arg_count);
        exit(0);
    }

    if (is_builtin(args[0])) {
        for (int i = 0; builtins[i].name != NULL; i++) {
            if (strcmp(builtins[i].name, args[0]) == 0) {
                builtins[i].func(args, arg_count);
                break;
            }
        }
    } else {
        pid_t pid = fork();
        if (pid == 0) {
            if (execvp(args[0], args) == -1) {
                perror("vsh");
                exit(1);
            }
        } else {
            wait(NULL);
        }
    }

    // Clean up
    free_args(args, arg_count);
}

char **split_command(char *command, int *arg_count) {
    char **args = malloc(VSH_MAX_INPUT * sizeof(char *));
    char *token = strtok(command, " \t\n");
    *arg_count = 0;

    while (token != NULL) {
        args[*arg_count] = token;
        (*arg_count)++;
        token = strtok(NULL, " \t\n");
    }
    args[*arg_count] = NULL;
    return args;
}

void free_args(char **args, int count) {
    free(args);
}

int is_builtin(char *cmd) {
    for (int i = 0; builtins[i].name != NULL; i++) {
        if (strcmp(builtins[i].name, cmd) == 0) {
            return 1;
        }
    }
    return 0;
}

int main() {
    char input[VSH_MAX_INPUT];

    // Initialize shell
    initialize_shell();
    initialize_job_control();

    while (1) {
        display_prompt();

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                exit(0);
            }
            continue;
        }

        input[strcspn(input, "\n")] = 0;

        // Execute the command
        execute_command(input);
    }

    return 0;
}
