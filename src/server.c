#include "server.h"
#include "felis.h"
#include "log.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <evhttp.h>
#include "cJSON.h"
#include "smart_str.h"

#define SOCKET_READ_TIMEOUT 10
#define SOCKET_WRITE_TIMEOUT 10

#define get_buffer_from_req(body, req) do { \
    smart_str *__str = (smart_str *)body; \
    struct evbuffer *__evbuff = evhttp_request_get_input_buffer(req); \
    int __n = 0; \
    char __buf[128]; \
    while ((__n = evbuffer_remove(__evbuff, __buf, 128)) > 0) { \
        smart_str_appendl(__str, __buf, __n); \
    } \
    smart_str_0(__str); \
 } while(0)

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

static int socket_listen(char *addr, int port)
{
    int listenfd;
    struct sockaddr_in listen_addr;
    int flags;
    struct linger ln = {0, 0};

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        log_error("failed to listen socket");
        return -1;
    }
    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(inet_addr(addr));
    listen_addr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&listen_addr,
                sizeof(listen_addr)) < 0) {
        log_error("failed to bind %s:%d", addr, port);
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, 128) < 0) {
        close(listenfd);
        log_error("listen failure");
        return -1;
    }

    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
    setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &ln, sizeof(ln));
#if !defined(TCP_NOPUSH) && defined(TCP_NODELAY)
    setsockopt(listenfd, IPPROTO_TCP, TCP_NODELAY, &flags, sizeof(flags));
#endif

    if (setnonblock(listenfd) < 0) {
        close(listenfd);
        log_error("failed to set server socket to non-blocking");
        return -1;
    }
    return listenfd;
}

static void output(struct evhttp_request *req, char *content, int code, int json)
{
    struct evbuffer *buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s", content);
    if (json) {
        evhttp_add_header(req->output_headers,
                "Content-Type", "application/json; charset=utf-8");
    }
    evhttp_send_reply(req, code, NULL, buf);
    evbuffer_free(buf);
}

#define send_bad_request(req, msg) \
    output(req, msg, HTTP_BADREQUEST, 0)
#define send_normal_request(req, msg) \
    output(req, msg, HTTP_OK, 1)
#define send_internal_request(req, msg) \
    output(req, msg, HTTP_INTERNAL, 0)
#define send_bad_method(req, msg) \
    output(req, msg, HTTP_BADMETHOD, 0)
#define send_not_found(req, msg) \
    output(req, "{\"result\": false, \"msg\":" #msg "}", HTTP_NOTFOUND, 0)
#define send_result_failure(req, msg) \
    send_normal_request(req, "{\"result\": false, \"msg\":" #msg "}")
#define send_params_errors(req) \
    send_result_failure(req, "params error")
#define send_result_success(req) \
    send_normal_request(req, "{\"result\": true}")

static void http_dict_list(struct evhttp_request *req, void *arg)
{
    felis_ctx_t *ctx = get_ctx();
    dict_t *dict;
    cJSON *js = cJSON_CreateArray();

    FOREACH_DICTS(ctx->dict_head, dict) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, "name", dict->name);
        cJSON_AddNumberToObject(item, "count", dict->count);
        cJSON_AddItemToArray(js, item);
    }
    char *json_encoded = cJSON_Print(js);
    send_normal_request(req, json_encoded);
}

static void http_dict_add(struct evhttp_request *req, void *arg)
{
    felis_ctx_t *ctx = get_ctx();
    smart_str body = {0};
    get_buffer_from_req(&body, req);
    if (body.len == 0) {
        send_params_errors(req);
        return;
    }
    cJSON *js, *name;
    dict_t *dict;
    js = cJSON_Parse(body.c);
    if (!js) {
        send_params_errors(req);
        smart_str_free(&body);
        return;
    }

    name = cJSON_GetObjectItem(js, "name");
    if (!name) {
        send_params_errors(req);
        goto final;
    }

    pthread_mutex_lock(&ctx->mutex);

    FOREACH_DICTS(ctx->dict_head, dict) {
        if (strcmp(dict->name, name->valuestring) == 0) {
            send_result_failure(req, "dict does exists!");
            pthread_mutex_unlock(&ctx->mutex);
            goto final;
        }
    }
    dict = dict_new(name->valuestring);

    if (ctx->dict_head == NULL) {
        ctx->dict_head = ctx->dict_tail = dict;
    } else {
        ctx->dict_tail->next = dict;
        ctx->dict_tail = dict;
    }

    pthread_mutex_unlock(&ctx->mutex);

    send_result_success(req);

final:
    cJSON_Delete(js);
    smart_str_free(&body);
}

static void http_dict_word_add(struct evhttp_request *req,
        void *arg, char *dict_name)
{
    felis_ctx_t *ctx = get_ctx();
    dict_t *dict = dict_find(ctx->dict_head, dict_name);
    if (NULL == dict) {
        send_not_found(req, "dict does not exists");
        return;
    }
    smart_str body = {0};
    get_buffer_from_req(&body, req);
    if (body.len == 0) {
        send_params_errors(req);
        return;
    }

    cJSON *js, *word, *ext;
    char *ext_str = NULL;
    js = cJSON_Parse(body.c);
    if (!js) {
        send_params_errors(req);
        smart_str_free(&body);
        return;
    }

    word = cJSON_GetObjectItem(js, "word");
    if (!word) {
        send_params_errors(req);
        goto final;
    }
    ext = cJSON_GetObjectItem(js, "ext");
    if (ext && ext->valuestring) {
        ext_str = strdup(ext->valuestring);
    }

    if (dict_word_add(dict, word->valuestring, ext_str) < 0) {
        send_result_failure(req, "failed to add word to dict");
    } else {
        send_result_success(req);
    }

final:
    cJSON_Delete(js);
    smart_str_free(&body);
}

static void http_match(struct evhttp_request *req,
        void *arg, char *dict_name)
{
    felis_ctx_t *ctx = get_ctx();
    dict_t *dict = dict_find(ctx->dict_head, dict_name);
    if (NULL == dict) {
        send_not_found(req, "dict does not exists");
        return;
    }
    smart_str body = {0};
    get_buffer_from_req(&body, req);
    if (body.len == 0) {
        send_params_errors(req);
        return;
    }

    cJSON *js = NULL, *content = NULL, *js_ret = NULL;
    js = cJSON_Parse(body.c);
    if (!js) {
        send_params_errors(req);
        smart_str_free(&body);
        goto final;
    }

    content = cJSON_GetObjectItem(js, "content");
    if (!content) {
        send_params_errors(req);
        goto final;
    }

    js_ret = cJSON_CreateArray();
    dict_match_json(dict, content->valuestring, js_ret);
    char *json_encoded = cJSON_Print(js_ret);
    send_normal_request(req, json_encoded);

final:
    if (js) cJSON_Delete(js);
    smart_str_free(&body);
    if (js_ret) cJSON_Delete(js_ret);
}

static void default_http_handler(struct evhttp_request *req, void *arg)
{
    char html[] = "<!DOCTYPE html>"
        "<html><head><title>Welcome to felis!</title></head>"
        "<body><center><h1>Welcome to felis!</h1></center><hr />"
        "<div align=\"center\">felis %s</div><Paste>"
        "</body></html>";
    const char *uri = evhttp_request_uri(req);
    if (strlen(uri) == 1 && *uri == '/') {
        struct evbuffer *buf = evbuffer_new();
        if(!buf) {
            log_error("failed to create response buffer");
            return;
        }

        evbuffer_add_printf(buf, html, VERSION);
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
        return;
    }

    if (strcasecmp(uri, "/dict") == 0 || strcasecmp(uri, "/dict/") == 0) {

        switch (evhttp_request_get_command(req)) {
            case EVHTTP_REQ_GET: // show dict list
                http_dict_list(req, arg);
                break;
            case EVHTTP_REQ_POST:
                http_dict_add(req, arg);
                break;
            default:
                send_bad_method(req, "Bad Method");
        }

    } else if (strncasecmp(uri, "/dict/", strlen("/dict/")) == 0) {
        if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
                send_bad_method(req, "Bad Method");
        } else {
            char *dict_name = ((char *)uri) + strlen("/dict/");
            http_dict_word_add(req, arg, dict_name);
        }
    } else if (strncasecmp(uri, "/match/", strlen("/match/")) == 0) {
        if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
                send_bad_method(req, "Bad Method");
        } else {
            char *dict_name = ((char *)uri) + strlen("/match/");
            http_match(req, arg, dict_name);
        }
    } else {
        send_not_found(req, "Not Found");
    }
}

static void child_signal_handler(int sig) {
     switch (sig) {
         case SIGTERM:
         case SIGHUP:
         case SIGQUIT:
         case SIGINT:
         case SIGSEGV:
         case SIGKILL:
         case SIGUSR1:
             event_loopbreak();
             pthread_exit(0);
             break;
     }
}


static void *dispatch(void *arg)
{
    signal(SIGUSR1, child_signal_handler);
    felis_thread_t *thread = (felis_thread_t *)arg;
    event_base_dispatch(thread->evbase);
    return NULL;
}

int server_start()
{
    int i = 0;
    felis_ctx_t *ctx = get_ctx();
    felis_config_t *cfg = ctx->cfg;
    ctx->listenfd = socket_listen(cfg->listen_host, cfg->listen_port);

    if (ctx->listenfd < 1) {
        return -1;
    }

    for (; i < cfg->threads; i++) {
        felis_thread_t *thread = &ctx->threads[i];
        thread->evbase = event_init();
        if (!thread->evbase) {
            log_error("Failed to initialize event");
            return -1;
        }

        thread->httpd = evhttp_new(thread->evbase);
        if (!thread->httpd) {
            log_error("Failed to allocate event http object");
            return -1;
        }

        if (evhttp_accept_socket(thread->httpd, ctx->listenfd) == -1) {
            log_error("evhttp_accept_socket failure");
            return -1;
        }

        evhttp_set_timeout(thread->httpd, cfg->timeout);

        // set route
        evhttp_set_gencb(thread->httpd, default_http_handler, NULL);

        if (pthread_create(&thread->thread, NULL, &dispatch, (void *)thread) != 0) {
            return -1;
        }
    }

    for (i = 0; i < cfg->threads; i++) {
        pthread_join(ctx->threads[i].thread, NULL);
    }
    log_notice("Server shutdown");
    return 0;
}

void server_shutdown()
{
    felis_ctx_t *ctx = get_ctx();
    int i;
    for (i = 0; i < ctx->cfg->threads; i++) {
        pthread_kill(ctx->threads[i].thread, SIGUSR1);
    }

    dict_t *dict;
    FOREACH_DICTS(ctx->dict_head, dict) {
        dict_release(dict);
    }
    close(ctx->listenfd);
}
