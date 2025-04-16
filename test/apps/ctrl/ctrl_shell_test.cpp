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

struct pending_payload{
	struct pending_payload *next, *prev;
	char payload[1024];
};

class CtrlShellTest : public ::t::Test
{
	public:
		::t::StrictMock<CtrlShellMock> ctrl_shell_mock_{};
		::t::StrictMock<EditLineMock> editline_mock_{};
		::t::StrictMock<LibMosquittoMock> libmosquitto_mock_{};
		::t::StrictMock<PThreadMock> pthread_mock_{};
		LIBMOSQ_CB_connect on_connect{};
		LIBMOSQ_CB_message on_message{};
		struct pending_payload *pending_payloads = nullptr;

		void expect_setup(struct mosq_config *config)
		{
			editline_mock_.reset();
			EXPECT_CALL(editline_mock_, rl_bind_key(t::Eq('\t'), t::_));
			EXPECT_CALL(editline_mock_, add_history(t::_)).WillRepeatedly(t::Return(0));
			EXPECT_CALL(editline_mock_, clear_history()).Times(t::AnyNumber());
			config->no_colour = true;

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

			EXPECT_CALL(libmosquitto_mock_, mosquitto_connect_callback_set(t::Eq(mosq), t::A<LIBMOSQ_CB_connect>()))
				.WillRepeatedly(t::SaveArg<1>(&this->on_connect));
			EXPECT_CALL(libmosquitto_mock_, mosquitto_message_callback_set(t::Eq(mosq), t::A<LIBMOSQ_CB_message>()))
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
};


#if 0
/* Hangs on CI, presumably due to blocking read() */
TEST_F(CtrlShellTest, NoConfig)
{
	/* No config means we have colour mode enabled */
	mosq_config config{};

	expect_setup(&config);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("\x1\x1B[38;5;80m\x2>\x1\x1B[0m\x2 ")))
		.WillOnce(t::Return(strdup("exit")));

	const char *outputs[] = {
		"\x1B]10;?\a\x1B]11;?\a",
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(nullptr);
}
#endif

extern "C" { void set_no_colour(void); }

TEST_F(CtrlShellTest, PrintLabelValue)
{
	const char *outputs[] = {
		"My Label:",
		"   10\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	set_no_colour();
	ctrl_shell_print_label_value(0, "My Label:", 10, "%u\n", 10);
}
