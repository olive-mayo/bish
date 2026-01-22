#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

/* ================= TOKENS ================= */

enum token_type {
    TOK_WORD,
    TOK_PIPE,
    TOK_REDIR_OUT,
    TOK_REDIR_APPEND,
    TOK_REDIR_IN
};

typedef struct token {
    char *value;
    enum token_type type;
    struct token *next;
} token;

/* ================= COMMAND ================= */

typedef struct command {
    char **argv;
    char *in_file;
    char *out_file;
    int append;
    struct command *next;
} command;

/* ================= UTIL ================= */

token *add_token(token *head, const char *data, int len, enum token_type type) {
    token *t = malloc(sizeof(token));
    t->value = strndup(data, len);
    t->type = type;
    t->next = NULL;

    if (!head) return t;
    token *cur = head;
    while (cur->next) cur = cur->next;
    cur->next = t;
    return head;
}

void free_tokens(token *t) {
    while (t) {
        token *n = t->next;
        free(t->value);
        free(t);
        t = n;
    }
}

/* ================= LEXER ================= */

token *lex(const char *s) {
    token *head = NULL;
    char buf[1024];
    int i = 0, j = 0;

    while (s[i]) {
        if (s[i] == ' ' || s[i] == '\t') {
            if (j) head = add_token(head, buf, j, TOK_WORD), j = 0;
        }
        else if (s[i] == '|') {
            if (j) head = add_token(head, buf, j, TOK_WORD), j = 0;
            head = add_token(head, "|", 1, TOK_PIPE);
        }
        else if (s[i] == '<') {
            if (j) head = add_token(head, buf, j, TOK_WORD), j = 0;
            head = add_token(head, "<", 1, TOK_REDIR_IN);
        }
        else if (s[i] == '>') {
            if (j) head = add_token(head, buf, j, TOK_WORD), j = 0;
            if (s[i+1] == '>') {
                head = add_token(head, ">>", 2, TOK_REDIR_APPEND);
                i++;
            } else {
                head = add_token(head, ">", 1, TOK_REDIR_OUT);
            }
        }
        else buf[j++] = s[i];
        i++;
    }
    if (j) head = add_token(head, buf, j, TOK_WORD);
    return head;
}

/* ================= PARSER ================= */

command *new_command() {
    command *c = calloc(1, sizeof(command));
    c->argv = calloc(64, sizeof(char *));
    return c;
}

command *parse(token *t) {
    command *head = new_command();
    command *cur = head;
    int argc = 0;

    while (t) {
        if (t->type == TOK_WORD) {
            cur->argv[argc++] = strdup(t->value);
        }
        else if (t->type == TOK_PIPE) {
            cur->argv[argc] = NULL;
            cur->next = new_command();
            cur = cur->next;
            argc = 0;
        }
        else if (t->type == TOK_REDIR_IN) {
            t = t->next;
            cur->in_file = strdup(t->value);
        }
        else if (t->type == TOK_REDIR_OUT || t->type == TOK_REDIR_APPEND) {
            t = t->next;
            cur->out_file = strdup(t->value);
            cur->append = (t->type == TOK_REDIR_APPEND);
        }
        t = t->next;
    }
    cur->argv[argc] = NULL;
    return head;
}

void free_commands(command *c) {
    while (c) {
        command *n = c->next;
        for (int i = 0; c->argv[i]; i++) free(c->argv[i]);
        free(c->argv);
        free(c->in_file);
        free(c->out_file);
        free(c);
        c = n;
    }
}

/* ================= BUILTINS ================= */

int builtin(command *c) {
    if (!c->argv[0]) return 1;

    if (!strcmp(c->argv[0], "exit")) exit(0);

    if (!strcmp(c->argv[0], "cd")) {
        chdir(c->argv[1] ? c->argv[1] : getenv("HOME"));
        return 1;
    }

    if (!strcmp(c->argv[0], "pwd")) {
        char buf[1024];
        getcwd(buf, sizeof(buf));
        printf("%s\n", buf);
        return 1;
    }

    if (!strcmp(c->argv[0], "echo")) {
        for (int i = 1; c->argv[i]; i++)
            printf("%s%s", c->argv[i], c->argv[i+1] ? " " : "\n");
        return 1;
    }

    if (!strcmp(c->argv[0], "type")) {
        printf("%s is a shell builtin\n", c->argv[1]);
        return 1;
    }

    return 0;
}

/* ================= EXECUTION ================= */

void execute(command *c) {
    int fd_in = 0;
    int pipefd[2];

    while (c) {
        pipe(pipefd);
        if (!builtin(c)) {
            pid_t pid = fork();
            if (pid == 0) {
                if (fd_in) dup2(fd_in, 0);
                if (c->next) dup2(pipefd[1], 1);

                if (c->in_file) {
                    int f = open(c->in_file, O_RDONLY);
                    dup2(f, 0);
                }
                if (c->out_file) {
                    int f = open(
                        c->out_file,
                        O_WRONLY | O_CREAT | (c->append ? O_APPEND : O_TRUNC),
                        0644
                    );
                    dup2(f, 1);
                }

                close(pipefd[0]);
                execvp(c->argv[0], c->argv);
                perror("exec");
                exit(1);
            }
        }
        wait(NULL);
        close(pipefd[1]);
        fd_in = pipefd[0];
        c = c->next;
    }
}

/* ================= MAIN ================= */

int main() {
    char line[1024];

    while (1) {
        printf("$ ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;

        token *t = lex(line);
        command *c = parse(t);
        execute(c);

        free_tokens(t);
        free_commands(c);
    }
    return 0;
}
