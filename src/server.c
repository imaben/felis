#include "server.h"
#include "felis.h"
#include "log.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "workerqueue.h"

#define SOCKET_READ_TIMEOUT 10
#define SOCKET_WRITE_TIMEOUT 10

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

static void close_client(felis_client_t *client)
{
    if (client && client->fd >= 0) {
        close(client->fd);
        client->fd = -1;
    }
}

static void release_client(felis_client_t *client)
{
    if (NULL == client) {
        return;
    }
    close_client(client);
    if (client->buf_ev) {
        bufferevent_free(client->buf_ev);
        client->buf_ev = NULL;
    }
    if (client->evbase) {
        event_base_free(client->evbase);
        client->evbase = NULL;
    }
    if (client->output_buffer) {
        evbuffer_free(client->output_buffer);
        client->output_buffer = NULL;
    }
    free(client);
}

static void buffered_on_read(struct bufferevent *bev, void *arg)
{

}

static void buffered_on_write(struct bufferevent *bev, void *arg)
{
}

static void buffered_on_error(struct bufferevent *bev, short what, void *arg)
{
	close_client((felis_client_t *)arg);
}

static void server_job_function(struct job *job) {
	felis_client_t *client = (felis_client_t *)job->user_data;

	event_base_dispatch(client->evbase);
	release_client(client);
	free(job);
}
static void on_accept(int fd, short ev, void *arg)
{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    workqueue_t *workqueue = (workqueue_t *)arg;
    felis_client_t *client;
    job_t *job;

    client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        log_warn("accept failed");
        return;
    }

    if (setnonblock(client_fd) < 0) {
        log_warn("Failed to set client socket to non-blocking");
        close(client_fd);
        return;
    }

    if (NULL == (client = malloc(sizeof(*client)))) {
        log_warn("Failed to allocate memory for client");
        close(client_fd);
        return;
    }
    memset(client, 0, sizeof(*client));
    client->fd = client_fd;

    if (NULL == (client->output_buffer = evbuffer_new())) {
        log_warn("Client output buffer allocation failure");
        release_client(client);
        return;
    }

    if (NULL == (client->evbase = event_base_new())) {
        log_warn("Client event base creation failure");
        release_client(client);
        return;
    }

    if (NULL == (client->buf_ev = bufferevent_new(client_fd,
                    buffered_on_read, buffered_on_write,
                    buffered_on_error, client))) {
        log_warn("Client bufferevent creation failure");
        release_client(client);
        return;
    }
    bufferevent_base_set(client->evbase, client->buf_ev);
    bufferevent_settimeout(client->buf_ev,
            SOCKET_READ_TIMEOUT, SOCKET_WRITE_TIMEOUT);

    bufferevent_enable(client->buf_ev, EV_READ);

    if (NULL == (job = malloc(sizeof(*job)))) {
        log_warn("Failed to allocate memory for job");
        release_client(client);
        return;
    }
    job->job_cb = server_job_function;
    job->user_data = client;

    workqueue_append(workqueue, job);

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

    /* Initailize worker queue */
    if (workqueue_init(&ctx->workqueue, cfg->threads) < 1) {
        log_error("Failed to initailize work queue");
        workqueue_shutdown(&ctx->workqueue);
        return -1;
    }

    event_set(&ev_accept, ctx->listenfd, EV_READ|EV_PERSIST,
            on_accept, (void *)&ctx->workqueue);
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
