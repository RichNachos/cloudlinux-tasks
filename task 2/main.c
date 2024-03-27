#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <unistd.h>

#define BUF_SIZE 256
#define INDENT_SIZE 4

/**
 * Function: recursive_ls
 * ----------------------
 * Prints the given directory's files and subdirectories recursively
 * using the given task example's formatting
 * 
 * dir: Pointer to string containing the path to directory we want to print
 * depth: Depth defines how much indentation will be printed
 * 
 * Returns: void
*/
void recursive_ls(const char *dir, int depth);


int main(void) {
    // First we have to print the working directory's full path
    char buf[BUF_SIZE];
    if (getcwd(buf, BUF_SIZE) == NULL) {
        printf("BUF_SIZE not enough to hold full cwd path!\n");
        exit(1);
    }
    printf("%s\n", buf);

    // Now we have to recursively print out every single subdirectory and file
    recursive_ls(buf, 1);
    
    return 0;
}


void recursive_ls(const char *dir, int depth) {
    // Either this directory path is invalid, or it is a file.
    // In either case, it is a base case in recursion
    // and we return without printing any additional data.
    DIR *dirp;
    if ((dirp = opendir(dir)) == NULL) {
        return;
    }
    
    // We loop over all directory entries using this.
    // As stated in the man page, readdir will return NULL
    // if it reached the end of the directory or an error has occured
    struct dirent *dent; // holds current directory entry
    char buf[BUF_SIZE]; // holds the full path of current directory entry
    while ((dent = readdir(dirp)) != NULL) {
        // Ignore hidden files (this includes . and .. entries as well)
        if (dent->d_name[0] == '.')  {
            continue;
        }

        // Printing correct indentation for this depth
        for (int i = 0; i < INDENT_SIZE * depth; i++) {
            printf(" ");
        }
        // Print the name of this entry
        printf("%s\n", dent->d_name);

        // After printing the name of the entry, try to recursively print files
        // Note: We don't check if the entry is directory or file because our
        // recursive base case already has that case handled

        // We need to be careful here due to memory and safely concatenate entry name to full path
        // Check if buf can hold the full path to entry
        if (strlen(dir) + 1 + strlen(dent->d_name) + 1 >= BUF_SIZE) { // "dir / d_name '\0'"
            printf("BUF_SIZE not enough to hold full path!\n");
            exit(1);
        }
        strcpy(buf, dir); // Safe because both are BUF_SIZE
        buf[strlen(dir)] = '/'; // Add slash to full path (this overwrites the null byte created by previous strcpy)
        buf[strlen(dir) + 1] = '\0'; // Needed for strcat to function properly
        strcat(buf, dent->d_name); // Safe because of previous check

        // Recursive call
        recursive_ls(buf, depth + 1);
    }
}
