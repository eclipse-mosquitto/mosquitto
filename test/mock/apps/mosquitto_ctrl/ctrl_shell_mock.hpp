#pragma once

#include <gmock/gmock.h>
#include <editline/readline.h>

#include "ctrl_shell_internal.h"

#include "c_function_mock.hpp"

class CtrlShellMock : public CFunctionMock<CtrlShellMock> {
	public:
		CtrlShellMock();
		virtual ~CtrlShellMock();

		MOCK_METHOD(void, ctrl_shell__output, (const char *s));
		MOCK_METHOD(char *, ctrl_shell_fgets, (char *s, int size, FILE *stream));
};
