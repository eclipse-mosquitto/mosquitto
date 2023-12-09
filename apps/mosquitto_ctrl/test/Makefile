R=../../..
include ${R}/config.mk

.PHONY: all check test-compile test test-broker test-lib clean coverage

LOCAL_CPPFLAGS+= \
	-DWITH_THREADING \
	-DWITH_TLS \
	-I../ \
	-I${R}/include \
	-I${R} \
	-I${R}/lib \
	-I${R}/lib/mock \
	-I${R}/test/mock

LOCAL_CXXFLAGS+=-std=c++20 -Wall -ggdb -D TEST_SOURCE_DIR='"$(realpath .)"'
LOCAL_LDFLAGS+=
LOCAL_LIBADD+=-lcjson -lgmock -lgtest_main -lgtest ${R}/libcommon/libmosquitto_common.a

OBJS = \
	../ctrl_shell_broker.o \
	../ctrl_shell.o \
	../ctrl_shell_client.o \
	../ctrl_shell_completion_tree.o \
	../ctrl_shell_dynsec.o \
	../ctrl_shell_ha.o \
	../ctrl_shell_inspect.o \
	../ctrl_shell_license.o \
	../ctrl_shell_post_connect.o \
	../ctrl_shell_pre_connect.o \
	../ctrl_shell_printf.o \
	../ctrl_shell_topictree.o

LIBMOSQ_MOCKS = \
	${R}/lib/mock/libmosquitto_mock.o \
	${R}/lib/mock/actions_publish_mock.o \
	${R}/lib/mock/actions_subscribe_mock.o \
	${R}/lib/mock/callbacks_mock.o \
	${R}/lib/mock/connect_mock.o \
	${R}/lib/mock/loop_mock.o \
	${R}/lib/mock/options_mock.o \
	${R}/lib/mock/thread_mosq_mock.o

LIB_OBJS = \
	${OBJS} \
	json_help.o \
	ctrl_shell_mock.o \
	${R}/test/mock/editline_mock.o \
	${R}/test/mock/pthread_mock.o \
	${LIBMOSQ_MOCKS}

TEST_OBJS = \
	ctrl_shell_broker_test.o \
	ctrl_shell_dynsec_test.o \
	ctrl_shell_help_test.o \
	ctrl_shell_pre_connect_test.o

ALL_TESTS = \
	ctrl_shell_broker_test \
	ctrl_shell_dynsec_test \
	ctrl_shell_help_test \
	ctrl_shell_pre_connect_test

all : test-compile

check : test

# DEPS

${LIBMOSQ_MOCKS}:
	$(MAKE) -C ${R}/lib

${OBJS} :
	$(MAKE) -C ../

json_help.o : ${R}/common/json_help.c ${R}/common/json_help.h
	${CROSS_COMPILE}${CC} $(LOCAL_CPPFLAGS) $(LOCAL_CFLAGS) -c $< -o $@

# MOCKS

ctrl_shell_mock.o : ctrl_shell_mock.cpp ctrl_shell_mock.hpp
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_CXXFLAGS) -c $< -o $@

${R}/test/mock/editline_mock.o : ${R}/test/mock/editline_mock.cpp ${R}/test/mock/editline_mock.hpp
	$(MAKE) -C ${R}/test/mock test-compile

${R}/test/mock/pthread_mock.o : ${R}/test/mock/pthread_mock.cpp ${R}/test/mock/pthread_mock.hpp
	$(MAKE) -C ${R}/test/mock test-compile

# TESTS

${TEST_OBJS} : %.o: %.cpp
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_CXXFLAGS) -c $< -o $@

ctrl_shell_broker_test : ctrl_shell_broker_test.o ${LIB_OBJS}
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)

ctrl_shell_dynsec_test : ctrl_shell_dynsec_test.o ${LIB_OBJS}
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)

ctrl_shell_help_test : ctrl_shell_help_test.o ${LIB_OBJS}
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)

ctrl_shell_pre_connect_test : ctrl_shell_pre_connect_test.o ${LIB_OBJS}
	$(CROSS_COMPILE)$(CXX) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)


test-compile : $(ALL_TESTS)

test : test-compile
	./ctrl_shell_help_test
	./ctrl_shell_dynsec_test

clean :
	-rm -rf $(ALL_TESTS)
	-rm -rf *.o *.gcda *.gcno coverage.info out/

coverage :
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory out

install:

uninstall:
