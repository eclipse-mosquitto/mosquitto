/*
Copyright (c) 2009-2021 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

Contributors:
   Roger Light - initial implementation and documentation.
   Tatsuzo Osawa - Add epoll.
*/

#include "config.h"

#ifndef WIN32
#  include <errno.h>
#  include <fcntl.h>
#  include <string.h>
#  include <unistd.h>
#endif

#include "mux.h"

#ifndef WIN32
/* Self-pipe used by the signal handler to wake up the mux. Without it, a
 * signal that arrives after the main loop has checked g_run and the signal
 * flags, but before the mux starts waiting, is not acted upon until the wait
 * times out, which may be a long time. */
static int wakeup_pipe[2] = {-1, -1};


static int mux__wakeup_init(void)
{
	if(pipe(wakeup_pipe) == -1){
		log__printf(NULL, MOSQ_LOG_ERR, "Error creating wakeup pipe: %s", strerror(errno));
		return MOSQ_ERR_UNKNOWN;
	}
	for(int i=0; i<2; i++){
		int flags = fcntl(wakeup_pipe[i], F_GETFL, 0);
		if(flags == -1
				|| fcntl(wakeup_pipe[i], F_SETFL, flags | O_NONBLOCK) == -1
				|| fcntl(wakeup_pipe[i], F_SETFD, FD_CLOEXEC) == -1){

			log__printf(NULL, MOSQ_LOG_ERR, "Error configuring wakeup pipe: %s", strerror(errno));
			return MOSQ_ERR_UNKNOWN;
		}
	}
	return MOSQ_ERR_SUCCESS;
}


static void mux__wakeup_cleanup(void)
{
	for(int i=0; i<2; i++){
		if(wakeup_pipe[i] != -1){
			(void)close(wakeup_pipe[i]);
			wakeup_pipe[i] = -1;
		}
	}
}


int mux__wakeup_fd(void)
{
	return wakeup_pipe[0];
}


void mux__wakeup_drain(void)
{
	char buf[64];

	while(read(wakeup_pipe[0], buf, sizeof(buf)) > 0){
	}
}
#endif


/* Called from signal handlers, so must be async-signal-safe. */
void mux__wakeup(void)
{
#ifndef WIN32
	int saved_errno = errno;

	if(wakeup_pipe[1] != -1){
		/* If the pipe is full a wakeup is already pending, so a failed write
		 * does not matter. */
		if(write(wakeup_pipe[1], "", 1) == -1){
		}
	}
	errno = saved_errno;
#endif
}


int mux__init(void)
{
#ifndef WIN32
	if(mux__wakeup_init()){
		return MOSQ_ERR_UNKNOWN;
	}
#endif

#ifdef WITH_EPOLL
	return mux_epoll__init();
#elif defined(WITH_KQUEUE)
	return mux_kqueue__init();
#else
	return mux_poll__init();
#endif
}


int mux__add_listeners(struct mosquitto__listener_sock *listensock, int listensock_count)
{
#ifdef WITH_EPOLL
	return mux_epoll__add_listeners(listensock, listensock_count);
#elif defined(WITH_KQUEUE)
	return mux_kqueue__add_listeners(listensock, listensock_count);
#else
	return mux_poll__add_listeners(listensock, listensock_count);
#endif
}


int mux__delete_listeners(struct mosquitto__listener_sock *listensock, int listensock_count)
{
#ifdef WITH_EPOLL
	return mux_epoll__delete_listeners(listensock, listensock_count);
#elif defined(WITH_KQUEUE)
	return mux_kqueue__delete_listeners(listensock, listensock_count);
#else
	return mux_poll__delete_listeners(listensock, listensock_count);
#endif
}


int mux__add_out(struct mosquitto *context)
{
#ifdef WITH_EPOLL
	return mux_epoll__add_out(context);
#elif defined(WITH_KQUEUE)
	return mux_kqueue__add_out(context);
#else
	return mux_poll__add_out(context);
#endif
}


int mux__remove_out(struct mosquitto *context)
{
#ifdef WITH_EPOLL
	return mux_epoll__remove_out(context);
#elif defined(WITH_KQUEUE)
	return mux_kqueue__remove_out(context);
#else
	return mux_poll__remove_out(context);
#endif
}


int mux__new(struct mosquitto *context)
{
#ifdef WITH_EPOLL
	return mux_epoll__new(context);
#elif defined(WITH_KQUEUE)
	return mux_kqueue__new(context);
#else
	return mux_poll__new(context);
#endif
}


int mux__delete(struct mosquitto *context)
{
#ifdef WITH_EPOLL
	return mux_epoll__delete(context);
#elif defined(WITH_KQUEUE)
	return mux_kqueue__delete(context);
#else
	return mux_poll__delete(context);
#endif
}


int mux__handle(struct mosquitto__listener_sock *listensock, int listensock_count)
{
#ifdef WITH_EPOLL
	UNUSED(listensock);
	UNUSED(listensock_count);
	return mux_epoll__handle();
#elif defined(WITH_KQUEUE)
	UNUSED(listensock);
	UNUSED(listensock_count);
	return mux_kqueue__handle();
#else
	return mux_poll__handle(listensock, listensock_count);
#endif
}


int mux__cleanup(void)
{
	int rc;

#ifdef WITH_EPOLL
	rc = mux_epoll__cleanup();
#elif defined(WITH_KQUEUE)
	rc = mux_kqueue__cleanup();
#else
	rc = mux_poll__cleanup();
#endif
#ifndef WIN32
	mux__wakeup_cleanup();
#endif
	return rc;
}
