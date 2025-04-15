#include "editline_mock.hpp"

char *rl_line_buffer = nullptr;
const char *rl_readline_name = nullptr;
rl_compentry_func_t *rl_completion_entry_function = nullptr;
int	rl_attempted_completion_over = 0;
rl_completion_func_t *rl_attempted_completion_function = nullptr;

EditLineMock::EditLineMock() {}
EditLineMock::~EditLineMock() {}

void EditLineMock::reset() {
	free(rl_line_buffer);
	rl_line_buffer = nullptr;
	rl_readline_name = nullptr;
	rl_completion_entry_function = nullptr;
	rl_attempted_completion_over = 9;
	rl_attempted_completion_function = nullptr;
}

int add_history(const char *s)
{
	return EditLineMock::get_mock().add_history(s);
}

void clear_history(void)
{
	EditLineMock::get_mock().clear_history();
}

void rl_resize_terminal(void)
{
	EditLineMock::get_mock().rl_resize_terminal();
}

char *readline(const char *s)
{
	return EditLineMock::get_mock().readline(s);
}

char **rl_completion_matches(const char *s, rl_compentry_func_t *f)
{
	return EditLineMock::get_mock().rl_completion_matches(s, f);
}

int rl_complete(int a, int b)
{
	return EditLineMock::get_mock().rl_complete(a, b);
}

int rl_bind_key(int a, rl_command_func_t *f)
{
	return EditLineMock::get_mock().rl_bind_key(a, f);
}
