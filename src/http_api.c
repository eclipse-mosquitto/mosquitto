/*
Copyright (c) 2025 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
	 https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
	http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR EDL-1.0

Contributors:
	 Roger Light - initial implementation and documentation.
*/

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <microhttpd.h>
#include <string.h>
#include <sys/stat.h>
#ifndef WIN32
#  include <netdb.h>
#endif

#include "json_help.h"
#include "mosquitto_broker_internal.h"
#include "mosquitto/mqtt_protocol.h"
#include "sys_tree.h"

#ifdef WITH_HTTP_API

#ifndef HTTP_API_DIR
#  define HTTP_API_DIR ""
#endif

struct metric{
	int64_t current;
	int64_t next;
	const char *topic, *topic_alias;
	bool is_max;
};

time_t broker_uptime(void);
struct MHD_Daemon *mhd = NULL;

#ifdef WITH_SYS_TREE
extern struct metric metrics[mosq_metric_max];
#endif

#define HTTP_RESPONSE_BUFLEN 10000

#ifdef WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif

static char *http__canonical_filename(
		const char *url,
		const char *http_dir,
		int *error_code)
{
	size_t urllen, slen;
	char *filename, *filename_canonical;

	urllen = strlen(url);
	if(url[urllen-1] == '/'){
		slen = strlen(http_dir) + urllen + strlen("/index.html") + 2;
	}else{
		slen = strlen(http_dir) + urllen + 2;
	}
	filename = mosquitto_malloc(slen);
	if(!filename){
		*error_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
		return NULL;
	}
	if(((char *)url)[urllen-1] == '/'){
		snprintf(filename, slen, "%s%c%sindex.html", http_dir, DIR_SEP, url);
	}else{
		snprintf(filename, slen, "%s%c%s", http_dir, DIR_SEP, url);
	}


	/* Get canonical path and check it is within our http_dir */
#ifdef WIN32
	filename_canonical = _fullpath(NULL, filename, 0);
	mosquitto_FREE(filename);
	if(!filename_canonical){
		*error_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
		return NULL;
	}
#else
	filename_canonical = realpath(filename, NULL);
	mosquitto_FREE(filename);
	if(!filename_canonical){
		if(errno == EACCES){
			*error_code = MHD_HTTP_FORBIDDEN;
		}else if(errno == EINVAL || errno == EIO || errno == ELOOP){
			*error_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
		}else if(errno == ENAMETOOLONG){
			*error_code = MHD_HTTP_URI_TOO_LONG;
		}else if(errno == ENOENT || errno == ENOTDIR){
			*error_code = MHD_HTTP_NOT_FOUND;
		}
		return NULL;
	}
#endif
	if(strncmp(http_dir, filename_canonical, strlen(http_dir))){
		/* Requested file isn't within http_dir, deny access because it's not found. */
		SAFE_FREE(filename_canonical);
		*error_code = MHD_HTTP_NOT_FOUND;
		return NULL;
	}

	return filename_canonical;
}

static enum MHD_Result http_api__send_error_response(struct MHD_Connection *connection, const char *error_str, unsigned int error_code)
{
	struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_str), (void *)error_str, MHD_RESPMEM_MUST_COPY);
	enum MHD_Result ret = MHD_queue_response(connection, error_code, response);
	MHD_destroy_response(response);
	return ret;
}

static enum MHD_Result http_api__send_response_with_headers(struct MHD_Connection *connection, const char *buf)
{
	enum MHD_Result ret;
	struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_MUST_COPY);

	ret = MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
	if(ret != MHD_YES){
		MHD_destroy_response(response);
		return ret;
	}
	ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
	MHD_destroy_response(response);
	return ret;
}

static enum MHD_Result http_api__process_version(struct MHD_Connection *connection)
{
	return http_api__send_response_with_headers(connection, VERSION);
}

static enum MHD_Result http_api__process_listeners(struct MHD_Connection *connection)
{
	char *buf;
	enum MHD_Result ret;

	cJSON *j_tree = cJSON_CreateObject();
	if(!j_tree){
		return http_api__send_error_response(connection, "Internal server error.\n", MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	cJSON *j_listeners = cJSON_AddArrayToObject(j_tree, "listeners");
	if(!j_listeners){
		cJSON_Delete(j_tree);
		return http_api__send_error_response(connection, "Internal server error.\n", MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	for(int i=0; i<db.config->listener_count; i++){
		cJSON *j_listener = cJSON_CreateObject();
		if(!j_listener){
			cJSON_Delete(j_tree);
			return http_api__send_error_response(connection, "Internal server error.\n", MHD_HTTP_INTERNAL_SERVER_ERROR);
		}
		cJSON_AddItemToArray(j_listeners, j_listener);

		struct mosquitto__listener *listener = &db.config->listeners[i];
#ifdef WITH_UNIX_SOCKETS
		if(listener->unix_socket_path){
			cJSON_AddStringToObject(j_listener, "path", listener->unix_socket_path);
		}else
#endif
		{
			cJSON_AddIntToObject(j_listener, "port", listener->port);
		}

		switch(listener->protocol){
			case mp_mqtt:
				cJSON_AddStringToObject(j_listener, "protocol", "mqtt");
				break;
			case mp_mqttsn:
				cJSON_AddStringToObject(j_listener, "protocol", "mqtt-sn");
				break;
			case mp_websockets:
				cJSON_AddStringToObject(j_listener, "protocol", "websockets");
				break;
			case mp_http_api:
				cJSON_AddStringToObject(j_listener, "protocol", "httpapi");
				break;
		}

		cJSON_AddBoolToObject(j_listener, "tls", listener->certfile && listener->keyfile);
		cJSON_AddBoolToObject(j_listener, "mtls", listener->require_certificate);
		cJSON_AddBoolToObject(j_listener, "allow_anonymous", listener->security_options->allow_anonymous);
	}
	buf = cJSON_Print(j_tree);
	cJSON_Delete(j_tree);
	if(buf){
		ret = http_api__send_response_with_headers(connection, buf);
		free(buf);
	}else{
		ret = http_api__send_error_response(connection, "Internal server error.\n", MHD_HTTP_INTERNAL_SERVER_ERROR);
	}

	return ret;
}


static enum MHD_Result http_api__process_systree(struct MHD_Connection *connection)
{
	enum MHD_Result ret;
#ifdef WITH_SYS_TREE
	char *buf;

	cJSON *j_tree = cJSON_CreateObject();
	for(int i=0; i<mosq_metric_max; i++){
		if(metrics[i].topic){
			cJSON_AddIntToObject(j_tree, metrics[i].topic, metrics[i].current);
		}
	}
	cJSON_AddIntToObject(j_tree, "$SYS/broker/uptime", broker_uptime());

	buf = cJSON_Print(j_tree);
	cJSON_Delete(j_tree);

	if(buf){
		ret = http_api__send_response_with_headers(connection, buf);
		free(buf);
	}else{
		ret = http_api__send_error_response(connection, "Internal server error.\n", MHD_HTTP_INTERNAL_SERVER_ERROR);
	}
#else
	ret = http_api__send_error_response(connection, "Not found.\n", 404);
#endif

	return ret;
}


static ssize_t http_file_read_cb(void *cls, uint64_t pos, char *buf, size_t max)
{
	FILE *fptr = cls;
	if(fseek(fptr, (long )pos, SEEK_SET) < 0){
		return -1;
	}
	return (ssize_t )fread(buf, 1, max, fptr);
}


static void http_file_free_cb(void *cls)
{
	FILE *fptr = cls;
	fclose(fptr);
}


static enum MHD_Result http_api__process_file(struct mosquitto__listener *listener, struct MHD_Connection *connection, const char *url)
{
	int error_code = MHD_HTTP_INTERNAL_SERVER_ERROR;
	char *canonical_filename;

	if(!listener || !listener->http_dir){
		http_api__send_error_response(connection, "Not found.\n", 404);
		return MHD_YES;
	}

	canonical_filename = http__canonical_filename(url, listener->http_dir, &error_code);
	if(!canonical_filename){
		http_api__send_error_response(connection, "Not found.\n", 404);
		return MHD_YES;
	}

	FILE *fptr = fopen(canonical_filename, "rb");
	if(!fptr){
		http_api__send_error_response(connection, "Not found.\n", 404);
		return MHD_YES;
	}

	struct stat statbuf;
	if(fstat(fileno(fptr), &statbuf)){
		fclose(fptr);
		http_api__send_error_response(connection, "Internal server error.\n", 500);
		return MHD_YES;
	}

	if(!S_ISREG(statbuf.st_mode)){
		fclose(fptr);
		http_api__send_error_response(connection, "Not found.\n", 404);
        return MHD_YES;
	}
	uint64_t flen = (uint64_t )statbuf.st_size;

	/* Using MHD_create_response_from_fd would be easier here, but is less portable */
	struct MHD_Response *response = MHD_create_response_from_callback(
			flen,
			65536,
			http_file_read_cb,
			fptr,
			http_file_free_cb);

	enum MHD_Result ret = MHD_queue_response(connection, 200, response);
	MHD_destroy_response(response);
	return ret;
}


static enum MHD_Result http_api__process_api(struct MHD_Connection *connection, const char *url)
{
	if(strcmp(url, "/api/v1/systree") == 0){
		return http_api__process_systree(connection);
	}else if(strcmp(url, "/api/v1/listeners") == 0){
		return http_api__process_listeners(connection);
	}else if(strcmp(url, "/api/v1/version") == 0){
		return http_api__process_version(connection);
	}else{
		return http_api__send_error_response(connection, "Not found.\n", 404);
	}
}


static enum MHD_Result http_api_handler(void *cls, struct MHD_Connection
		*connection, const char *url, const char *method, const char *version,
		const char *upload_data, size_t *upload_data_size, void **con_cls)
{
	UNUSED(version);
	UNUSED(upload_data);
	UNUSED(upload_data_size);
	UNUSED(con_cls);

	struct mosquitto__listener *listener = cls;

	if(strcmp(method, "GET") != 0){
		char *buf = "Invalid HTTP Method\n";
		struct MHD_Response *response = MHD_create_response_from_buffer(strlen(buf), (void *)buf, MHD_RESPMEM_MUST_COPY);
		enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response);
		MHD_destroy_response(response);
		return ret;
	}
	if(!strncasecmp(url, "/api/", strlen("/api/"))){
		return http_api__process_api(connection, url);
	}else{
		return http_api__process_file(listener, connection, url);
	}
}


int http_api__start_local(struct mosquitto__listener *listener)
{
	listener->host = mosquitto_strdup("127.0.0.1");
	if(!listener->host){
		return MOSQ_ERR_NOMEM;
	}
	listener->port = 9883;

	if(listener->http_dir == NULL && strlen(HTTP_API_DIR) > 1){
#ifdef WIN32
		char *http_dir_canonical = _fullpath(NULL, HTTP_API_DIR, 0);
#else
		char *http_dir_canonical = realpath(HTTP_API_DIR, NULL);
#endif
		if(!http_dir_canonical){
			return MOSQ_ERR_NOMEM;
		}
		mosquitto_FREE(listener->http_dir);
		listener->http_dir = mosquitto_strdup(http_dir_canonical);
		mosquitto_FREE(http_dir_canonical);
	}

	return http_api__start(listener);
}

int http_api__start(struct mosquitto__listener *listener)
{
	unsigned int flags = MHD_USE_AUTO_INTERNAL_THREAD;
	const char *bind_address;
	uint16_t port = 9883;
	char *x509_cert = NULL;
	char *x509_key = NULL;

	if(!listener->security_options){
		listener->security_options = mosquitto_calloc(1, sizeof(struct mosquitto__security_options));
		if(listener->security_options == NULL){
			return MOSQ_ERR_NOMEM;
		}
	}
	listener->security_options->allow_anonymous = true;
	listener->protocol = mp_http_api;

	bind_address = listener->host;
	port = listener->port;

	if(listener->certfile && listener->keyfile){
		if(mosquitto_read_file(listener->certfile, &x509_cert, NULL)){
			log__printf(NULL, MOSQ_LOG_ERR, "Error: Unable to load server certificate \"%s\". Check certfile.", listener->certfile);
			return MOSQ_ERR_INVAL;
		}
		if(mosquitto_read_file(listener->keyfile, &x509_key, NULL)){
			log__printf(NULL, MOSQ_LOG_ERR, "Error: Unable to load server key file \"%s\". Check keyfile.", listener->keyfile);
			mosquitto_FREE(x509_cert);
			return MOSQ_ERR_INVAL;
		}
		flags |= MHD_USE_TLS;
	}

	if(bind_address){
		char service[10];
		struct addrinfo hints;
		struct addrinfo *ainfo, *rp;

		snprintf(service, sizeof(service), "%d", port);

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;

		int rc = getaddrinfo(bind_address, service, &hints, &ainfo);
		if(rc){
			log__printf(NULL, MOSQ_LOG_ERR, "Unable to start http api listener.");
			mosquitto_FREE(x509_cert);
			mosquitto_FREE(x509_key);
			return MOSQ_ERR_ERRNO;
		}

		for(rp = ainfo; rp; rp = rp->ai_next){
			if(rp->ai_family == AF_INET || rp->ai_family == AF_INET6){
				break;
			}
		}
		if(!rp){
			log__printf(NULL, MOSQ_LOG_ERR, "Unable to start http api listener, could not find address.");
			freeaddrinfo(ainfo);
			mosquitto_FREE(x509_cert);
			mosquitto_FREE(x509_key);

			return MOSQ_ERR_INVAL;
		}

		if(rp->ai_family == AF_INET6){
			flags |= MHD_USE_IPv6;
		}
		listener->mhd = MHD_start_daemon(flags, port, NULL, NULL, &http_api_handler, listener,
				MHD_OPTION_SOCK_ADDR, rp->ai_addr,
				MHD_OPTION_HTTPS_MEM_CERT, x509_cert,
				MHD_OPTION_HTTPS_MEM_KEY, x509_key,
				MHD_OPTION_END);
		freeaddrinfo(ainfo);
	}else{
		listener->mhd = MHD_start_daemon(flags | MHD_USE_DUAL_STACK, port, NULL, NULL, &http_api_handler, listener,
				MHD_OPTION_HTTPS_MEM_CERT, x509_cert,
				MHD_OPTION_HTTPS_MEM_KEY, x509_key,
				MHD_OPTION_END);
	}

	mosquitto_FREE(x509_cert);
	mosquitto_FREE(x509_key);

	if(listener->mhd){
		log__printf(NULL, MOSQ_LOG_INFO, "Opening http api listen socket on port %d.", port);
		if(listener->http_dir){
			log__printf(NULL, MOSQ_LOG_INFO, "Using http_dir %s", listener->http_dir);
		}
		return MOSQ_ERR_SUCCESS;
	}else{
		return MOSQ_ERR_UNKNOWN;
	}
}

void http_api__stop(struct mosquitto__listener *listener)
{
	MHD_stop_daemon(listener->mhd);
	listener->mhd = NULL;
}

#endif
