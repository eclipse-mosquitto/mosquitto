/*
Copyright (c) 2022 Cedalo GmbH
*/

// clang-format off
#include "mosquitto_internal.h" // keep this at the top for `#define uthash_free`
#include "ctrl_shell.h"
#include "ctrl_shell_internal.h"
#include "json_help.h"
#include "utlist.h"
// clang-format on
#include <gmock/gmock-actions.h>
#include <gtest/gtest.h>

#include <cstring>

#include "ctrl_shell_mock.hpp"
#include "editline_mock.hpp"
#include "libmosquitto_mock.hpp"
#include "pthread_mock.hpp"

namespace t = testing;

typedef void (*LIBMOSQ_FUNC_connect_callback)(struct mosquitto *, void *, int);
typedef void (*LIBMOSQ_FUNC_message_callback)(struct mosquitto *, void *, const struct mosquitto_message *);

struct pending_payload{
	struct pending_payload *next, *prev;
	char payload[1024];
};

class CtrlShellOptionsTest : public ::t::Test
{
	public:
		::t::StrictMock<CtrlShellMock> ctrl_shell_mock_{};
		::t::StrictMock<EditLineMock> editline_mock_{};
		::t::StrictMock<LibMosquittoMock> libmosquitto_mock_{};
		::t::StrictMock<PThreadMock> pthread_mock_{};
		LIBMOSQ_FUNC_connect_callback on_connect{};
		LIBMOSQ_FUNC_message_callback on_message{};
		struct pending_payload *pending_payloads = nullptr;

		void expect_setup(struct mosq_config *config)
		{
			editline_mock_.reset();
			EXPECT_CALL(editline_mock_, rl_bind_key(t::Eq('\t'), t::_));
			EXPECT_CALL(editline_mock_, add_history(t::_)).WillRepeatedly(t::Return(0));
			EXPECT_CALL(editline_mock_, clear_history()).Times(t::AnyNumber());
			config->no_colour = true;
			config->port = PORT_UNDEFINED;

			EXPECT_CALL(ctrl_shell_mock_, ctrl_shell__output(t::StartsWith("mosquitto_ctrl shell v")));
		}

		void expect_connect(struct mosquitto *mosq, const char *host, int port)
		{
			EXPECT_CALL(libmosquitto_mock_, mosquitto_new(t::Eq(nullptr), t::Eq(true), t::Eq(nullptr)))
				.WillOnce(t::Return(mosq));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_int_option(t::Eq(mosq), MOSQ_OPT_PROTOCOL_VERSION, 5));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_subscribe_callback_set(t::Eq(mosq), t::_));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_publish_v5_callback_set(t::Eq(mosq), t::_));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_connect(t::Eq(mosq), t::StrEq(host), port, 60));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_loop_start(t::Eq(mosq)));

			EXPECT_CALL(libmosquitto_mock_, mosquitto_connect_callback_set(t::Eq(mosq), t::A<LIBMOSQ_FUNC_connect_callback>()))
				.WillRepeatedly(t::SaveArg<1>(&this->on_connect));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_message_callback_set(t::Eq(mosq), t::A<LIBMOSQ_FUNC_message_callback>()))
				.WillOnce(t::SaveArg<1>(&this->on_message));
		}

		void expect_disconnect(struct mosquitto *mosq)
		{
			EXPECT_CALL(libmosquitto_mock_, mosquitto_disconnect(t::Eq(mosq)));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_loop_stop(t::Eq(mosq), false));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_destroy(t::Eq(mosq)));
		}

		void expect_outputs(const char **outputs, size_t count)
		{
			for(size_t i=0; i<count; i++){
				EXPECT_CALL(ctrl_shell_mock_, ctrl_shell__output(t::StrEq(outputs[i]))).Times(t::AtLeast(1));
			}
		}

		void expect_request_response(struct mosquitto *mosq, const char *request, const char *respons)
		{
			struct pending_payload *pp = (struct pending_payload *)calloc(1, sizeof(struct pending_payload));
			snprintf(pp->payload, sizeof(pp->payload), "%s", respons);

			EXPECT_CALL(libmosquitto_mock_, mosquitto_publish(t::Eq(mosq), nullptr, t::StrEq("$CONTROL/broker/v1"), t::_,
						t::StrEq(request), 1, false))
				.WillOnce(t::Invoke([this, pp](){
					DL_APPEND(this->pending_payloads, pp);
					return 0;
				}));
		}

		void expect_request_response_success(struct mosquitto *mosq, const char *request, const char *command)
		{
			char response[100];
			snprintf(response, sizeof(response), "{\"responses\":[{\"command\":\"%s\"}]}", command);
			expect_request_response(mosq, request, response);
		}

		void expect_request_response_empty(struct mosquitto *mosq, const char *command)
		{
			char request[100];
			char response[100];

			snprintf(request, sizeof(request), "{\"commands\":[{\"command\":\"%s\"}]}", command);
			snprintf(response, sizeof(response), "{\"responses\":[{\"command\":\"%s\",\"data\":{}}]}", command);

			EXPECT_CALL(libmosquitto_mock_, mosquitto_publish(t::Eq(mosq), nullptr, t::StrEq("$CONTROL/broker/v1"), t::_,
						t::StrEq(request), 1, false))
				.WillOnce(t::Invoke([this, &command](){
					append_empty_response(command);
					return 0;
					}));
		}

		void append_response(const char *response)
		{
			struct pending_payload *pp = (struct pending_payload *)calloc(1, sizeof(struct pending_payload));
			snprintf(pp->payload, sizeof(pp->payload), "%s", response);
			DL_APPEND(this->pending_payloads, pp);
		}

		void append_empty_response(const char *command)
		{
			struct pending_payload *pp = (struct pending_payload *)calloc(1, sizeof(struct pending_payload));
			snprintf(pp->payload, sizeof(pp->payload),
				"{\"responses\":[{\"command\":\"%s\",\"data\":{}}]}", command);
			DL_APPEND(this->pending_payloads, pp);
		}

		void expect_broker(const char *host, int port)
		{
			char buf[200];
			snprintf(buf, sizeof(buf), "connect mqtt://%s:%d", host, port);
			char *s_conn = strdup(buf);

			EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
				.WillOnce(t::Return(s_conn));

			EXPECT_CALL(editline_mock_, readline(t::StrEq("mqtt://localhost:1883> ")))
				.WillOnce(t::Return(strdup("broker")));

			EXPECT_CALL(libmosquitto_mock_, mosquitto_subscribe(t::_, nullptr, t::StrEq("$CONTROL/broker/v1/response"), 1))
				.WillOnce(t::Return(0));
		}

		void expect_connect_and_messages(struct mosquitto *mosq)
		{
			/* This is a hacky way of working around the async mqtt send/receive which we don't directly control.
			* Each send starts a wait which times out after two seconds. We use that call to produce the effect we want.
			*/
			EXPECT_CALL(pthread_mock_, pthread_cond_timedwait(t::_, t::_, t::_))
				.WillOnce(t::Invoke([this, mosq](pthread_cond_t *, pthread_mutex_t *, const struct timespec *){
					this->on_connect(mosq, nullptr, 0); return 0; }))
				.WillRepeatedly(t::Invoke([this, mosq](pthread_cond_t *, pthread_mutex_t *, const struct timespec *){
					mosquitto_message msg{};
					struct pending_payload *pp = pending_payloads;
					if(pp){
						DL_DELETE(pending_payloads, pp);
						msg.payload = pp->payload;
						msg.payloadlen = (int)strlen((char *)msg.payload);
						this->on_message(mosq, nullptr, &msg);
						free(pp);
					}
					return 0;
					}));
		}
};


TEST_F(CtrlShellOptionsTest, ConnectTLS)
{
	mosq_config config{};
	mosquitto mosq{};
	const char host[] = "localhost";
	int port = 8883;

	expect_setup(&config);
	expect_connect(&mosq, host, port);
	expect_connect_and_messages(&mosq);


	char buf[200];
	snprintf(buf, sizeof(buf), "connect mqtts://%s:%d", host, port);
	char *s_conn = strdup(buf);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(s_conn));

	EXPECT_CALL(libmosquitto_mock_, mosquitto_int_option(t::_, MOSQ_OPT_TLS_USE_OS_CERTS, 1));

	EXPECT_CALL(editline_mock_, readline(t::StrEq("mqtts://localhost:8883> ")))
		.WillOnce(t::Return(strdup("exit")));

	const char *outputs[] = {
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
}


TEST_F(CtrlShellOptionsTest, ConnectWebsockets)
{
	mosq_config config{};
	mosquitto mosq{};
	const char host[] = "localhost";
	int port = 8080;

	expect_setup(&config);
	expect_connect(&mosq, host, port);
	expect_connect_and_messages(&mosq);


	char buf[200];
	snprintf(buf, sizeof(buf), "connect ws://%s:%d", host, port);
	char *s_conn = strdup(buf);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(s_conn));

	EXPECT_CALL(libmosquitto_mock_, mosquitto_int_option(t::_, MOSQ_OPT_TRANSPORT, MOSQ_T_WEBSOCKETS));

	EXPECT_CALL(editline_mock_, readline(t::StrEq("ws://localhost:8080> ")))
		.WillOnce(t::Return(strdup("exit")));

	const char *outputs[] = {
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
}


TEST_F(CtrlShellOptionsTest, ConnectImplicitPort)
{
	mosq_config config{};
	mosquitto mosq{};
	const char host[] = "localhost";
	int port = 1883;

	expect_setup(&config);
	expect_connect(&mosq, host, port);
	expect_connect_and_messages(&mosq);


	char buf[200];
	snprintf(buf, sizeof(buf), "connect %s", host);
	char *s_conn = strdup(buf);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(s_conn));

	EXPECT_CALL(editline_mock_, readline(t::StrEq("mqtt://localhost:1883> ")))
		.WillOnce(t::Return(strdup("exit")));

	const char *outputs[] = {
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
}


TEST_F(CtrlShellOptionsTest, ConnectCertNotFound)
{
	mosq_config config{};
	mosquitto mosq{};
	const char host[] = "localhost";
	int port = 1883;

	expect_setup(&config);
	expect_connect(&mosq, host, port);
	expect_connect_and_messages(&mosq);

	config.cafile = strdup("missing cafile");
	config.capath = strdup("missing capath");
	config.certfile = strdup("missing certfile");
	config.keyfile = strdup("missing keyfile");

	char buf[200];
	snprintf(buf, sizeof(buf), "connect %s", host);
	char *s_conn = strdup(buf);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(s_conn));

	EXPECT_CALL(editline_mock_, readline(t::StrEq("mqtt://localhost:1883> ")))
		.WillOnce(t::Return(strdup("exit")));

	EXPECT_CALL(libmosquitto_mock_, mosquitto_tls_set(
				t::_,
				t::StrEq("missing cafile"),
				t::StrEq("missing capath"),
				t::StrEq("missing certfile"),
				t::StrEq("missing keyfile"),
				nullptr))
		.WillOnce(t::Return(MOSQ_ERR_INVAL));

	const char *outputs[] = {
		"Error setting TLS options: File not found.\n",
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
	free(config.cafile);
	free(config.capath);
	free(config.certfile);
	free(config.keyfile);
}


TEST_F(CtrlShellOptionsTest, ConnectCertError)
{
	mosq_config config{};
	mosquitto mosq{};
	const char host[] = "localhost";
	int port = 1883;

	expect_setup(&config);
	expect_connect(&mosq, host, port);
	expect_connect_and_messages(&mosq);

	config.cafile = strdup("cafile");
	config.capath = strdup("capath");
	config.certfile = strdup("certfile");
	config.keyfile = strdup("keyfile");

	char buf[200];
	snprintf(buf, sizeof(buf), "connect %s", host);
	char *s_conn = strdup(buf);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(s_conn));

	EXPECT_CALL(editline_mock_, readline(t::StrEq("mqtt://localhost:1883> ")))
		.WillOnce(t::Return(strdup("exit")));

	EXPECT_CALL(libmosquitto_mock_, mosquitto_tls_set(
				t::_,
				t::StrEq("cafile"),
				t::StrEq("capath"),
				t::StrEq("certfile"),
				t::StrEq("keyfile"),
				nullptr))
		.WillOnce(t::Return(MOSQ_ERR_TLS));

	const char *outputs[] = {
		"Error setting TLS options: A TLS error occurred.\n",
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
	free(config.cafile);
	free(config.capath);
	free(config.certfile);
	free(config.keyfile);
}
