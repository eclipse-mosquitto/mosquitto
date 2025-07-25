R=..
include ${R}/config.mk

.PHONY: all check test test-compile ptest clean ssl

all :

test-compile:
ifeq ($(WITH_GMOCK),yes)
	$(MAKE) -C mock test-compile
endif
	$(MAKE) -C broker test-compile
	$(MAKE) -C client test-compile
	$(MAKE) -C lib test-compile
	$(MAKE) -C unit test-compile
	$(MAKE) -C apps test-compile

check : test

ssl: ssl/all-ca.crt

ssl/all-ca.crt: ssl/gen.sh
	cd "${<D}" && "${CURDIR}/${<}"
	test -e "$@"

test : utest ssl
	$(MAKE) -C broker test
	$(MAKE) -C lib test
	$(MAKE) -C client test
	$(MAKE) -C apps test

ptest : utest ssl
	$(MAKE) -C broker ptest
	$(MAKE) -C lib ptest
	$(MAKE) -C client ptest
	$(MAKE) -C apps ptest

utest :
	$(MAKE) -C unit test

reallyclean : clean
clean :
	$(MAKE) -C lib clean
	$(MAKE) -C broker clean
	$(MAKE) -C client clean
	$(MAKE) -C unit clean
	$(MAKE) -C apps clean
