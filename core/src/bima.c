#include "core.h"

#include <linux/aio_abi.h>

static response_callback main_callback;

#define MAX_EVENTS  65536
#define MAX_CLIENTS 65536
conn_t *connections;

static int cored = -1;

/* epoll descriptor */
static int32_t epoll_fd = -1;

struct epoll_event *events;

static void
init_connections()
{
    connections = (conn_t*) xmalloc(MAX_CLIENTS * sizeof(conn_t));
    bzero(connections, MAX_CLIENTS * sizeof(conn_t));
    for (int32_t i = 0; i < MAX_CLIENTS; i++)
        connections[i].id = -1;
}

static inline conn_t *
get_connection(int index)
{
    return &connections[index];
}

static void
bima_release_connection(void)
{
    for (int32_t id = 0; id < MAX_CLIENTS; id++)
    {
        if (connections[id].id == id)
        {
            log_d("close connection %d", id);
            close(id);
        }
    }

    log_w(MOD_BIMA" release connections and epoll");
    free(events);
    free(connections);
    /* normal close socket !!! */
    close(cored);
}

static void
sigterm_handler(int signal, siginfo_t *info, void *_unused)
{
    exit(0);
}

static void
bima_catch_exit()
{
    atexit(bima_release_connection);
    struct sigaction action = {
        .sa_handler = NULL,
        .sa_sigaction = sigterm_handler,
        .sa_mask = 0,
        .sa_flags = SA_SIGINFO,
        .sa_restorer = NULL
    };

    sigaction(SIGTERM, &action, NULL);
}

static int
bind_epoll(char *port)
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s, sfd = -1;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    s = getaddrinfo(NULL, port, &hints, &result);
    if (s != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }
    
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        s = bind(sfd, rp->ai_addr, rp->ai_addrlen);
        if (s == 0)
        {
            /* We managed to bind successfully! */
            break;
        }

        log_e("%s", strerror(errno));
        close (sfd);
    }
    
    if (rp == NULL)
    {
        log_e("%s", MOD_BIMA": could not bind");
        return -1;
    }
    
    freeaddrinfo(result);
    return sfd;
}

static int
add_fd_vision(int epolld, int _fd)
{
    static struct epoll_event event;
    event.data.fd = _fd;
    event.events = EPOLLIN | EPOLLET;
    return epoll_ctl(epolld, EPOLL_CTL_ADD, _fd, &event);
}

static int
set_socket_non_block(int sfd)
{
    int flags, s;
    
    flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1)
    {
        log_e("fcntl");
        return -1;
    }

    s = fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
    if (s == -1)
    {
        log_e("fcntl");
        return -1;
    }
    
    return 0;
}

int
bima_init(response_callback callback)
{
    init_connections();

    main_callback = callback;
    
    if((cored = bind_epoll("3000")) < 0)
        goto error;
    
    if(set_socket_non_block(cored) < 0)
        goto error;
    
    return RES_OK;
error:
    return RES_ERR;
}

static int32_t
bima_connection_accept(struct epoll_event *ev)
{
    assert(ev);

    struct sockaddr in_addr;
    socklen_t in_len;
    int infd, tmp;

    while (true)
    {
        in_len = sizeof(in_addr);
        infd = accept(cored, &in_addr, &in_len);
        if (infd == -1)
        {
            if ((errno == EAGAIN) ||
                (errno == EWOULDBLOCK))
            {
                /* read all connections */
                break;
            }
            else
            {
                log_e(MOD_BIMA
                    ": cannot accept connection!");
                return RES_ERR;
            }
        }

#ifdef DEBUG
        static char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

        tmp = getnameinfo(&in_addr, in_len, hbuf, sizeof(hbuf),
            sbuf, sizeof(sbuf),
            NI_NUMERICHOST | NI_NUMERICSERV);
        if (tmp == 0)
        {
            log_d(MOD_BIMA
                ": Accepted connection on descriptor %d "
                    "(host=%s, port=%s)", infd, hbuf, sbuf);
        }
#endif

        tmp = set_socket_non_block(infd);
        if (tmp == -1)
        {
            log_e(MOD_BIMA
                ": cannot create non block");
            return RES_ERR;
        }


        /* edge-triggered mode */
        if (add_fd_vision(epoll_fd, infd) == -1)
        {
            log_e(MOD_BIMA
                ": cannot EPOLL_CTL_ADD");
            return RES_ERR;
        }

        conn_t *cn = get_connection(infd);

        static int32_t unique_id = -1;
        cn->id = ++unique_id;
        cn->fd = infd;
        cn->client_info.uaddr = in_addr;
    }

    return RES_OK;
}

void
bima_main_loop()
{
    xassert(cored >= 0, "none socket");

    int32_t res = RES_OK, tmp, count_clients;
    
    tmp = listen(cored, SOMAXCONN);
    assert(tmp != -1);
    
    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        log_e(MOD_BIMA": cannot create epoll");
        abort();
    }
    
    events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

    /* add me */
    xassert(add_fd_vision(epoll_fd, cored) == 0, MOD_BIMA": cannot add element to epoll");

    log_d(MOD_BIMA": START SERVER!");

    bima_catch_exit();

    /* TODO (a.naberezhnyi) EPOLLONESHOT */
    while(1)
    {
        count_clients = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for(int i = 0; i < count_clients; i++)
        {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP /* TODO disconnect */ ))
            {
                log_e(MOD_BIMA": EPOLLERR");
                close(events->data.fd);
                continue;
            }
            else
            if(cored == events[i].data.fd)
            {
                if (bima_connection_accept(&events[i]) == RES_ERR)
                    log_e(MOD_BIMA": cannot accept connection");
            }
            else
            if (events[i].events & EPOLLIN)
            {
                char buf[512];
                ssize_t r_len = 0;

                conn_t *cn = get_connection(events[i].data.fd);
                cn->out = buf;
                cn->out_len = 0;

                while (true)
                {
                    if ((r_len = read(cn->fd, buf, sizeof(buf))) < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            log_d("normal exit read");
                            break;
                        }

                        log_e("packet loss!");
                        assert(0);
                        break;
                    }

                    cn->out_len += r_len;
                }

                if (cn->out_len <= 0)
                    continue;

                // log_d("count %ld -> %s\n", cn->out_len, buf);

                /* TODO (a.naberezhnyi) check http */

                /* kill me! ab -n 10000 -c 10 -r http://127.0.0.1:3000/ */
                /* TODO (a.naberezhnyi) too slow new object */
                static struct epoll_event event;
                event.data.fd = cn->fd;
                event.events = EPOLLOUT; // TODO (a.naberezhnyi) wtf? | EPOLLET;
                epoll_ctl(epoll_fd, EPOLL_CTL_MOD, cn->fd, &event);
            }
            else
            if (events[i].events & EPOLLOUT)
            {
                conn_t *cn = get_connection(events[i].data.fd);

                res = main_callback(cn);
                if (res == RES_OK)
                    log_d("RES_OK");
                else
                {
                    log_d("RES_ERR");
                }

                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cn->fd, NULL);
                bima_connection_close(cn);
            }
        }
    }
}

int32_t
bima_write(conn_t *cn, char *buf, size_t buf_len)
{
    if (!cn)
        return RES_ERR;

    if (write(cn->fd, buf, buf_len) < 0)
        return RES_ERR;

    return RES_OK;
}

void
bima_connection_close(conn_t *cn)
{
    log_d(MOD_BIMA": Closed connection on descriptor %d",
        cn->fd);

    close(cn->fd);
    // shutdown(cn->fd, SHUT_RDWR);
}