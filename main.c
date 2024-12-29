/*
    Enhanced Terminal File Explorer
*/

#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <dirent.h>     
#include <sys/stat.h>   
#include <unistd.h>     
#include <errno.h>      
#include <limits.h>     

#ifdef _WIN32
#include <windows.h>
#define clear_screen() system("cls")
#define strcasecmp _stricmp
#define S_ISLNK(m) (0)
#else
#define clear_screen() system("clear")
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_INPUT 256  

/* Display the header */
void display_header(const char *path) {
    printf("+--------------------------------------------------------------------+\n");
    printf("|                Terminal File Explorer                              |\n");
    printf("| Path: %-60s |\n", path);
    printf("+--------------------------------------------------------------------+\n");
}


/* Display the footer */
void display_footer() {
    printf("+--------------------------------------------------------------------+\n");
    printf("| [Q]uit | [U]p | [Enter] Open/Execute | [cd <path>] Change Dir      |\n");
    printf("+--------------------------------------------------------------------+\n");
}

/* List directory contents */
void list_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];
    struct stat file_stat;

    dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    printf("\n%-30s %-12s %-10s\n", "Name", "Type", "Size");
    printf("+--------------------------------------------------------------------+\n");

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

/* Execute a file */
void execute_file(const char *path) {
    printf("Attempting to open: %s\n", path);
#ifdef _WIN32
    if (system(path) == -1) {
        perror("Failed to open file");
    }
#else
    if (fork() == 0) {
        execl("/usr/bin/xdg-open", "xdg-open", path, (char *)NULL);
        perror("Error executing file");
        exit(1);
    }
    wait(NULL);
#endif
}

/* Main function */
int main() {
    char current_path[PATH_MAX];
    char input[MAX_INPUT];

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("Error getting current directory");
        return 1;
    }

    while (1) {
        clear_screen();
        display_header(current_path);
        list_directory(current_path);
        display_footer();

        printf("\nEnter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) break;
            perror("Error reading input");
            continue;
        }

        // Remove trailing newline
        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n')
            input[input_len - 1] = '\0';

        // Quit command
        if (strcasecmp(input, "q") == 0) {
            break;
        }

        // Move up one directory

        else if (strcasecmp(input, "u") == 0) {
            // Handle both slash types for cross-platform compatibility
            char *last_sep = strrchr(current_path, '/');
        #ifdef _WIN32
            if (last_sep == NULL) last_sep = strrchr(current_path, '\\');
        #endif
            if (last_sep != NULL && last_sep != current_path) {
                *last_sep = '\0';  // Remove the last directory from the path
            } else if (current_path[0] != '\0') {
                // If there's no separator or it's the root, we stay at root
                strcpy(current_path, "/");
            }

            // Change to the updated directory
            if (chdir(current_path) == 0) {
                if (getcwd(current_path, sizeof(current_path)) == NULL) {
                    perror("Error updating current directory");
                }
            } else {
                fprintf(stderr, "Cannot move up directory: %s\n", strerror(errno));
            }
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
                getchar();
            }
        }

        // Open or execute a file/directory
        else if (strlen(input) > 0) {
            char new_path[PATH_MAX];
            snprintf(new_path, sizeof(new_path), "%s/%s", current_path, input);

            struct stat path_stat;
            if (stat(new_path, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {
                    if (chdir(new_path) == 0) {
                        if (getcwd(current_path, sizeof(current_path)) == NULL) {
                            perror("Error updating current directory");
                            strncpy(current_path, "/", sizeof(current_path));
                        }
                    } else {
                        fprintf(stderr, "Cannot open directory: %s\n", strerror(errno));
                    }
                } else if (S_ISREG(path_stat.st_mode)) {
                    execute_file(new_path);
                    printf("Press Enter to continue...");
                    getchar();
                }
            } else {
                fprintf(stderr, "Invalid path or file: %s\n", strerror(errno));
                printf("Press Enter to continue...");
                getchar();
            }
        }
    }

    return 0;
}
