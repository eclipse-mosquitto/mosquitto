/*
Copyright (c) 2011-2021 Roger Light <roger@atchoo.org>

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
*/
#ifndef PASSWORD_FILE_H
#define PASSWORD_FILE_H

#include <uthash.h>

struct mosquitto__unpwd{
	UT_hash_handle hh;
	char *username;
	char *clientid;
	struct mosquitto_pw *pw;
};

struct password_file_data{
	struct mosquitto__unpwd *unpwd;
	char *password_file;
};

int password_file__parse(struct password_file_data *data);
int password_file__check(int event, void *event_data, void *userdata);
void password_file__cleanup(struct password_file_data *data);

#endif
