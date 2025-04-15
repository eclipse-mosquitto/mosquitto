#pragma once

#include <gmock/gmock.h>
#include <editline/readline.h>

#include "c_function_mock.hpp"

class EditLineMock : public CFunctionMock<EditLineMock> {
	public:
		EditLineMock();
		virtual ~EditLineMock();
		void reset();

		MOCK_METHOD(int, add_history, (const char *s));
		MOCK_METHOD(void, clear_history, ());
		MOCK_METHOD(void, rl_resize_terminal, ());
		MOCK_METHOD(char *, readline, (const char *s));
		MOCK_METHOD(char **, rl_completion_matches, (const char *s, rl_compentry_func_t *f));
		MOCK_METHOD(int, rl_complete, (int a, int b));
		MOCK_METHOD(int, rl_bind_key, (int a, rl_command_func_t *f));
};
