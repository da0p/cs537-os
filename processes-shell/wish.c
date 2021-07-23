#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>

#define MAX_ARGS_NUM 10
#define MAX_SEARCH_PATHS 20
#define DELIM_STR " \n"
#define EMPTY_SPACE ""
#define INITIAL_SEARCH_PATH "/bin"
#define IN_REDIR    "<"
#define OUT_REDIR   ">"

typedef struct cmd_t {
    char **arg;
    int redir;
    char *fname;
}cmd_t;

void err_msg (void) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
}

char **create_search_paths(void) {

    char **paths = (char**) malloc(MAX_SEARCH_PATHS * sizeof(char*));

    for (size_t i = 0; i < MAX_SEARCH_PATHS; i++)
        paths[i] = NULL;

    paths[0] = (char *) malloc((strlen(INITIAL_SEARCH_PATH) + 1) * sizeof(char));
    strncpy(paths[0], INITIAL_SEARCH_PATH, strlen(INITIAL_SEARCH_PATH));

    return paths;
}

void insert_paths(char **paths, char **dirs) {

    size_t i = 0;
    
    while (dirs[i] != NULL) {
        if (i < MAX_SEARCH_PATHS - 1) {
            paths[i] = (char*) malloc((strlen(dirs[i]) + 1) * sizeof(char)); 
            strncpy(paths[i], dirs[i], strlen(dirs[i]) * sizeof(char)); 
            i++;
        }
    }

    while (paths[i] != NULL && i < MAX_SEARCH_PATHS - 1) {
        free(paths[i]);
        paths[i] = NULL;
        i++;
    }
}

void free_paths(char **paths) {
    for (size_t i = 0; paths[i] != NULL; i++) {
        free(paths[i]);
    }
    free(paths);
}

void show_paths(char **paths) {
    for (size_t i = 0; paths[i] != NULL; i++) {
        printf("%s, ", paths[i]);
    }
}

char *cmd_avail(char **paths, char *cmd) {

    char *path = (char *) malloc(PATH_MAX * sizeof(char));
    
    for (size_t i = 0; paths[i] != NULL; i++) {
        memset(path, 0, PATH_MAX);          
        sprintf(path, "%s/%s", paths[i], cmd);
        if (access((const char*)path, X_OK) == 0) return path;
    }

    free(path);
    return NULL;
}

void free_cmd(cmd_t *cmd) {
    free(cmd->arg);
    if (cmd->fname != NULL)
        free(cmd->fname);
    free(cmd);
}

cmd_t *cmd_parse(char *str, const char *delim) {
    char *found = NULL;
    size_t num_args = 0;
    size_t max_args_num = MAX_ARGS_NUM;
    cmd_t *cmd = NULL;

    cmd = (cmd_t*)malloc(sizeof(cmd_t));
    cmd->arg = (char **) malloc(sizeof(char*) * MAX_ARGS_NUM);
    if (cmd->arg == NULL) err_msg();

    cmd->redir = -1;
    cmd->fname = NULL;

    while ((found = strsep(&str, delim)) != NULL) {
        if (strcmp(EMPTY_SPACE, found) != 0) {
            if (strcmp(IN_REDIR, found) == 0) {
                cmd->redir = 0;
            }
            else if (strcmp(OUT_REDIR, found) == 0){
                cmd->redir = 1;
            }
            else {
                if (cmd->redir != -1 ) {
                    if (cmd->fname == NULL) {
                        cmd->fname = (char *) malloc((strlen(found) + 1) * sizeof(char));
                        strncpy(cmd->fname, found, strlen(found));
                    }
                    else {
                        // more than one files after redirected sign
                        free_cmd(cmd);
                        err_msg();
                    }
                }
                else {
                    cmd->arg[num_args++] = found;
                    // one left for NULL
                    if (num_args > max_args_num - 2) {
                        max_args_num *= 2;
                        char **re_arg = (char**) realloc(cmd->arg, max_args_num * sizeof(char*));
                        if (re_arg == NULL) {
                            break;
                        }
                        cmd->arg = re_arg;
                    }
                }
            }
        }
    }
    // Make sure the last pointer pointing to NULL
    cmd->arg[num_args] = NULL;

    return cmd;
}

void redirect(cmd_t *cmd) {
    int fd;
    if (cmd != NULL && cmd->redir != -1) {
        fd = open(cmd->fname, O_CREAT | O_RDWR, 
                                  S_IRUSR | S_IWUSR | 
                                  S_IRGRP | S_IWGRP |
                                  S_IROTH | S_IWOTH);
        if (cmd->redir == 0) {
            if (fd != STDIN_FILENO) {
                dup2(fd, STDIN_FILENO);
            }
        }
        else if (cmd->redir == 1) {
            if (fd != STDOUT_FILENO) {
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }
    }
}

int main (int argc, char *argv[]) {

    char *line = NULL;
    int eof = 0;
    size_t n = 0;
    cmd_t *cmd = NULL;
    char *full_cmd = NULL;
    char **paths = NULL;

    paths = create_search_paths();
    while (eof != -1) {
        printf("wish> ");
        eof = getline(&line, &n, stdin);
        if (eof != -1) {
            cmd = cmd_parse(line, DELIM_STR);
            if (strcmp("cd", cmd->arg[0]) == 0) {
                chdir(cmd->arg[1]);
            }
            else if (strcmp("exit", cmd->arg[0]) == 0) {
                free_cmd(cmd);
                exit(0);
            }
            else if (strcmp("path", cmd->arg[0]) == 0) {
                insert_paths(paths, &cmd->arg[1]);    
                //show_paths(paths);
            }
            else {
                full_cmd = cmd_avail(paths, cmd->arg[0]);
                if (full_cmd != NULL) {
                   pid_t wsh = fork();

                    if (wsh == 0) {
                        redirect(cmd);
                        cmd->arg[0] = full_cmd;
                        execv(full_cmd, cmd->arg);
                        err_msg();
                    }
                    else {
                        wait(NULL);
                        free(full_cmd);
                        free_cmd(cmd);
                    }
                }
            }
        }
    }
    free_paths(paths);
    free(line);

    return 0;
}
