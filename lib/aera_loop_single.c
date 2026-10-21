/* Android request-loop adapter for libfuse (LGPL-2.0-or-later).
 * FUSE channel references: Copyright (C) 2001-2007 Miklos Szeredi.
 * Cooperative workers avoid POSIX thread cancellation, which bionic lacks.
 */
#include "fuse_i.h"
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

struct fuse_chan *fuse_chan_get(struct fuse_chan *ch) {
    assert(ch->ctr>0);
    pthread_mutex_lock(&ch->lock);++ch->ctr;pthread_mutex_unlock(&ch->lock);
    return ch;
}
void fuse_chan_put(struct fuse_chan *ch) {
    if (!ch) return;
    pthread_mutex_lock(&ch->lock);
    if (!--ch->ctr) {
        pthread_mutex_unlock(&ch->lock);close(ch->fd);
        pthread_mutex_destroy(&ch->lock);free(ch);
    } else pthread_mutex_unlock(&ch->lock);
}

int fuse_start_thread(pthread_t *thread,void *(*function)(void *),void *data) {
    return pthread_create(thread,NULL,function,data);
}
struct aera_loop {
    struct fuse_session *session;
    pthread_mutex_t receive;
    int error;
};
static void *aera_worker(void *opaque) {
    struct aera_loop *loop=opaque;
    struct fuse_buf buffer={.mem=NULL};
    while (!fuse_session_exited(loop->session)) {
        /* One reader per channel; filesystem callbacks may run concurrently. */
        pthread_mutex_lock(&loop->receive);
        int rc=0;
        if (!fuse_session_exited(loop->session)) {
            struct pollfd fd={.fd=fuse_session_fd(loop->session),.events=POLLIN};
            rc=poll(&fd,1,100);
            if (rc>0) rc=fuse_session_receive_buf(loop->session,&buffer);
            else if (rc<0) rc=-errno;
            else rc=-EAGAIN;
            if (rc<=0 && rc!=-EINTR && rc!=-EAGAIN) {
                if (rc<0 && !loop->error) loop->error=rc;
                fuse_session_exit(loop->session);
            }
        }
        pthread_mutex_unlock(&loop->receive);
        if (rc>0) fuse_session_process_buf(loop->session,&buffer);
    }
    fuse_buf_free(&buffer);
    return NULL;
}
int fuse_session_loop_mt_312(struct fuse_session *session,struct fuse_loop_config *config) {
    if (config && fuse_loop_cfg_verify(config)) return -EINVAL;
    unsigned count=config ? config->max_threads : 8;
    if (!count || count>8) count=8;
    int fd=fuse_session_fd(session),flags=fcntl(fd,F_GETFL);
    if (flags<0 || fcntl(fd,F_SETFL,flags|O_NONBLOCK)) return -errno;
    struct aera_loop loop={.session=session,.receive=PTHREAD_MUTEX_INITIALIZER};
    pthread_t threads[8];
    unsigned started=0;
    while (started<count) {
        int error=pthread_create(&threads[started],NULL,aera_worker,&loop);
        if (error) {
            pthread_mutex_lock(&loop.receive);loop.error=-error;
            fuse_session_exit(session);pthread_mutex_unlock(&loop.receive);break;
        }
        ++started;
    }
    for (unsigned i=0;i<started;++i) pthread_join(threads[i],NULL);
    pthread_mutex_destroy(&loop.receive);
    fcntl(fd,F_SETFL,flags);
    return loop.error ? loop.error : session->error;
}
int fuse_session_loop_mt_32(struct fuse_session *session,struct fuse_loop_config_v1 *config) {
    struct fuse_loop_config *current=fuse_loop_cfg_create();
    if (!current) return -ENOMEM;
    current->max_threads=8;
    if (config) fuse_loop_cfg_convert(current,config);
    int rc=fuse_session_loop_mt_312(session,current);
    fuse_loop_cfg_destroy(current);return rc;
}
int fuse_session_loop_mt_31(struct fuse_session *session,int clone_fd) {
    (void)clone_fd;return fuse_session_loop_mt_312(session,NULL);
}
struct fuse_loop_config *fuse_loop_cfg_create(void) {
    struct fuse_loop_config *c=calloc(1,sizeof(*c));
    if (c) {c->version_id=INT_MAX-2;c->max_idle_threads=UINT_MAX;c->max_threads=1;}
    return c;
}
void fuse_loop_cfg_destroy(struct fuse_loop_config *c) {free(c);}
int fuse_loop_cfg_verify(struct fuse_loop_config *c) {return c && c->version_id==INT_MAX-2 ? 0 : -EINVAL;}
void fuse_loop_cfg_convert(struct fuse_loop_config *c,struct fuse_loop_config_v1 *v) {
    c->clone_fd=v->clone_fd;c->max_idle_threads=v->max_idle_threads;
}
void fuse_loop_cfg_set_idle_threads(struct fuse_loop_config *c,unsigned value) {c->max_idle_threads=value;}
void fuse_loop_cfg_set_max_threads(struct fuse_loop_config *c,unsigned value) {c->max_threads=value;}
void fuse_loop_cfg_set_clone_fd(struct fuse_loop_config *c,unsigned value) {c->clone_fd=value;}
