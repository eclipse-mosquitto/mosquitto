#include "ctrl_shell_mock.hpp"

CtrlShellMock::CtrlShellMock() {}
CtrlShellMock::~CtrlShellMock() {}

void ctrl_shell__output(const char *s)
{
	return CtrlShellMock::get_mock().ctrl_shell__output(s);
}

char *ctrl_shell_fgets(char *s, int size, FILE *stream)
{
	return CtrlShellMock::get_mock().ctrl_shell_fgets(s, size, stream);
}
