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

#ifdef _WIN32
  #include <windows.h>
  #include <shlwapi.h>
  #pragma comment(lib, "shlwapi.lib")
  #define CLEAR_COMMAND "cls"
  #define strcasecmp _stricmp
  #define S_ISLNK(m) (0)
#else
  #include <sys/wait.h>
  #define CLEAR_COMMAND "clear"
#endif

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

#define MAX_INPUT 256

/*
 * FilterOptions structure for filtering by size, type, date, etc.
 */
typedef struct {
    long min_size;
    long max_size;
    char file_type;       /* 'f' for files, 'd' for directories, or '\0' for no filter */
    time_t min_date;
    time_t max_date;
    char search_term[MAX_INPUT];
} FilterOptions;

/*
 * Case-insensitive substring search. 
 * On some systems strcasestr is available, but we can define our own for portability.
 */
char *strcasestr_custom(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    size_t needle_len = strlen(needle);
    while (*haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return (char *)haystack;
        }
        haystack++;
    }
    return NULL;
}

/*
 * Clear the screen using system commands.
 */
void clear_screen(void) {
    system(CLEAR_COMMAND);
}

/*
 * Draw a horizontal line for the UI.
 */
void draw_line(int width, char corner, char fill) {
    putchar(corner);
    for (int i = 0; i < width - 2; i++) {
        putchar(fill);
    }
    putchar(corner);
    putchar('\n');
}

/*
 * Display a header for the file manager output.
 */
void display_header(const char *path, size_t table_width) {
    draw_line(table_width, '+', '-');
    printf("| %-*s |\n", (int)(table_width - 4), "Enhanced Terminal File Explorer");
    printf("| Path: %-*s |\n", (int)(table_width - 10), path);
    draw_line(table_width, '+', '-');
}

/*
 * Display a footer with usage hints.
 */
void display_footer(size_t table_width) {
    draw_line(table_width, '+', '-');
    printf("| %-*s |\n", (int)(table_width - 4), "[Q]uit | [U]p | [Enter] Open/Execute | [cd <path>] Change Dir");
    draw_line(table_width, '+', '-');
}

/*
 * Nicely truncate names to keep the UI aligned.
 */
void truncate_name(const char *name, char *output, size_t max_len) {
    if (strlen(name) > max_len) {
        strncpy(output, name, max_len - 3);
        output[max_len - 3] = '\0';
        strncat(output, "...", 3);
    } else {
        strncpy(output, name, max_len);
        output[max_len] = '\0';
    }
}

/*
 * Parse size inputs like 10K, 2M, 1G for convenience filtering.
 */
int parse_size(const char *str, long *size) {
    if (!str || !size) return 0;
    char *endptr;
    *size = strtol(str, &endptr, 10);
    if (*endptr == 'K' || *endptr == 'k') {
        *size *= 1024;
    } else if (*endptr == 'M' || *endptr == 'm') {
        *size *= 1024 * 1024;
    } else if (*endptr == 'G' || *endptr == 'g') {
        *size *= 1024LL * 1024LL * 1024LL;
    }
    return (*size >= 0);
}

/*
 * Format a Unix timestamp into a standard date string.
 */
void format_date(char *buffer, size_t size, time_t timestamp) {
    struct tm tm_info;
#if defined(_WIN32)
    localtime_s(&tm_info, &timestamp);
#else
    localtime_r(&timestamp, &tm_info);
#endif
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", &tm_info);
}

/*
 * (Optional) Print in color on platforms that support ANSI escape codes.
 * We can differentiate directories vs. files. 
 * For Windows, you might need to enable Virtual Terminal Processing or skip color.
 */
void print_colored(const char *text, int is_directory) {
#ifndef _WIN32
    if (is_directory) {
        printf("\033[1;34m%s\033[0m", text); /* Blue for directories */
    } else {
        printf("\033[0m%s\033[0m", text);   /* Default color for files */
    }
#else
    /* For Windows, just print without color or set console attributes if you prefer. */
    printf("%s", text);
#endif
}

/*
 * List directory contents with the given filter.
 * This function also calculates a suitable column width to keep output aligned.
 */
void list_directory(const char *path, FilterOptions *filter) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];
    struct stat file_stat;
    size_t max_name_len = 40;
    long max_size = 0;

    /* First pass: find max file size in the directory to size the columns. */
    dir = opendir(path);
    if (!dir) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (stat(full_path, &file_stat) == -1) {
            continue;
        }
        if (S_ISREG(file_stat.st_mode) && file_stat.st_size > max_size) {
            max_size = file_stat.st_size;
        }
    }
    rewinddir(dir);

    /* Calculate columns and adjust table width. */
    size_t size_col_width = snprintf(NULL, 0, "%ld", max_size) + 6; 
    if (size_col_width < 10) size_col_width = 10; /* Minimum width for sizes */
    size_t table_width = max_name_len + size_col_width + 40;

    display_header(path, table_width);

    printf("%-*s %-12s %-*s %-19s\n",
           (int)max_name_len, "Name",
           "Type",
           (int)size_col_width, "Size",
           "Last Modified");

    draw_line(table_width, '|', '-');

    /* Second pass: actually list directory, applying filters. */
    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) == -1) {
            continue;
        }

        /* Determine type (file or directory). We skip symlinks for brevity, but you can handle them similarly. */
        char file_type = S_ISDIR(file_stat.st_mode) ? 'd' : 'f';
        long file_size = file_stat.st_size;

        /* Filter checks */
        if (filter->file_type != '\0' && filter->file_type != file_type) {
            continue;
        }
        if (filter->min_size != -1 && file_size < filter->min_size) {
            continue;
        }
        if (filter->max_size != -1 && file_size > filter->max_size) {
            continue;
        }
        if (filter->min_date != 0 && file_stat.st_mtime < filter->min_date) {
            continue;
        }
        if (filter->max_date != 0 && file_stat.st_mtime > filter->max_date) {
            continue;
        }
        if (filter->search_term[0] != '\0' &&
            !strcasestr_custom(entry->d_name, filter->search_term)) {
            continue;
        }

        /* Prepare truncated name and date. */
        char truncated_name[PATH_MAX];
        char date_buffer[20];
        truncate_name(entry->d_name, truncated_name, max_name_len);
        format_date(date_buffer, sizeof(date_buffer), file_stat.st_mtime);

        /* Print with optional color. */
        print_colored(truncated_name, (file_type == 'd'));
        size_t name_len = strlen(truncated_name);

        /* Pad spaces if the name is shorter than max_name_len. */
        if (name_len < max_name_len) {
            for (size_t i = 0; i < max_name_len - name_len; i++) {
                putchar(' ');
            }
        }

        /* Print file type, size, and date. */
        printf(" %-12s %*ld %-19s\n",
               (file_type == 'd' ? "Directory" : "File"),
               (int)size_col_width, file_size,
               date_buffer);
    }

    closedir(dir);
    draw_line(table_width, '|', '-');
    display_footer(table_width);
}

/*
 * Cross-platform "execute" function to open a file with a default application.
 * On Linux, we use xdg-open. On Windows, you can enhance with ShellExecute, etc.
 */
void execute_file(const char *path) {
    printf("Attempting to open: %s\n", path);
#ifdef _WIN32
    if ((int)ShellExecute(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL) <= 32) {
        perror("Failed to open file");
    }
#else
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/xdg-open", "xdg-open", path, (char *)NULL);
        perror("Error executing file");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork() failed");
    }
#endif
}

/*
 * Helper: reset all filters.
 */
void clear_all_filters(FilterOptions *filter) {
    filter->min_size = -1;
    filter->max_size = -1;
    filter->file_type = '\0';
    filter->min_date  = 0;
    filter->max_date  = 0;
    filter->search_term[0] = '\0';
}

/*
 * Main loop
 */
int main(void) {
    char current_path[PATH_MAX];
    char input[MAX_INPUT];
    FilterOptions filter = { -1, -1, '\0', 0, 0, "" };

    /* Get current working directory */
    if (!getcwd(current_path, sizeof(current_path))) {
        perror("Error getting current directory");
        return EXIT_FAILURE;
    }

    while (1) {
        clear_screen();
        list_directory(current_path, &filter);

        printf("\nEnter command: ");
        if (!fgets(input, sizeof(input), stdin)) {
            if (feof(stdin)) break;
            perror("Error reading input");
            continue;
        }

        /* Remove trailing newline */
        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') {
            input[input_len - 1] = '\0';
        }

        /* Process commands */
        if (strcasecmp(input, "q") == 0) {
            /* Quit the application */
            break;

        } else if (strcasecmp(input, "u") == 0) {
            /* Go up one directory, if possible */
            char *last_sep = strrchr(current_path, '/');
#ifdef _WIN32
            if (!last_sep) last_sep = strrchr(current_path, '\\');
#endif
            if (last_sep && last_sep != current_path) {
                *last_sep = '\0';
            } else if (current_path[0] != '\0') {
                strcpy(current_path, "/");
            }
            if (chdir(current_path) != 0) {
                perror("Error changing directory");
            }

        } else if (strncmp(input, "cd ", 3) == 0) {
            /* Change directory to user-specified path */
            const char *new_path = input + 3;
            if (chdir(new_path) == 0) {
                if (!getcwd(current_path, sizeof(current_path))) {
                    perror("Error updating current directory");
                    strcpy(current_path, "/");
                }
            } else {
                perror("Cannot change directory");
            }

        } else if (strncmp(input, "filter size ", 12) == 0) {
            /* e.g. "filter size 10K-1M"  or "filter size 1G" */
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
            /* e.g. "filter type f" or "filter type d" */
            filter.file_type = (char)tolower(input[12]);

        } else if (strncmp(input, "search ", 7) == 0) {
            /* Case-insensitive search of file names */
            strncpy(filter.search_term, input + 7, sizeof(filter.search_term) - 1);
            filter.search_term[sizeof(filter.search_term) - 1] = '\0';

        } else if (strcasecmp(input, "clear filter") == 0) {
            /* Reset all filters */
            clear_all_filters(&filter);

        } else if (strlen(input) > 0) {
            /*
             * If user typed something else, assume it's a file or directory in the current path.
             * Attempt to open/execute if file, or change directory if directory.
             */
            char new_path[PATH_MAX];
            snprintf(new_path, sizeof(new_path), "%s/%s", current_path, input);

            struct stat path_stat;
            if (stat(new_path, &path_stat) == 0) {
                if (S_ISDIR(path_stat.st_mode)) {
                    /* Enter directory */
                    if (chdir(new_path) == 0) {
                        if (!getcwd(current_path, sizeof(current_path))) {
                            perror("Error updating current directory");
                            strcpy(current_path, "/");
                        }
                    } else {
                        perror("Cannot open directory");
                    }
                } else if (S_ISREG(path_stat.st_mode)) {
                    /* Open file with system or xdg-open */
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

    return EXIT_SUCCESS;
}
