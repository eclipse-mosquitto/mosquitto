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
	$(MAKE) -C passwd test
	$(MAKE) -C signal test

ptest :
ifeq ($(WITH_PERSISTENCE),yes)
	$(MAKE) -C db_dump ptest
endif
	$(MAKE) -C ctrl ptest
	$(MAKE) -C passwd ptest
	$(MAKE) -C signal ptest

reallyclean : clean
clean :
	$(MAKE) -C db_dump clean
	$(MAKE) -C ctrl clean
	$(MAKE) -C passwd clean
	$(MAKE) -C signal clean
