/*
Copyright (c) 2023 Cedalo GmbH

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

#define kMaxInputLength 100000
#include "fuzz_packet_read_base.h"

extern "C" int fuzz_packet_read_init(struct mosquitto *context)
{
	context->protocol = mosq_p_mqtt5;
	context->auth_method = strdup("FUZZ");
	return !context->auth_method;
}

extern "C" void fuzz_packet_read_cleanup(struct mosquitto *context)
{
	free(context->auth_method);
	context->auth_method = NULL;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	int rc = fuzz_packet_read_base(data, size, handle__auth);
	return rc;
}
