/* Android single-request-loop adapter for libfuse (LGPL-2.0-or-later).
 * FUSE channel references: Copyright (C) 2001-2007 Miklos Szeredi.
 * The native NAS helper serializes access to its protocol sessions and
 * does not use POSIX thread cancellation, which bionic does not provide.
 */
#include "fuse_i.h"
#include <limits.h>
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
int fuse_session_loop_mt_312(struct fuse_session *session,struct fuse_loop_config *config) {
    (void)session;(void)config;return -ENOTSUP;
}
int fuse_session_loop_mt_32(struct fuse_session *session,struct fuse_loop_config_v1 *config) {
    (void)session;(void)config;return -ENOTSUP;
}
int fuse_session_loop_mt_31(struct fuse_session *session,int clone_fd) {
    (void)session;(void)clone_fd;return -ENOTSUP;
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
