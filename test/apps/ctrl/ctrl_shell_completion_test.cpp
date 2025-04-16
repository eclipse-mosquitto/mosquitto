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
#include "libmosquitto_mock.hpp"
#include "pthread_mock.hpp"

extern "C" {
	char **completion_matcher(const char *text, int start, int end);
	char *completion_generator(const char *text, int state);
	void ctrl_shell__cleanup(void);
}

namespace t = testing;

class CtrlShellCompletionTest : public ::t::Test
{
	public:
		::t::StrictMock<CtrlShellMock> ctrl_shell_mock_{};
		::t::StrictMock<LibMosquittoMock> libmosquitto_mock_{};
		::t::StrictMock<PThreadMock> pthread_mock_{};

		void expect_setup()
		{
			EXPECT_CALL(pthread_mock_, pthread_cond_timedwait(t::_, t::_, t::_))
				.WillRepeatedly(t::Return(0));

			EXPECT_CALL(libmosquitto_mock_, mosquitto_subscribe(t::_, t::_, t::_, 1))
				.WillRepeatedly(t::Return(0));

			EXPECT_CALL(libmosquitto_mock_, mosquitto_publish(t::_, nullptr, t::_, t::_, t::_, 1, false))
				.WillRepeatedly(t::Return(0));

			rl_readline_name = "mosquitto_ctrl";
			rl_completion_entry_function = completion_generator;
			rl_attempted_completion_function = completion_matcher;

			ctrl_shell__load_module(ctrl_shell__dynsec_init);
		}

		void expect_outputs(const char **outputs, size_t count)
		{
			for(size_t i=0; i<count; i++){
				EXPECT_CALL(ctrl_shell_mock_, ctrl_shell__output(t::StrEq(outputs[i]))).Times(t::AtLeast(1));
			}
		}
};

TEST_F(CtrlShellCompletionTest, NoMatch)
{
	expect_setup();

	rl_line_buffer = strdup("q");
	char **matches = completion_matcher("q", 0, 0);
	free(rl_line_buffer);
	ASSERT_EQ(matches, nullptr);
}

TEST_F(CtrlShellCompletionTest, MatchArg1)
{
	int match_count;

	expect_setup();

	rl_line_buffer = strdup("a");
	char **matches = completion_matcher("a", 0, 0);
	free(rl_line_buffer);
	ASSERT_NE(matches, nullptr);

	EXPECT_STREQ(matches[0], "add");
	for(match_count = 1; matches[match_count]; match_count++);
	ASSERT_EQ(match_count, 5);

	char *match_array[4] = {matches[1], matches[2], matches[3], matches[4]};

	EXPECT_THAT(match_array, t::UnorderedElementsAreArray({
				t::StrEq("addGroupRole"),
				t::StrEq("addRoleACL"),
				t::StrEq("addGroupClient"),
				t::StrEq("addClientRole")
				}));
	for(int i=0; i<match_count; i++){
		free(matches[i]);
	}
	free(matches);
	ctrl_shell__cleanup();
}
