#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include "error.h"
#include "util.h"
#include "socket.h"

int create_socket(const char *path)
{
    struct sockaddr_un addr;

    if (strlen(path) + 1 > ARRAY_SIZE(addr.sun_path)) {
        errno = E2BIG;
        return -1;
    }

    unlink(path);

    const int sockfd = CHK(socket(AF_LOCAL, SOCK_DGRAM, 0));

    strcpy(addr.sun_path, path);
    addr.sun_family = AF_LOCAL;

    CHK(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)));

    CHK(chmod(path, 0666));

    return sockfd;
}
