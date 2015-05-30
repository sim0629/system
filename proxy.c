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

/*
 * Function prototypes
 */
static void print_log_entry(struct sockaddr_in *sockaddr, char *uri, int size);

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
