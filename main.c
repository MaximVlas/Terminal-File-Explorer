/*
    Terminal File Explorer
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")


/* Case-insensitive string search function */


char *strcasestr(const char *haystack, const char *needle) {
    size_t needle_len = strlen(needle);
    while (*haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    return NULL;
}

/* Platform-specific definitions */


#ifdef _WIN32
#include <windows.h>
#define clear_screen() system("cls")
#define strcasecmp _stricmp
#define S_ISLNK(m) (0)
#else
#include <sys/wait.h>
#define clear_screen() system("clear")
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_INPUT 256

/* Filter options structure */

typedef struct {
    long min_size;
    long max_size;
    char file_type;
    time_t min_date;
    time_t max_date;
    char search_term[MAX_INPUT];
} FilterOptions;

/* Draw a horizontal line */

void draw_line(int width, char corner, char fill) {
    putchar(corner);
    for (int i = 0; i < width - 2; i++) {
        putchar(fill);
    }
    putchar(corner);
    putchar('\n');
}

/* Display the header */

void display_header(const char *path, size_t table_width) {
    draw_line(table_width, '+', '-');
    printf("| %-*s |\n", (int)(table_width - 4), "Enhanced Terminal File Explorer");
    printf("| Path: %-*s |\n", (int)(table_width - 10), path);
    draw_line(table_width, '+', '-');
}

/* Display the footer */

void display_footer(size_t table_width) {
    draw_line(table_width, '+', '-');
    printf("| %-*s |\n", (int)(table_width - 4), "[Q]uit | [U]p | [Enter] Open/Execute | [cd <path>] Change Dir");
    draw_line(table_width, '+', '-');
}

/* Truncate long file names */

void truncate_name(const char *name, char *output, size_t max_len) {
    if (strlen(name) > max_len) {
        strncpy(output, name, max_len - 3);
        strcat(output, "...");
    } else {
        strcpy(output, name);
    }
}

/* Parse size input (e.g., 10K, 5M, 2G) */

int parse_size(const char *str, long *size) {
    char *endptr;
    *size = strtol(str, &endptr, 10);
    if (*endptr == 'K' || *endptr == 'k') *size *= 1024;
    else if (*endptr == 'M' || *endptr == 'm') *size *= 1024 * 1024;
    else if (*endptr == 'G' || *endptr == 'g') *size *= 1024 * 1024 * 1024;
    return *size >= 0;
}

/* Format date for display */

void format_date(char *buffer, size_t size, time_t timestamp) {
    struct tm *tm_info = localtime(&timestamp);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* List directory contents with filtering */

void list_directory(const char *path, FilterOptions *filter) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];
    struct stat file_stat;
    size_t max_name_len = 40;
    long max_size = 0;
    char truncated_name[PATH_MAX];
    char date_buffer[20];

    dir = opendir(path);
    if (dir == NULL) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (stat(full_path, &file_stat) == -1) continue;
        if (S_ISREG(file_stat.st_mode) && file_stat.st_size > max_size) {
            max_size = file_stat.st_size;
        }
    }
    rewinddir(dir);

    size_t size_col_width = snprintf(NULL, 0, "%ld", max_size) + 6;
    size_t table_width = max_name_len + size_col_width + 40;

    display_header(path, table_width);
    printf("%-*s %-12s %-*s %-19s\n", (int)max_name_len, "Name", "Type", (int)size_col_width, "Size", "Last Modified");
    draw_line(table_width, '|', '-');

    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (stat(full_path, &file_stat) == -1) continue;

        char file_type = S_ISDIR(file_stat.st_mode) ? 'd' : 'f';
        long file_size = file_stat.st_size;

        if (filter->file_type != '\0' && filter->file_type != file_type) continue;
        if (filter->min_size != -1 && file_size < filter->min_size) continue;
        if (filter->max_size != -1 && file_size > filter->max_size) continue;
        if (filter->min_date != 0 && file_stat.st_mtime < filter->min_date) continue;
        if (filter->max_date != 0 && file_stat.st_mtime > filter->max_date) continue;
        if (filter->search_term[0] != '\0' && strcasestr(entry->d_name, filter->search_term) == NULL) continue;

        truncate_name(entry->d_name, truncated_name, max_name_len);
        format_date(date_buffer, sizeof(date_buffer), file_stat.st_mtime);
        printf("%-*s %-12s %-*ld %-19s\n", (int)max_name_len, truncated_name, 
               file_type == 'd' ? "Directory" : "File", (int)size_col_width, file_size, date_buffer);
    }
    closedir(dir);
    draw_line(table_width, '|', '-');
    display_footer(table_width);
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
    FilterOptions filter = {-1, -1, '\0', 0, 0, ""};

    if (getcwd(current_path, sizeof(current_path)) == NULL) {
        perror("Error getting current directory");
        return 1;
    }

    while (1) {
        clear_screen();
        list_directory(current_path, &filter);

        printf("\nEnter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) break;
            perror("Error reading input");
            continue;
        }

        // Remove trailing newline
        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') input[input_len - 1] = '\0';
       
        // Process user commands
        if (strcasecmp(input, "q") == 0) {
            break;
        } else if (strcasecmp(input, "u") == 0) {
            char *last_sep = strrchr(current_path, '/');
            #ifdef _WIN32
            if (last_sep == NULL) last_sep = strrchr(current_path, '\\');
            #endif
            if (last_sep != NULL && last_sep != current_path) {
                *last_sep = '\0';
            } else if (current_path[0] != '\0') {
                strcpy(current_path, "/");
            }
            if (chdir(current_path) != 0) {
                perror("Error changing directory");
            }
        } else if (strncmp(input, "cd ", 3) == 0) {
            char *new_path = input + 3;
            if (chdir(new_path) == 0) {
                if (getcwd(current_path, sizeof(current_path)) == NULL) {
                    perror("Error updating current directory");
                    strncpy(current_path, "/", sizeof(current_path));
                }
            } else {
                perror("Cannot change directory");
            }
        } else if (strncmp(input, "filter size ", 12) == 0) {
            char *size_str = input + 12;
            char *dash = strchr(size_str, '-');
            if (dash) {
                *dash = '\0';
                parse_size(size_str, &filter.min_size);
                parse_size(dash + 1, &filter.max_size);
            } else {
                parse_size(size_str, &filter.max_size);
                filter.min_size = 0;
            }
        } else if (strncmp(input, "filter type ", 12) == 0) {
            filter.file_type = tolower(input[12]);
        } else if (strncmp(input, "filter date ", 12) == 0) {
            // Implement date filtering
        } else if (strncmp(input, "search ", 7) == 0) {
            strncpy(filter.search_term, input + 7, sizeof(filter.search_term) - 1);
        } else if (strcasecmp(input, "clear filter") == 0) {
            filter.min_size = -1;
            filter.max_size = -1;
            filter.file_type = '\0';
            filter.min_date = 0;
            filter.max_date = 0;
            filter.search_term[0] = '\0';
        } else if (strlen(input) > 0) {
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
                        perror("Cannot open directory");
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
