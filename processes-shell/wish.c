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
#define MAX_REDIR_FILES 10
#define MAX_SEARCH_PATHS 20
#define MAX_PARALLEL_CMDS 20
#define DELIM_STR " \n"
#define PAR_DELIM "&\n"
#define EMPTY_SPACE ""
#define INITIAL_SEARCH_PATH "/bin"
#define IN_REDIR    "<"
#define OUT_REDIR   ">"

typedef struct cmd_t {
    char abs_name[PATH_MAX];
    char **arg;
    char **fname;
    int redir;
    size_t num_args;
    size_t num_files;
}cmd_t;

void err_msg (int err_code, bool quit) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    if (quit)
        exit(err_code);
}

cmd_t *create_cmd (void) {
    cmd_t *cmd = (cmd_t*)malloc(sizeof(cmd_t));
    if (cmd == NULL) err_msg(1, true);

    cmd->arg = (char **) malloc(MAX_ARGS_NUM * sizeof(char *));
    if (cmd->arg == NULL) err_msg(1, true);
    for (size_t i = 0; i < MAX_ARGS_NUM; i++) cmd->arg[i] = NULL;

    cmd->fname = (char **) malloc(MAX_REDIR_FILES * sizeof(char *));
    if (cmd->fname == NULL) err_msg(1, true);
    for (size_t i = 0; i < MAX_REDIR_FILES; i++) cmd->fname[i] = NULL;

    memset(cmd->abs_name, 0, PATH_MAX);
    cmd->redir = 0;
    cmd->num_args = 0;
    cmd->num_files = 0;

    return cmd;
}

void free_cmd (cmd_t *cmd) {
    free(cmd->arg);
    free(cmd->fname);
    free(cmd);
}

char **create_search_paths (void) {

    char **paths = (char**) malloc(MAX_SEARCH_PATHS * sizeof(char*));
    if (paths == NULL) return NULL;

    for (size_t i = 0; i < MAX_SEARCH_PATHS; i++)
        paths[i] = NULL;

    return paths;
}

int initialize_search_path(char **paths) {
    
    if (paths == NULL) return -1;

    paths[0] = (char *) malloc((strlen(INITIAL_SEARCH_PATH) + 1) * sizeof(char));
    if (paths[0] == NULL) {
        free(paths);
        return -1;
    }
    strncpy(paths[0], INITIAL_SEARCH_PATH, strlen(INITIAL_SEARCH_PATH));

    return 0;
}

void insert_paths (char **paths, char **dirs) {

    size_t i = 0;
    
    while (dirs[i] != NULL) {
        if (i < MAX_SEARCH_PATHS - 1) {
            if (paths[i] != NULL) {
                free(paths[i]);
                paths[i] = NULL;
            }
            paths[i] = (char*) malloc((strlen(dirs[i]) + 1) * sizeof(char)); 
            if (paths[i] == NULL) return;
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

void show_paths (char **paths) {
    for (size_t i = 0; paths[i] != NULL; i++) {
        printf("%s, ", paths[i]);
    }
}

void free_paths (char **paths) {
    for (size_t i = 0; paths[i] != NULL; i++) {
        free(paths[i]);
    }
    free(paths);
}

cmd_t *single_cmd_parse (char *str) {
    char *found = NULL;
    char *tmp = NULL;
    size_t max_args_num = MAX_ARGS_NUM;
    cmd_t *cmd = create_cmd();

    while ((found = strsep(&str, DELIM_STR)) != NULL)
        if (strcmp(EMPTY_SPACE, found) != 0) { 
            cmd->arg[cmd->num_args++] = found;
            break;
        }

    found = strsep(&str, OUT_REDIR);
    if (found != NULL) {
        while ((tmp = strsep(&found, DELIM_STR)) != NULL) {
            if (strcmp(EMPTY_SPACE, tmp) != 0)
                cmd->arg[cmd->num_args++] = tmp;
        }
    }
    
    while ((found = strsep(&str, OUT_REDIR)) != NULL) {
        cmd->redir++;
        while ((tmp = strsep(&found, DELIM_STR)) != NULL) {
            if (strcmp(EMPTY_SPACE, tmp) != 0)  {
                cmd->fname[cmd->num_files++] = tmp;
            }
        }
    }
   // Make sure the last pointer pointing to NULL
    cmd->arg[cmd->num_args] = NULL;
    cmd->fname[cmd->num_files] = NULL;

    return cmd;
}

char  **parallel_cmd_parse (char *str) {
   
    char *found = NULL;
    char **pcmds = NULL;
    size_t i = 0;

    pcmds = (char **) malloc (MAX_PARALLEL_CMDS * sizeof(char *));
    if (pcmds == NULL) err_msg(1, true);

    for (i = 0; i < MAX_PARALLEL_CMDS; i++)
        pcmds[i] = NULL;
    i = 0;
    while ((found = strsep(&str, PAR_DELIM)) != NULL) {
        if (strcmp(EMPTY_SPACE, found) != 0) {
            pcmds[i++] = found;
        }
    }
    
    pcmds[i] = NULL;
    return pcmds;
}

void free_parallel_cmds (char **pcmds) {
    free(pcmds);
}

int change_dir (cmd_t *cmd) {
    if (cmd->num_args != 2) {
        err_msg(0, false);
        return -1;
    }
    chdir(cmd->arg[1]);
    return 0;
}

void exit_shell (cmd_t *cmd) {

    if (cmd->num_args > 1) {
        err_msg(0, false);
    }
}

int redirect_output (cmd_t *cmd) {
    if (cmd == NULL) return -1;

    if (cmd->redir != cmd->num_files)
        return -1;

    if (cmd->num_files > 1)
        return -1;

   int fd = open(cmd->fname[0], O_CREAT | O_RDWR,
                                S_IRUSR | S_IWUSR |
                                S_IRGRP | S_IWGRP |
                                S_IROTH | S_IWOTH);

   if (fd != STDOUT_FILENO) {
       dup2(fd, STDOUT_FILENO);
       close(fd);
   }
    return 0;
}

int cmd_avail (char **paths, cmd_t *cmd) {

    if (paths == NULL || cmd == NULL) return -1;

    for (size_t i = 0; paths[i] != NULL; i++) {
        memset(cmd->abs_name, 0, PATH_MAX);          
        if (cmd->arg[0] != NULL) {
            sprintf(cmd->abs_name, "%s/%s", paths[i], cmd->arg[0]);
            if (access((const char*)cmd->abs_name, X_OK) == 0) return 0;
        }
    }

    return -1;
}

void execute_cmd (char **paths, cmd_t *cmd, size_t *j) {
    int avail = -1;
    avail = cmd_avail(paths, cmd);
    if (avail == 0) {
        pid_t wsh = fork();

        if (wsh == 0) {
            if (redirect_output(cmd) != 0) {
                err_msg(0, false);
            } else {
                execv(cmd->abs_name, cmd->arg);
                err_msg(1, true);
            }
        }
        else {
            *j = *j + 1;
        }
    } else {
        err_msg(0, false);
    }
}

void wait_all_procs(size_t j) {
    size_t k = 0;
    while (k < j) {
        wait(NULL);
        k++;
    }
}

int read_line (bool rd_mode, char **line, size_t *n, FILE *fp) {
    ssize_t  eof = 0;

    if (rd_mode) { 
        printf("wish> ");
        eof = getline(line, n, stdin);
    } else {
        eof = getline(line, n, fp);
    }
    return eof;
}

int main (int argc, char *argv[]) {

    char *line = NULL;
    ssize_t eof = 0;
    size_t n = 0, i = 0, j = 0;
    bool rd_mode = true;
    cmd_t *cmd = NULL;
    char **pcmds = NULL;
    char **paths = NULL;
    FILE *fp = NULL;

    if (argc > 2) {
        err_msg(1, true);
    }

    if (argc == 2) {
        rd_mode = false;
        fp = fopen(argv[1], "r");
        if (fp == NULL) err_msg(1, true);
    }
    paths = create_search_paths();
    if (paths == NULL) err_msg(1, true);
    if (initialize_search_path(paths) != 0) err_msg(1, true);
    while (eof != -1) {
        eof = read_line(rd_mode, &line, &n, fp);
        if (eof != -1) {
            j = 0;
            i = 0;
            pcmds = parallel_cmd_parse(line);
            while (pcmds != NULL && pcmds[i] != NULL) {
                cmd = single_cmd_parse(pcmds[i]);
                if (cmd != NULL) {
                    if (cmd->arg[0] != NULL) {
                        if (strcmp("cd", cmd->arg[0]) == 0) {
                            int r = change_dir(cmd);
                            if (r != 0) goto exit_clean;
                        }
                        else if (strcmp("exit", cmd->arg[0]) == 0) {
                            exit_shell(cmd);
                            goto exit_clean;
                        }
                        else if (strcmp("path", cmd->arg[0]) == 0) {
                            insert_paths(paths, &cmd->arg[1]);    
                        }
                        else {
                            execute_cmd(paths, cmd, &j); 
                            }
                        }
                    i++;

                free_cmd(cmd);
                cmd = NULL;
                }
            }
            wait_all_procs(j);
            free_parallel_cmds(pcmds);
            pcmds = NULL;
        }
    }

exit_clean:
    if (cmd != NULL) free_cmd(cmd);
    if (pcmds != NULL) free_parallel_cmds(pcmds);
    free_paths(paths);
    free(line);
    exit(0);
}
