/*
Copyright (c) 2022 Cedalo GmbH
*/

// clang-format off
#include "mosquitto_internal.h" // keep this at the top for `#define uthash_free`
#include "ctrl_shell.h"
#include "ctrl_shell_internal.h"
#include "json_help.h"
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

class CtrlShellPreConnectTest : public ::t::Test
{
	public:
		::t::StrictMock<CtrlShellMock> ctrl_shell_mock_{};
		::t::StrictMock<EditLineMock> editline_mock_{};
		::t::StrictMock<LibMosquittoMock> libmosquitto_mock_{};
		::t::StrictMock<PThreadMock> pthread_mock_{};
		LIBMOSQ_FUNC_connect_callback on_connect{};
		LIBMOSQ_FUNC_message_callback on_message{};

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
		void expect_empty_connect_and_messages(struct mosquitto *mosq)
		{
			/* This is a hacky way of working around the async mqtt send/receive which we don't directly control.
			* Each send starts a wait which times out after two seconds. We use that call to produce the effect we want.
			*/
			EXPECT_CALL(pthread_mock_, pthread_cond_timedwait(t::_, t::_, t::_))
				.WillOnce(t::Invoke([this, mosq](pthread_cond_t *, pthread_mutex_t *, const struct timespec *){
					this->on_connect(mosq, nullptr, 0); return 0; }))
				.WillRepeatedly(t::Invoke([this, mosq](pthread_cond_t *, pthread_mutex_t *, const struct timespec *){
					mosquitto_message msg{};
					this->on_message(mosq, nullptr, &msg); return 0; }));
		}
};


TEST_F(CtrlShellPreConnectTest, AuthNoUsername)
{
	mosq_config config{};

	expect_setup(&config);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(strdup("auth")))
		.WillOnce(t::Return(strdup("exit")));

	EXPECT_CALL(editline_mock_, readline(t::StrEq("username: ")))
		.WillOnce(t::Return(strdup("username1")));

	EXPECT_CALL(ctrl_shell_mock_, ctrl_shell_fgets(t::_, t::_, t::_))
		.WillOnce(t::Invoke([](char *s, int size, FILE *){
			snprintf(s, (size_t)size, "password1");
			return s;
		}));

	const char *outputs[] = {
		"password:",
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
}


TEST_F(CtrlShellPreConnectTest, AuthWithUsername)
{
	mosq_config config{};

	expect_setup(&config);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(strdup("auth username1")))
		.WillOnce(t::Return(strdup("exit")));

	EXPECT_CALL(ctrl_shell_mock_, ctrl_shell_fgets(t::_, t::_, t::_))
		.WillOnce(t::Invoke([](char *s, int size, FILE *){
			snprintf(s, (size_t)size, "password1");
			return s;
		}));

	const char *outputs[] = {
		"password:",
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
}


TEST_F(CtrlShellPreConnectTest, AuthNoPassword)
{
	mosq_config config{};

	expect_setup(&config);

	EXPECT_CALL(editline_mock_, readline(t::StrEq("> ")))
		.WillOnce(t::Return(strdup("auth username1")))
		.WillOnce(t::Return(strdup("exit")));

	EXPECT_CALL(ctrl_shell_mock_, ctrl_shell_fgets(t::_, t::_, t::_))
		.WillOnce(t::Return(nullptr));

	const char *outputs[] = {
		"password:",
		"No password.\n",
		"\n",
	};
	expect_outputs(outputs, sizeof(outputs)/sizeof(char *));

	ctrl_shell__main(&config);
}
