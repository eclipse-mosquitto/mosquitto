#include "pthread_mock.hpp"

PThreadMock::PThreadMock() {}
PThreadMock::~PThreadMock() {}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
	return PThreadMock::get_mock().pthread_cond_timedwait(cond, mutex, abstime);
}
