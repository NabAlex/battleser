#include "core.h"

static reader_callback reader_handler;
static response_callback main_handler;

#define MAX_EVENTS  65535
#define MAX_CLIENTS 65535
conn_t *connections;

static int cored = -1;

/* epoll descriptor */
static int32_t epoll_fd = -1;
static int32_t aio_fd = -1;

struct epoll_event *events;

static void debug_epoll_event(struct epoll_event ev);

static void
init_connections()
{
    connections = (conn_t*) xmalloc(MAX_CLIENTS * sizeof(conn_t));
    bzero(connections, MAX_CLIENTS * sizeof(conn_t));
    for (int32_t i = 0; i < MAX_CLIENTS; i++)
    {
        connections[i].id = CONNECT_UNUSED;
    }
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
bima_add_event(int _fd, uint32_t events)
{
    static struct epoll_event event = {};
    event.data.fd = _fd;
    event.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd, &event);
}

static int
bima_add_event_with_ptr(int _fd, uint32_t events, bima_epoll_type_t type, void *ptr)
{
    static struct epoll_event event = {};
    event.data.fd = _fd;
    event.events = events;

    bima_epoll_data_t *data = calloc(1, sizeof(*data));
    data->type = type;
    data->alloc_ptr = ptr;

    connections[_fd].epoll_ptr = data;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, _fd, &event);
}

int
bima_remove_event(int _fd)
{
    static struct epoll_event event;
    event.data.fd = 0;
    event.events = NULL;
    return epoll_ctl(epoll_fd, EPOLL_CTL_DEL, _fd, &event);
}

int
bima_change_event(int _fd, uint32_t events)
{
    static struct epoll_event event;
    event.data.fd = _fd;
    event.events = events;
    return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, _fd, &event);
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

void
bima_set_reader(reader_callback _reader) { reader_handler = _reader; }

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
        if (bima_add_event(infd, EPOLLIN | EPOLLET) == -1)
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

static int32_t
bima_add_aio_context(bima_aio_context_t *aio)
{
    return bima_add_event_with_ptr(aio->fd, EPOLLIN | EPOLLET, BIMA_AIO_CTX, aio);
}

int
bima_init(response_callback callback)
{
    int32_t r;

    init_connections();
    main_handler = callback;
    reader_handler = NULL;

    if((cored = bind_epoll("3000")) < 0)
        goto error;

    if(set_socket_non_block(cored) < 0)
        goto error;

    r = listen(cored, SOMAXCONN);
    assert(r != -1);

    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
    {
        log_e(MOD_BIMA": cannot create epoll");
        abort();
    }

    /* add me */
    bima_add_event(cored, EPOLLIN | EPOLLET);

    if (bima_aio_init(bima_add_aio_context) != RES_OK)
    {
        log_e("cannot init aio");
        goto error;
    }
    return RES_OK;
error:
    return RES_ERR;
}

void
bima_main_loop()
{
    xassert(cored >= 0, "none socket");

    int32_t count_clients;
    events = calloc(MAX_EVENTS, sizeof(struct epoll_event));

    bima_catch_exit();

    /* TODO (a.naberezhnyi) EPOLLONESHOT */
    log_d(MOD_BIMA": START SERVER!");
    while(1)
    {
        count_clients = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for(int event_id = 0; event_id < count_clients; event_id++)
        {
            debug_epoll_event(events[event_id]);

            int32_t res;
            conn_t *cn = get_connection(events[event_id].data.fd);

            if (events[event_id].events & EPOLLERR ||
                cn->id == CONNECT_DONE)
            {
                /*************************/
                /* this events is assert */
                /*************************/
                log_e("epoll fail!");
                bima_connection_close(cn);
                continue;
            }

            if (events[event_id].events & ~(EPOLLHUP | EPOLLOUT | EPOLLIN))
            {
                log_e("strange event!");
                bima_connection_close(cn);
                continue;
            }

            if (cn->epoll_ptr != NULL)
            {
                bima_epoll_data_t *data = cn->epoll_ptr;
                if (data->type == BIMA_AIO_CTX)
                {
                    bima_aio_context_t *aio = data->alloc_ptr;

                    struct io_event *aio_events = NULL;
                    int32_t r = bima_aio_get_events(aio, &aio_events);
                    if (r > 0 && aio_events != NULL)
                    {
                        /* TODO change active */
                        aio->active = false;

                        for (int32_t i = 0; i < r; i++)
                            bima_connection_close((conn_t *) aio_events[i].data);
                    }
                    else
                    {
                        log_e("wrong EPOLLIN with aio!");
                    }
                }
                continue;
            }

            /* main part */
            if (events[event_id].events & EPOLLHUP)
            {
                /**************/
                /* disconnect */
                /**************/
                log_d("disconnect %d", cn->fd);
                if (reader_handler && cn->add_buf && cn->add_buf_len)
                {
                    res = reader_handler(cn, cn->add_buf, cn->add_buf_len);
                    if (res == RES_OK)
                    {
                        cn->id = CONNECT_UNUSED;
                        /* TODO (a.naberezhnyi) check this! */
                        xassert(main_handler(cn) == RES_OK,
                            "wrong main_handler if user close socket");
                    }
                }
            }
            else
            if (cored == events[event_id].data.fd)
            {
                if (bima_connection_accept(&events[event_id]) != RES_OK)
                    log_e(MOD_BIMA": cannot accept connection");
            }
            else
            if (events[event_id].events & EPOLLIN)
            {
                ssize_t r_len = 0;

                char buf[512];
                cn->out = buf;
                cn->out_len = 0;

                if (cn->add_buf && cn->add_buf_len)
                {
                    xmemmove(cn->out, cn->add_buf, cn->add_buf_len);
                    cn->out_len = cn->add_buf_len;
                }

                while (true)
                {
                    if ((r_len = read(cn->fd, buf + cn->out_len, sizeof(buf) - cn->out_len)) < 0)
                    {
                        /* TODO check read full */
                        if (errno == EWOULDBLOCK)
                            break;

                        if (errno == EAGAIN)
                            continue;

                        /* strange error */
                        bima_connection_close(cn);
                        break;
                    }

                    if (r_len == 0)
                    {
                        bima_connection_close(cn);
                        continue;
                    }

                    cn->out_len += r_len;
                }

                if (cn->out_len <= 0)
                    continue;

                if (reader_handler)
                {
                    res = reader_handler(cn, cn->out, cn->out_len);
                    if (res == RES_AGAIN)
                    {
                        if (!cn->add_buf)
                            cn->add_buf = malloc(cn->out_len);
                        else
                            cn->add_buf = realloc(cn->add_buf, cn->add_buf_len + cn->out_len);

                        xmemmove(cn->add_buf + cn->add_buf_len, cn->out, cn->out_len);
                        cn->add_buf_len += cn->out_len;
                        continue;
                    }

                    if (res != RES_OK)
                    {
                        bima_connection_close(cn);
                        continue;
                    }

                    /* TODO normal buf :( */
                    free(cn->add_buf);

                    cn->out = xmemdup(cn->out, cn->out_len);
                }

                /* TODO (a.naberezhnyi) check http */

                /* kill me! ab -n 10000 -c 10 -r http://127.0.0.1:3000/ */
                /* TODO (a.naberezhnyi) too slow new object */

                /* TODO need event_id use epollet? */
                bima_change_event(cn->fd, EPOLLOUT | EPOLLET);
            }
            else
            if (events[event_id].events & EPOLLOUT)
            {
                res = main_handler(cn);
                if (res == RES_OK)
                    log_d("RES_OK");
                else
                    log_d("RES_ERR");
            }
        }
    }
}

/* please, dont check this method */
int32_t
bima_write(conn_t *cn, char *buf, size_t buf_len)
{
    if (!cn || cn->id < 0)
        return RES_FAIL;

    if (write(cn->fd, buf, buf_len) < 0)
        return RES_ERR;

    return RES_OK;
}

int32_t
bima_write_and_close(conn_t *cn, char *buf, size_t buf_len)
{
    int32_t r;
    if ((r = bima_write(cn, buf, buf_len)) == RES_OK)
        bima_connection_close(cn);

    return r;
}

int32_t
bima_write_descriptor(conn_t *cn, int32_t dscr, size_t dscr_size)
{
    if (!cn || cn->id < 0)
        return RES_FAIL;

    off_t offset = 0;
    while (offset < dscr_size)
    {
        if (sendfile(cn->fd, dscr, &offset, dscr_size - offset) < 0)
        {
            log_e("sendfile error to %d %s", dscr, strerror(errno));
            return RES_OK;
        }
    }

    bima_connection_close(cn);
    return RES_OK;
}

void
bima_connection_close(conn_t *cn)
{
    log_d(MOD_BIMA": Closed connection on descriptor %d",
        cn->fd);

    bima_remove_event(cn->fd);

    close(cn->fd);
    cn->id = CONNECT_DONE;

    cn->out = NULL;
    cn->out_len = 0;


    free(cn->out);
    cn->out = NULL;
    cn->out_len = 0;

    free(cn->add_buf);
    cn->add_buf = NULL;
    cn->add_buf_len = 0;

    // shutdown(cn->fd, SHUT_RDWR);
}

static void debug_epoll_event(struct epoll_event ev)
{
#ifdef DEBUG
    printf("fd(%d), ev.events:", ev.data.fd);

    if(ev.events & EPOLLIN)
        printf(" EPOLLIN ");
    if(ev.events & EPOLLOUT)
        printf(" EPOLLOUT ");
    if(ev.events & EPOLLET)
        printf(" EPOLLET ");
    if(ev.events & EPOLLPRI)
        printf(" EPOLLPRI ");
    if(ev.events & EPOLLRDNORM)
        printf(" EPOLLRDNORM ");
    if(ev.events & EPOLLRDBAND)
        printf(" EPOLLRDBAND ");
    if(ev.events & EPOLLWRNORM)
        printf(" EPOLLRDNORM ");
    if(ev.events & EPOLLWRBAND)
        printf(" EPOLLWRBAND ");
    if(ev.events & EPOLLMSG)
        printf(" EPOLLMSG ");
    if(ev.events & EPOLLERR)
        printf(" EPOLLERR ");
    if(ev.events & EPOLLHUP)
        printf(" EPOLLHUP ");
    if(ev.events & EPOLLONESHOT)
        printf(" EPOLLONESHOT ");

    printf("\n");
#endif
}