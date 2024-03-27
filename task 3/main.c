#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>


// DEBUG can be either 0 or 1, it only affects if stderr fd is closed or not
#define DEBUG 0

int main(int argc, char **argv) {
    if (!DEBUG) {
        close(2);
    }

    /*
        We have to implement the following shell command:
        prog1 && prog2 | prog3 > file

        Let's break down what this command does.
        First, it runs prog1 and checks its exit status
        If the exit status is success from prog1 then prog2 will be ran.
        prog2 output will be redirected through a pipe and prog3 will be ran while reading prog2 output.
        prog3 output will be redirected to write to the file 'file'

        Lets first check that the arguments are sufficent
    */
    
    if (argc < 5) {
        printf("Not enough arguments.\n");
        exit(EXIT_FAILURE);
    }

    // Lets run prog1 and check it's exit status
    // We will use execvp to search for programs in $PATH directories as well

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed!\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // From here, only child process will run
        char *args[2];
        args[0] = argv[1];
        args[1] = NULL;

        // Check if execvp fails
        if (execvp(argv[1], args) == -1) {
            fprintf(stderr, "Execvp failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process continues here
    // This waits for the child process, which is executing prog1, to exit.
    // The exit status is checked and if it is anything other than 0 (success)
    // then it stops program executing, emulating the && operator in shell.
    // Otherwise, it continues the execution of the rest of the code.
    int wstatus;
    wait(&wstatus);
    if (WEXITSTATUS(wstatus) != 0) {
        return 0;
    }

    // Here we should start prog2, pipe its output to prog3 input and output it into file.
    int fds[2];
    if (pipe(fds) != 0) { // Gotta check for failure
        fprintf(stderr, "Pipe failed.\n");
        exit(EXIT_FAILURE);
    }
    // fprintf(stderr, "%d %d\n", fds[0], fds[1]);

    // Henceforth, fd[0] refers to read end of the pipe, while fd[1] refers to write end of the pipe.
    // prog2 should have fd[1] to write to it.
    // prog3 should have fd[0] to read from it.
    // Since prog3 writes to file, we also have to implement that logic, more on that later.

    // Get argument array partialy ready for prog2 and prog3
    // Since fork duplicates all the memory, this is safe to do so.
    char *args[2];
    args[1] = NULL;
    
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed.\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // prog2 child process
        args[0] = argv[2];

        // Close fds[0] because not needed in this process
        if (close(fds[0]) != 0) {
            fprintf(stderr, "prog2 close failed on pipe fd %d.\n", fds[0]);
            exit(EXIT_FAILURE);
        }

        // Close stdout fd and replace it with write end of the pipe (fd[1])
        if (close(1) != 0) {
            fprintf(stderr, "prog2 close failed on stdout fd.\n");
            exit(EXIT_FAILURE);
        }

        // dup syscall duplicates the given fd and assigns a new fd to the same file or pipe referred to old fd.
        // We use the functionality of dup using the lowest number integer availabe for fds to replace stdout.
        if (dup(fds[1]) == -1) {
            fprintf(stderr, "prog2 dup failed on pipe fd %d.\n", fds[1]);
            exit(EXIT_FAILURE);
        }

        // Finally, after setting up file descriptors, we use execvp to execute prog2
        // prog2 will have the modified file descriptors, so we can be sure it will be writing through the pipe

        // execvp fail checking
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Execvp failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork failed!\n");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        // prog3 child process
        args[0] = argv[3];

        // Close fds[1] because not needed in this process
        if (close(fds[1]) != 0) {
            fprintf(stderr, "prog2 close failed on pipe fd %d.\n", fds[1]);
            exit(EXIT_FAILURE);
        }

        // Close stdin fd and replace it with read end of the pipe (fd[0])
        if (close(0) != 0) {
            fprintf(stderr, "prog3 close failed on stdin fd.\n");
            exit(EXIT_FAILURE);
        }

        // Basically the same code as prog2 except we change the pipe we're dup-ing.
        if (dup(fds[0]) != 0) {
            fprintf(stderr, "prog3 dup failed on pipe fd %d.\n", fds[0]);
            exit(EXIT_FAILURE);
        }

        // Also, additional to this prog is that we have to write to file so lets implement that
        // Lets close the stdout fd and open the file, which will be the replacement fd for the closed stdout
        if (close(1) != 0) {
            fprintf(stderr, "prog3 close failed on stdout fd.\n");
            exit(EXIT_FAILURE);
        }

        // We also check if the open has failed for some reason
        // Also note: we're implementing prog3 > file, which means that when prog3 outputs to the file,
        // it has to overwrite the data if the file exists, and create the file if it doesn't exist.
        // We set the appropriate flags for open for that reason.
        // open will return the lowest available fd integer, in our case since we closed stdout fd, it will choose 1.
        if (open(argv[4], O_CREAT | O_WRONLY, S_IRWXO | S_IRWXG | S_IRWXU) == -1) {
            fprintf(stderr, "prog3 open file %s has failed.\n", argv[4]);
            exit(EXIT_FAILURE);
        }
        
        // Finally, start executing the given prog3 with redirected i/o
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Execvp failed.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    if ((close(fds[0]) & close(fds[1])) != 0) {
        fprintf(stderr, "Failed to close fds in parent process\n");
        exit(EXIT_FAILURE);
    }
    // fprintf(stderr, "Closed pipe fds in parents\n");

    // Parent process must wait for children or if it exits before the children finish executing,
    // the kernel will kill the children and not let them finish their job.
    for (int i = 1; i <= 2; i++) {
        // We don't care about children statuses.
        // If we wanted, we could implement additional logic here
        wait(NULL);
        fprintf(stderr, "%d Child terminated\n", i);
    }

    return 0;
}
