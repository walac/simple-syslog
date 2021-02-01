#define _GNU_SOURCE 1
#define _POSIX_C_SOURCE 1
#include <getopt.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include "socket.h"
#include "message_counter.h"
#include "error.h"

static int pid_file;
static char pid_pathname[PATH_MAX];
static char most_freq_msg[BUFFER_SIZE];
static size_t most_freq_size;
static size_t most_freq_count;

struct message {
    char buffer[BUFFER_SIZE];
    ssize_t size;
};

static void swap_messages(struct message **m1, struct message **m2)
{
    struct message *tmp = *m1;
    *m1 = *m2;
    *m2 = tmp;
}

static void usage(const char *name)
{
    fprintf(stderr, "%s - run a log server\n", name);
    fprintf(stderr, "%s [-f|--daemonize] [-s|--socket-path <path>] [-h|--help] [<log-file1> <log-file2> ...]\n", name);
    fprintf(stderr, "\t-f|--daemonize - run the process as a daemon\n");
    fprintf(stderr, "\t-s|--socket-path <path> - path to the unix socket file\n");
    fprintf(stderr, "\t-h|--help - print this message\n");
}

static void close_pid_file(void)
{
    close(pid_file);
    unlink(pid_pathname);
}

static void handle_signal(int sig)
{
    (void) sig;

    if (most_freq_count) {
        write(STDOUT_FILENO, most_freq_msg, most_freq_size);
        write(STDOUT_FILENO, "\n", 1);
    }

    exit(EXIT_SUCCESS);
}

static void install_signal_handler(int sig)
{
    struct sigaction act, oldact;
    memset(&act, 0, sizeof act);
    act.sa_handler = handle_signal;
    sigemptyset(&act.sa_mask);
    CHK(sigaction(sig, &act, &oldact));
}

static const char *find_digit(const char *s, const char *boundary)
{
    while (s < boundary && !isdigit(*s)) ++s;
    return s;
}

// Look for HH:MM:SS or HH-MM-SS format and skip it
static const char *skip_timestamp(const char *msg, size_t size)
{
#define CHECK_DIGIT(p) if (p >= boundary || !isdigit(*p)) continue
#define CHECK_SEP(p) if (p >= boundary || (*p != ':' && *p != '-')) continue
#define CHECK_CHUNK(p) \
    do { \
        CHECK_DIGIT(p); \
        ++p; \
        CHECK_DIGIT(p); \
        ++p; \
    } while (0);

    const char *p = msg;
    const char *boundary = msg + size;
    while (p < boundary) {
        p = find_digit(p, boundary);
        if (p == boundary) break;
        CHECK_CHUNK(p);
        CHECK_SEP(p);
        ++p;
        CHECK_CHUNK(p);
        CHECK_SEP(p);
        ++p;
        CHECK_CHUNK(p);
        break;
        
    }

    // If the timestamp was not found return the whole message
    return p < boundary ? p : msg;
#undef CHECK_CHUNK
#undef CHECK_DIGIT
#undef CHECK_POS
}

int main(int argc, char *argv[])
{
    int run_as_daemon = 0;
    const char *socket_path = "/dev/log";
    const char *optstr = "fhs:";

    static const struct option long_opts[] = {
        { "help", 0, NULL, 'h' },
        { "daemonize", 0, NULL, 'f' },
        { "socket-path", 1, NULL, 's' },
        { NULL, 0, NULL, 0 },
    };

    int optch;

    while ((optch = getopt_long(argc, argv, optstr, long_opts, NULL)) != EOF) {
        switch (optch) {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'f':
                run_as_daemon = 1;
                break;
            case 's':
                socket_path = optarg;
                break;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (run_as_daemon) {
        CHK(daemon(0, 0));

        const ssize_t written = CHK(snprintf(pid_pathname, PATH_MAX, "/var/run/logging-daemon.%lu", (unsigned long) getpid()));
        if (written > PATH_MAX) {
            fputs("Invalid pid file\n", stderr);
            return EXIT_FAILURE;
        }

        pid_file = (int) CHK(open(pid_pathname, O_CREAT | O_EXCL, 0600));
        atexit(close_pid_file);
    }

    install_signal_handler(SIGINT);
    install_signal_handler(SIGTERM);

    size_t nfiles;
    int *fds;

    if (run_as_daemon) {
        nfiles = 1;
        fds = CHK_P(calloc(1, sizeof(int)));
        fds[1] = (int) CHK(open("/var/log/messages", O_WRONLY | O_APPEND | O_CREAT, 0660));
    } else {
        nfiles = argc - optind;
        fds = CHK_P(calloc(nfiles + 1, sizeof(int)));

        for (size_t i = 0; i < nfiles; ++i)
            fds[i+1] = (int) CHK(open(argv[optind + i], O_WRONLY | O_APPEND | O_CREAT, 0666));
    }

    fds[0] = STDOUT_FILENO;

    struct message buffer[2] = {
        { {'\0'}, 0 },
        { {'\0'}, 0 },
    };

    struct message *current = buffer;
    struct message *last = buffer + 1;

    message_counter_t counter;
    msgcount_init(&counter);

    const int log_socket = create_socket(socket_path);

    do {
        while ((current->size = read(log_socket, current->buffer, BUFFER_SIZE)) >= 0) {
            if (current->size != 0) {
                // detect duplicate messages
                if (current->size == last->size && !memcmp(current->buffer, last->buffer, current->size))
                    continue;

                /*
                 * The format we receive from the logger client can vary a lot
                 * (see https://www.rsyslog.com/doc/master/whitepapers/syslog_parsing.html)
                 * The strategy that we adopted is to skip the timestamp and then
                 * compare the messages
                 */
                const char *p = skip_timestamp(current->buffer, current->size);
                const size_t size = current->size - (p - current->buffer);
                const size_t msg_count = msgcount_incr(&counter, p, size);

                if (msg_count > most_freq_count) {
                    memcpy(most_freq_msg, current->buffer, current->size);
                    most_freq_size = current->size;
                    most_freq_count = msg_count;
                }

                for (size_t i = 0; i <= nfiles; ++i) {
                    CHK(write(fds[i], current->buffer, current->size));
                    CHK(write(fds[i], "\n", 1));
                }
            }

            swap_messages(&current, &last);
        }
    } while (errno == EINTR);

    return 0;
}
