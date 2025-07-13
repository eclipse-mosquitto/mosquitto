R=..
include ${R}/config.mk

DIRS= \
		mosquitto_ctrl \
		mosquitto_passwd \
		mosquitto_signal

ifeq ($(WITH_PERSISTENCE),yes)
DIRS+=db_dump
endif

.PHONY : all binary check clean reallyclean test install uninstall

all :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d}; done

binary :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} $@; done

clean :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} $@; done

reallyclean :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} $@; done

check : test
test :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} $@; done

install :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} $@; done

uninstall :
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} $@; done
