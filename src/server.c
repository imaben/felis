#include "server.h"
#include "felis.h"
#include "log.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

static int setnonblock(int fd) {
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags < 0)
        return flags;
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0)
        return -1;
    return 0;
}

int server_start()
{
    /* Initialize libevent. */
    event_init();

    felis_config_t *cfg = get_config();
    felis_ctx_t *ctx = get_ctx();
    struct sockaddr_in listen_addr;
    struct event ev_accept;
    int flags;
    struct linger ln = {0, 0};

    ctx->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->listenfd < 0) {
        log_error("failed to listen socket");
        return -1;
    }
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(inet_addr(cfg->listen_host));
    listen_addr.sin_port = htons(cfg->listen_port);

    if (bind(ctx->listenfd, (struct sockaddr *)&listen_addr,
                sizeof(listen_addr)) < 0) {
        log_error("failed to bind %s:%d", cfg->listen_host, cfg->listen_port);
        return -1;
    }

    if (listen(ctx->listenfd, 128) < 0) {
        log_error("listen failure");
        return -1;
    }

    setsockopt(ctx->listenfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(ctx->listenfd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
    setsockopt(ctx->listenfd, SOL_SOCKET, SO_LINGER, &ln, sizeof(ln));
#if !defined(TCP_NOPUSH) && defined(TCP_NODELAY)
    setsockopt(ctx->listenfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
#endif

    if (setnonblock(ctx->listenfd) < 0) {
        log_error("failed to set server socket to non-blocking");
    }

    if (NULL == (ctx->evbase = event_base_new())) {
        log_error("Unable to create socket accept event base");
        close(ctx->listenfd);
        return -1;
    }

    event_set(&ev_accept, ctx->listenfd, EV_READ|EV_PERSIST, NULL, NULL);
    event_base_set(ctx->evbase, &ev_accept);
    event_add(&ev_accept, NULL);

    log_notice("Server running");

    event_base_dispatch(ctx->evbase);

    event_base_free(ctx->evbase);
    ctx->evbase = NULL;
    close(ctx->listenfd);

    log_notice("Server shutdown");
    return 0;
}

void server_shutdown()
{
    felis_ctx_t *ctx = get_ctx();
    if (event_base_loopexit(ctx->evbase, NULL)) {
        log_error("Error shutting down server");
    }
}
