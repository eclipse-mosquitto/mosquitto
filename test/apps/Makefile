R=../..
include ${R}/config.mk

.PHONY: all check test ptest clean
.NOTPARALLEL:

all :

check : test

test-compile:
	$(MAKE) -C ctrl test-compile

test :
ifeq ($(WITH_PERSISTENCE),yes)
	$(MAKE) -C db_dump test
endif
	$(MAKE) -C ctrl test
ifeq ($(WITH_TLS),yes)
	$(MAKE) -C passwd test
endif
	$(MAKE) -C signal test

ptest :
ifeq ($(WITH_PERSISTENCE),yes)
	$(MAKE) -C db_dump ptest
endif
	$(MAKE) -C ctrl ptest
ifeq ($(WITH_TLS),yes)
	$(MAKE) -C passwd ptest
endif
	$(MAKE) -C signal ptest

reallyclean : clean
clean :
	$(MAKE) -C db_dump clean
	$(MAKE) -C ctrl clean
	$(MAKE) -C passwd clean
	$(MAKE) -C signal clean
