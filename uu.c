#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

struct exec {
    char *name;
    char *files[32];
    char *opts[32];
};

static int _uu_forkexec(struct exec exec)
{
    char *argv[64];
    {
        int i = 0;

        fputs("[CMD] ", stdout);

        argv[i] = "cc";
        printf("%s ", argv[i]);
        i += 1;

        for (int o = 0; exec.opts[o]; o += 1) {
            argv[i] = exec.opts[o];
            printf("%s ", argv[i]);
            i += 1;
        }

        argv[i] = "-o";
        printf("%s ", argv[i]);
        i += 1;

        argv[i] = exec.name;
        printf("%s ", argv[i]);
        i += 1;

        for (int f = 0; exec.files[f]; f += 1) {
            argv[i] = exec.files[f];
            printf("%s ", argv[i]);
            i += 1;
        }

        argv[i] = NULL;
        putchar('\n');
    }

    int pid = fork();
    switch (pid) {
    case 0:
        execvp(argv[0], argv);
        perror("ERROR: execvp");
        exit(EXIT_FAILURE);
    case -1:
        perror("ERROR: fork");
        exit(EXIT_FAILURE);
    default: {
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("ERROR: waitpid");
            exit(EXIT_FAILURE);
        }
        if (!WIFEXITED(status)) {
            perror("ERROR: waitpid: Child was terminated by a third party entity");
            exit(EXIT_FAILURE);
        }
        return WEXITSTATUS(status);
    }
    }
}

static int uu(struct exec exec)
{
    struct stat st_exec;
    if (stat(exec.name, &st_exec) < 0) {
        if (errno != ENOENT) {
            perror("ERROR: stat exec");
            exit(EXIT_FAILURE);
        }
        return _uu_forkexec(exec) == 0;
    }

    for (int i = 0; exec.files[i]; i += 1) {
        struct stat st_file;
        if (stat(exec.files[i], &st_file) < 0) {
            perror("ERROR: stat file");
            exit(EXIT_FAILURE);
        }
        if (st_exec.st_mtime < st_file.st_mtime) {
            return _uu_forkexec(exec) == 0;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (uu((struct exec) { argv[0], { __FILE__ }, { "-O3", "-std=c17" } })) {
        execvp(argv[0], (char *[]) { argv[0], NULL });
        perror("ERROR: execvp");
        exit(EXIT_FAILURE);
    }

    uu((struct exec) {
        "test",
        { "rlenc.h" },
        { "-ggdb", "-O0", "-xc", "-std=c99",
          "-Wall", "-Wpedantic", "-Wmissing-prototypes",
          "-DRLENC_IMPLEMENTATION", "-DRLENC_TEST" },
    });

    return EXIT_SUCCESS;
}
