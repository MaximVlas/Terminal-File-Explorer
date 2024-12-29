/*
    Simple terminal file explorer. 
*/

#include <stdio.h>      // For standard input/output functions
#include <stdlib.h>     // For memory allocation, system commands, etc.
#include <string.h>     // For string manipulation functions
#include <dirent.h>     // For working with directories
#include <sys/stat.h>   // For file and directory metadata
#include <unistd.h>     // For directory navigation and environment handling
#include <errno.h>      // For error reporting
#include <limits.h>     // For defining system-specific limits like PATH_MAX

#ifdef _WIN32
#include <windows.h>    // For Windows-specific APIs
#define clear_screen() system("cls") // Clear screen on Windows
#define strcasecmp _stricmp           // Case-insensitive string comparison
#define S_ISLNK(m) (0)                // Symbolic links not supported on Windows
#else
#define clear_screen() system("clear") // Clear screen on Unix-like systems
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096  // Default maximum path length if not defined
#endif

#define MAX_INPUT 256  // Maximum length for user input

/* Display the header of the terminal explorer */
void display_header() {
    printf("+------------------------------------------------------------+\n");
    printf("|                Terminal File Explorer                      |\n");
    printf("+------------------------------------------------------------+\n");
}

/* Display the footer with available commands */
void display_footer() {
    printf("+------------------------------------------------------------+\n");
    printf("| [Q]uit | [U]p | [Enter] Open | [cd <path>] Change Dir       |\n");
    printf("| [lc] List Contents                                         |\n");
    printf("+------------------------------------------------------------+\n");
}

/* List contents of a given directory */
void list_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];
    struct stat file_stat;

    // Open the directory
    dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    printf("Current directory: %s\n\n", path);
    printf("%-30s %-12s %-10s\n", "Name", "Type", "Size");
    printf("-------------------------------------------------------------\n");

    // Iterate through directory contents
    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (stat(full_path, &file_stat) == -1) {
            perror("Error getting file info");
            continue;
        }

        printf("%-30s ", entry->d_name);
        if (S_ISREG(file_stat.st_mode))
            printf("%-12s %ld bytes\n", "File", (long)file_stat.st_size);
        else if (S_ISDIR(file_stat.st_mode))
            printf("%-12s <DIR>\n", "Directory");
        else
            printf("%-12s <SPECIAL>\n", "Special");
    }

    closedir(dir);
}

/* Main logic */
int main() {
    char current_path[PATH_MAX]; 
    char input[MAX_INPUT]; 

    // Get the current working directory

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("Error getting current directory");
        return 1;
    }

    while (1) {
        clear_screen();
        display_header();
        list_directory(current_path);
        display_footer();

        printf("Enter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) {
                break;
            } else {
                perror("Error reading input");
                continue;
            }
        }

        // Remove trailing newline from input

        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') {
            input[input_len - 1] = '\0';
        }

        // Quit command

        if (strcasecmp(input, "q") == 0) {
            break;
        }

        // Move up one directory

        else if (strcasecmp(input, "u") == 0) {
            char *last_slash = strrchr(current_path, '/');
            if (last_slash != NULL && last_slash != current_path) {
                *last_slash = '\0';
            }
        }

        // List contents (refresh screen)

        else if (strcasecmp(input, "lc") == 0) {
            continue;
        }

        // Change directory

        else if (strncmp(input, "cd ", 3) == 0) {
            char *new_path = input + 3;
            if (chdir(new_path) == 0) {
                if (getcwd(current_path, sizeof(current_path)) == NULL) {
                    perror("Error updating current directory");
                    strncpy(current_path, "/", sizeof(current_path));
                }
            } else {
                fprintf(stderr, "Cannot change directory: %s\n", strerror(errno));
                printf("Press Enter to continue...");
                while (getchar() != '\n');
            }
        }

        // Open directory
        
        else if (strlen(input) > 0) {
            char new_path[PATH_MAX];
            snprintf(new_path, sizeof(new_path), "%s/%s", current_path, input);
            if (chdir(new_path) == 0) {
                if (getcwd(current_path, sizeof(current_path)) == NULL) {
                    perror("Error updating current directory");
                    strncpy(current_path, "/", sizeof(current_path));
                }
            } else {
                fprintf(stderr, "Cannot open directory: %s\n", strerror(errno));
                printf("Press Enter to continue...");
                while (getchar() != '\n');
            }
        }
    }

    return 0;
}
