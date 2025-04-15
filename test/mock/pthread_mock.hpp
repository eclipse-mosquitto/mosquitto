#pragma once

#include <gmock/gmock.h>
#include <pthread.h>

#include "c_function_mock.hpp"

class PThreadMock : public CFunctionMock<PThreadMock> {
	public:
		PThreadMock();
		virtual ~PThreadMock();

		MOCK_METHOD(int, pthread_cond_timedwait, (pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime));
};
