R=../../..
include ${R}/config.mk

.PHONY: all check test-compile test clean

LOCAL_CPPFLAGS+= \
	-I../ \
	-I${R}/include \
	-I${R} \
	-I${R}/lib \
	-I${R}/test/mock

LOCAL_CXXFLAGS+=-std=c++20 -Wall -ggdb -D TEST_SOURCE_DIR='"$(realpath .)"'

MOCK_OBJS = \
	actions_publish_mock.o \
	actions_subscribe_mock.o \
	actions_unsubscribe_mock.o \
	callbacks_mock.o \
	connect_mock.o \
	extended_auth_mock.o \
	helpers_mock.o \
	libmosquitto_mock.o \
	loop_mock.o \
	messages_mosq_mock.o \
	net_mosq_mock.o \
	options_mock.o \
	socks_mosq_mock.o \
	srv_mosq_mock.o \
	thread_mosq_mock.o

all : test-compile

test-compile: ${MOCK_OBJS}
check : test

# MOCKS

${MOCK_OBJS} : %.o: %.cpp libmosquitto_mock.hpp
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_CXXFLAGS) -c $< -o $@

clean :
	-rm -rf *.o *.gcda *.gcno

install:

uninstall:
