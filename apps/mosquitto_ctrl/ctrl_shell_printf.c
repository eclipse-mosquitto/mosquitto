/*
Copyright (c) 2025 Roger Light <roger@atchoo.org>

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
#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#include "ctrl_shell_internal.h"

void ctrl_shell_vprintf(const char *fmt, va_list va)
{
	char buf[2000];
	vsnprintf(buf, sizeof(buf), fmt, va);
	ctrl_shell__output(buf);
}

void ctrl_shell_printf(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	ctrl_shell_vprintf(fmt, va);
	va_end(va);
}
