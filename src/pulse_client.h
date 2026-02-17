/*
Copyright (c) 2024 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License 2.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   https://www.eclipse.org/legal/epl-2.0/
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
*/

#ifndef PULSE_CLIENT_H
#define PULSE_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version constants for the Pulse agent */
#define PULSE_AGENT_VERSION "1.0.0"
#define PULSE_APPLICATION_VERSION "1.0.0"

/**
 * Initialize the Pulse gRPC client and perform the initial handshake.
 * On success, starts a background thread that sends heartbeats at the
 * interval specified by the server.
 *
 * @param server_address The gRPC server address (e.g., "localhost:50051")
 * @param agent_id UUID string for the agent
 * @param project_id UUID string for the project
 * @param auth_token Optional authorization token (connection string JWT) for metadata
 * @return 0 on success, non-zero on failure
 */
int pulse_handshake(const char *server_address, const char *agent_id, const char *project_id, const char *auth_token);

/**
 * Stop the heartbeat thread and cleanup the Pulse gRPC client resources.
 */
void pulse_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* PULSE_CLIENT_H */
