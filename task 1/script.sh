#!/bin/sh

# We use 'find -name "*.c"' to recursively find all files with *.c extension
# We then pipe 'find' program output to 'while read -r' command, which reads file paths from the pipe and
# puts them in $C_FILE variable
find -type f -name "*.c" | while read C_FILE; do

    # 'cp' program just copies one file to another, creating the new file
    cp "$C_FILE" "$C_FILE.orig"
    
    # Used for printing the shell script process
    # echo "$C_FILE ---> $C_FILE.orig"
done

