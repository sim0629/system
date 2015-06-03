/*
 * proxy.c - CS:APP Web proxy
 *
 * Student Information:
 * 심규민, 2009-11744
 *
 *  1. 주어진 command line argument로부터 port 번호를 파싱하고 listen 합니다.
 *  2. 브라우져를 accept 하여 그 정보를 conn_info에 저장합니다.
 *     별도의 thread를 만들어 3.-11.을 concurrent 하게 실행합니다.
 *     2.를 반복합니다.
 *  3. 브라우져의 요청을 빈 줄이 나올 때까지 한 줄씩 읽으며 파싱하여
 *     header_info에 저장합니다.
 *  4. 요청 맨 첫 줄을 파싱하여 웹서버의 주소를 알아냅니다.
 *  5. 웹서버에 connect 하여 그 정보를 client_info에 저장합니다.
 *  6. 요청 header를 웹서버에 전달합니다.
 *  7. 요청 header에서 chunked encoding인지 아닌지에 따라 content length만큼
 *     body를 읽어서 웹서버에 전달합니다.
 *  8. 웹서버의 응답을 빈 줄이 나올 때까지 한 줄씩 읽으며 파싱하여
 *     header_info에 저장합니다.
 *  9. 응답 header를 브라우져에 전달합니다.
 * 10. 응답 header에서 chunked encoding인지 아닌지에 따라 content length만큼
 *     body를 읽어서 브라우져에 전달합니다.
 * 11. 양쪽의 연결을 끊습니다.
 */

#include "csapp.h"

#define CHUNKED -1 // chunked encoding일 때 http_header의 content_length 값
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* 브라우져와 연결된 socket 정보 */
struct conn_info
{
    int fd;
    rio_t rio;
    struct sockaddr_in addr;
    socklen_t addr_len;
};

/* 웹서버와 연결된 socket 정보 */
struct client_info
{
    int fd;
    rio_t rio;
};

/*
 * http header 정보
 * request와 response를 겸합니다.
 */
struct http_header
{
    char first_line[MAXLINE];
    char buf[MAXBUF];
    int buf_length;
    int content_length;

    /* 아래는 request에서만 쓰입니다. */
    char *method;
    char *host;
    char *port;
    char *path;
    char *version;
};

/*
 * Function prototypes
 */
static void print_log_entry(struct sockaddr_in *sockaddr, char *uri, int size);
static int open_clientfd_mt(char *hostname, int port);

/*
 * listen_fd에서 socket을 accept 하여 conn_info를 채웁니다.
 * 성공할 경우 0을, 실패할 경우 -1을 리턴합니다.
 */
static int proxy_accept(int listen_fd, struct conn_info *conn_info_ptr)
{
    conn_info_ptr->addr_len = sizeof(conn_info_ptr->addr);
    conn_info_ptr->fd = accept(listen_fd, (SA *)&conn_info_ptr->addr,
                               &conn_info_ptr->addr_len);
    if (conn_info_ptr->fd < 0) {
        fprintf(stderr, "Fail to accept\n");
        return -1;
    }
    rio_readinitb(&conn_info_ptr->rio, conn_info_ptr->fd);
    return 0;
}

/*
 * rio buffer에서 http header를 읽고 파싱하여 채웁니다.
 * 성공할 경우 0을, 실패할 경우 -1을 리턴합니다.
 */
static int proxy_parse_http_header(rio_t *rio_ptr,
                                   struct http_header *header_ptr)
{
    char line[MAXLINE];
    int line_len;

    if ((line_len = rio_readlineb(rio_ptr, header_ptr->first_line, MAXLINE))
        <= 0)
        return -1;
    header_ptr->first_line[line_len] = '\0';

    header_ptr->buf_length = 0;
    header_ptr->content_length = 0;

    while ((line_len = rio_readlineb(rio_ptr, line, MAXLINE)) > 0) {
        memcpy(header_ptr->buf + header_ptr->buf_length, line, line_len);
        header_ptr->buf_length += line_len;

        if (header_ptr->content_length == 0) {
            const char *te = "Transfer-Encoding: ";
            const char *cl = "Content-Length: ";
            if (strncasecmp(line, te, strlen(te)) == 0) {
                if (strncmp(line + strlen(te), "indentity", 9) != 0)
                    header_ptr->content_length = CHUNKED;
            }
            else if (strncasecmp(line, cl, strlen(cl)) == 0) {
                header_ptr->content_length = atoi(line + strlen(cl));
            }
        }

        if (strncmp(line, "\r\n", 2) == 0)
            return 0;
    }

    return -1;
}

/*
 * rio buffer에서 http request header를 읽고 파싱하여 채웁니다.
 * 공통 header를 파싱하고, 맨 첫 줄을 파싱합니다.
 * 성공할 경우 0을, 실패할 경우 -1을 리턴합니다.
 */
static int proxy_parse_request_header(rio_t *rio_ptr,
                                      struct http_header *header_ptr)
{
    if (proxy_parse_http_header(rio_ptr, header_ptr) < 0)
        return -1;

    header_ptr->method = header_ptr->first_line;
    header_ptr->host = strchr(header_ptr->method, ' ');
    if (header_ptr->host == NULL)
        return -1;
    header_ptr->host[0] = '\0';
    header_ptr->host++;
    if (strncmp(header_ptr->host, "http://", 7) != 0)
        return -1;
    header_ptr->host += 7;
    header_ptr->port = strpbrk(header_ptr->host, ":/");
    if (header_ptr->port == NULL)
        return -1;
    if (header_ptr->port[0] == ':') {
        header_ptr->port[0] = '\0';
        header_ptr->port++;
        header_ptr->path = strchr(header_ptr->port, '/');
        if (header_ptr->path == NULL)
            return -1;
    }
    else {
        header_ptr->path = header_ptr->port;
        header_ptr->port = NULL;
    }
    header_ptr->path[0] = '\0';
    header_ptr->path++;
    header_ptr->version = strchr(header_ptr->path, ' ');
    if (header_ptr->version == NULL)
        return -1;
    header_ptr->version[0] = '\0';
    header_ptr->version++;

    return 0;
}

/*
 * rio buffer에서 http response header를 읽고 파싱하여 채웁니다.
 * 공통 header를 파싱하고, 특별한 작업을 더 하지 않습니다.
 * 성공할 경우 0을, 실패할 경우 -1을 리턴합니다.
 */
static int proxy_parse_response_header(rio_t *rio_ptr,
                                       struct http_header *header_ptr)
{
    return proxy_parse_http_header(rio_ptr, header_ptr);
}

/*
 * request header 정보를 토대로 웹서버에 connect 하고 client_info를 채웁니다.
 * 성공할 경우 0을, 실패할 경우 -1을 리턴합니다.
 */
static int proxy_connect(struct client_info *client_info_ptr,
                         struct http_header *header_ptr)
{
    int port = 80;

    if (header_ptr->port != NULL)
        port = atoi(header_ptr->port);

    if ((client_info_ptr->fd = open_clientfd_mt(header_ptr->host, port)) < 0) {
        fprintf(stderr, "Fail to connect\n");
        return -1;
    }
    rio_readinitb(&client_info_ptr->rio, client_info_ptr->fd);
    return 0;
}

/*
 * rio buffer에서 size만큼 읽어서 to socket에 씁니다.
 * 성공할 경우 0을, 실패할 경우 -1을 리턴합니다.
 */
static int proxy_toss(rio_t *from_rio_ptr, int to_fd, int size)
{
    char line[MAXLINE];
    int line_len;

    while (size > 0) {
        line_len = rio_readnb(from_rio_ptr, line, MIN(MAXLINE, size));
        if (line_len <= 0)
            return -1;
        if (rio_writen(to_fd, line, line_len) <= 0)
            return -1;
        size -= line_len;
    }

    return 0;
}

/*
 * 브라우져의 요청을 받아 웹서버에 전달하고
 * 웹서버의 응답을 받아 브라우져에 전달합니다.
 * 하나의 요청-응답만을 처리하고 log를 남긴 후에 양쪽의 연결의 모두 끊습니다.
 */
static void proxy_relay(struct conn_info *conn_info_ptr)
{
    struct client_info client_info;

    struct http_header request_header;
    struct http_header response_header;
    char line[MAXLINE];
    int line_len;
    int recv_size = 0;

    // 요청 header 파싱
    if (proxy_parse_request_header(&conn_info_ptr->rio, &request_header) < 0) {
        close(conn_info_ptr->fd);
        return;
    }

    // 웹서버에 연결
    if (proxy_connect(&client_info, &request_header) < 0) {
        close(conn_info_ptr->fd);
        return;
    }

    // 웹서버에 header 전달
    line_len = sprintf(line, "%s /%s %s", request_header.method,
                       request_header.path, request_header.version);
    if (rio_writen(client_info.fd, line, line_len) <= 0)
        goto proxy_relay_end;
    if (rio_writen(client_info.fd, request_header.buf,
                   request_header.buf_length) <= 0)
        goto proxy_relay_end;

    // 요청 body를 읽어서 웹서버에 전달
    if (request_header.content_length == CHUNKED) {
        while (1) {
            line_len = rio_readlineb(&conn_info_ptr->rio, line, MAXLINE);
            if (line_len <= 0)
                goto proxy_relay_end;
            if (rio_writen(client_info.fd, line, line_len) <= 0)
                goto proxy_relay_end;
            line_len = (int)strtol(line, NULL, 16) + 2;
            if (proxy_toss(&conn_info_ptr->rio, client_info.fd, line_len) < 0)
                goto proxy_relay_end;
            if (line_len == 2)
                break;
        }
    }else if (request_header.content_length > 0) {
        if (proxy_toss(&conn_info_ptr->rio, client_info.fd,
                       request_header.content_length) < 0)
            goto proxy_relay_end;
    }

    // 응답 header 파싱
    if (proxy_parse_response_header(&client_info.rio, &response_header) < 0)
        goto proxy_relay_end;

    // 브라우져에 header 전달
    line_len = strlen(response_header.first_line);
    recv_size += line_len;
    if (rio_writen(conn_info_ptr->fd, response_header.first_line, line_len)
        <= 0)
        goto proxy_relay_end;
    recv_size += response_header.buf_length;
    if (rio_writen(conn_info_ptr->fd, response_header.buf,
                   response_header.buf_length) <= 0)
        goto proxy_relay_end;

    // 응답 body를 읽어서 브라우져에 전달
    if (response_header.content_length == CHUNKED) {
        while (1) {
            line_len = rio_readlineb(&client_info.rio, line, MAXLINE);
            if (line_len <= 0)
                goto proxy_relay_end;
            recv_size += line_len;
            if (rio_writen(conn_info_ptr->fd, line, line_len) <= 0)
                goto proxy_relay_end;
            line_len = (int)strtol(line, NULL, 16) + 2;
            recv_size += line_len;
            if (proxy_toss(&client_info.rio, conn_info_ptr->fd, line_len) < 0)
                goto proxy_relay_end;
            if (line_len == 2)
                break;
        }
    }else if (response_header.content_length > 0) {
        line_len += request_header.content_length;
        if (proxy_toss(&client_info.rio, conn_info_ptr->fd,
                       response_header.content_length) < 0)
            goto proxy_relay_end;
    }

    // 로그 출력
    sprintf(line, "http://%s/%s", request_header.host, request_header.path);
    print_log_entry(&conn_info_ptr->addr, line, recv_size);

proxy_relay_end:
    // 연결 끊기
    close(client_info.fd);
    close(conn_info_ptr->fd);
}

/*
 * proxy_relay를 concurrent 하게 부르기 위한 thread function 입니다.
 */
static void *proxy_relay_thread(void *param)
{
    struct conn_info *conn_info_ptr = (struct conn_info *)param;
    proxy_relay(conn_info_ptr);
    free(conn_info_ptr);
    return NULL;
}

/*
 * 주어진 port에서 무한히 listen 하며 accept, relay 합니다.
 * accept는 synchronous 하게 하고 relay는 concurrent 하게 합니다.
 * listen에 실패하면 프로세스를 종료합니다.
 */
static void proxy_listen_forever(int port)
{
    int listen_fd;

    if ((listen_fd = open_listenfd(port)) < 0) {
        fprintf(stderr, "Can not listen on port %d\n", port);
        exit(1);
    }

    while (1) {
        pthread_t thread;
        struct conn_info *conn_info_ptr = malloc(sizeof(struct conn_info));
        if (proxy_accept(listen_fd, conn_info_ptr) < 0)
            continue;
        pthread_create(&thread, NULL, proxy_relay_thread, conn_info_ptr);
        if (thread < 0) {
            close(conn_info_ptr->fd);
            free(conn_info_ptr);
            continue;
        }
        pthread_detach(thread);
    }
}

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    // pipe signal 무시
    signal(SIGPIPE, SIG_IGN);

    proxy_listen_forever(atoi(argv[1]));
    exit(0);
}

/*
 * print_log_entry - Print a formatted log entry in log file.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
static void print_log_entry(struct sockaddr_in *sockaddr, char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    const char *filename = "proxy.log";
    FILE *fp;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&print_mutex);
    /* Print the formatted log entry string */
    fp = fopen(filename, "a");
    if (fp == NULL) {
        fprintf(stderr, "Can not open file %s\n", filename);
        goto print_log_entry_done;
    }
    fprintf(fp, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
    fclose(fp);
print_log_entry_done:
    pthread_mutex_unlock(&print_mutex);
}

/*
 * open_clientfd에 mutual exclusion을 적용한 wrapper 입니다.
 */
int open_clientfd_mt(char *hostname, int port)
{
    int clientfd;
    static pthread_mutex_t clientfd_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&clientfd_mutex);
    clientfd = open_clientfd(hostname, port);
    pthread_mutex_unlock(&clientfd_mutex);
    return clientfd;
}
