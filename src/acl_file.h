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
#ifndef ACL_FILE_H
#define ACL_FILE_H

#include <uthash.h>

struct acl__entry{
	struct acl__entry *next;
	char *topic;
	int access;
	int ucount;
	int ccount;
};


struct acl__user{
	UT_hash_handle hh;
	char *username;
	struct acl__entry *acl;
};


struct acl_file_data{
	char *acl_file;
	struct acl__user *acl_users;
	struct acl__user acl_anon;
	struct acl__entry *acl_patterns;
};


int acl__parse(struct acl_file_data *data);
int acl__check(int event, void *event_data, void *userdata);
int acl__cleanup(struct acl_file_data *data);

#endif
