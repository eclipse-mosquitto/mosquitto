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
*/

#include "mosquitto_broker_internal.h"

/* This is using the uthash internal hh.prev/hh.next pointers to make our own
 * double linked list. This function must not be used on a base_msg that is
 * stored in a hash table. */
void base_msg__dl_append(struct mosquitto__base_msg **head, struct mosquitto__base_msg *add_msg)
{
	if(*head){
		add_msg->hh.prev = db.plugin_msgs->hh.prev;
		((struct mosquitto__base_msg *)(*head)->hh.prev)->hh.next = add_msg;
		(*head)->hh.prev = add_msg;
		add_msg->hh.next = NULL;
	} else {
		(*head) = add_msg;
		(*head)->hh.prev = (*head);
		(*head)->hh.next = NULL;
	}
}

/* This is using the uthash internal hh.prev/hh.next pointers to make our own
 * double linked list. This function must not be used on a base_msg that is
 * stored in a hash table. */
void base_msg__dl_delete(struct mosquitto__base_msg **head, struct mosquitto__base_msg *del_msg)
{
	if(del_msg->hh.prev == del_msg){
		*head = NULL;
	}else if(del_msg == *head){
		((struct mosquitto__base_msg *)del_msg->hh.next)->hh.prev = del_msg->hh.prev;
		*head = del_msg->hh.next;
	}else{
		((struct mosquitto__base_msg *)del_msg->hh.prev)->hh.next = del_msg->hh.next;
		if(del_msg->hh.next){
			((struct mosquitto__base_msg *)del_msg->hh.next)->hh.prev = del_msg->hh.prev;
		}else{
			(*head)->hh.prev = del_msg->hh.prev;
		}
	}
}
