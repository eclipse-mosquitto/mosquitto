include ../config.mk

.PHONY: all check test ptest clean ssl

all :

check : test

ssl: ssl/all-ca.crt

ssl/all-ca.crt: ssl/gen.sh
	cd "${<D}" && "${CURDIR}/${<}"
	test -e "$@"

test : utest ssl
	$(MAKE) -C broker test
	$(MAKE) -C lib test
	$(MAKE) -C client test

ptest : utest ssl
	$(MAKE) -C broker ptest
	$(MAKE) -C lib ptest
	$(MAKE) -C client test

utest :
	$(MAKE) -C unit test

reallyclean : clean
clean :
	$(MAKE) -C lib clean
	$(MAKE) -C broker clean
	$(MAKE) -C unit clean
