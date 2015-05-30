/*
 * proxy.c - CS:APP Web proxy
 *
 * Student Information:
 * 심규민, 2009-11744
 *
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */

#include "csapp.h"

#define CHUNKED -1

struct conn_info
{
    int fd;
    rio_t rio;
    struct sockaddr_in addr;
    socklen_t addr_len;
};

struct client_info
{
    int fd;
    rio_t rio;
};

struct http_header
{
    char first_line[MAXLINE];
    char buf[MAXBUF];
    int buf_length;
    int content_length;

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

static int proxy_accept(int listen_fd, struct conn_info *conn_info_ptr)
{
    conn_info_ptr->addr_len = sizeof(conn_info_ptr->addr);
    conn_info_ptr->fd = accept(listen_fd, (SA *)&conn_info_ptr->addr, &conn_info_ptr->addr_len);
    if (conn_info_ptr->fd < 0) {
        fprintf(stderr, "Fail to accept\n");
        return -1;
    }
    rio_readinitb(&conn_info_ptr->rio, conn_info_ptr->fd);
    return 0;
}

static int proxy_parse_http_header(rio_t *rio_ptr, struct http_header *header_ptr)
{
    char line[MAXLINE];
    int line_len;

    if ((line_len = rio_readlineb(rio_ptr, header_ptr->first_line, MAXLINE)) <= 0)
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

static int proxy_parse_request_header(rio_t *rio_ptr, struct http_header *header_ptr)
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

static int proxy_parse_response_header(rio_t *rio_ptr, struct http_header *header_ptr)
{
    return proxy_parse_http_header(rio_ptr, header_ptr);
}

static int proxy_connect(struct client_info *client_info_ptr, struct http_header *header_ptr)
{
    int port = 80;

    if (header_ptr->port != NULL)
        port = atoi(header_ptr->port);

    if ((client_info_ptr->fd = open_clientfd(header_ptr->host, port)) < 0) {
        fprintf(stderr, "Fail to connect\n");
        return -1;
    }
    rio_readinitb(&client_info_ptr->rio, client_info_ptr->fd);
    return 0;
}

static void proxy_relay(struct conn_info *conn_info_ptr)
{
    struct http_header request_header;
    struct client_info client_info;

    if (proxy_parse_request_header(&conn_info_ptr->rio, &request_header) < 0) {
        close(conn_info_ptr->fd);
        return;
    }

    if (proxy_connect(&client_info, &request_header) < 0) {
        close(conn_info_ptr->fd);
        return;
    }
    // TODO
}

static void proxy_listen_forever(int port)
{
    int listen_fd;

    if ((listen_fd = open_listenfd(port)) < 0) {
        fprintf(stderr, "Can not listen on port %d\n", port);
        exit(1);
    }

    while (1) {
        struct conn_info conn_info;
        if (proxy_accept(listen_fd, &conn_info) < 0)
            continue;
        proxy_relay(&conn_info);
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

    /* Print the formatted log entry string */
    fp = fopen(filename, "a");
    if (fp == NULL) {
        fprintf(stderr, "Can not open file %s\n", filename);
        return;
    }
    fprintf(fp, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
    fclose(fp);
}
