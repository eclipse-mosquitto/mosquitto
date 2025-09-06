R=../..
include ${R}/fuzzing/config.mk

.PHONY: all clean

FUZZERS:= \
	libcommon_fuzz_pub_topic_check2 \
	libcommon_fuzz_sub_topic_check2 \
	libcommon_fuzz_topic_matching \
	libcommon_fuzz_topic_tokenise \
	libcommon_fuzz_utf8

LOCAL_CPPFLAGS+=-I${R}/include/ -I/usr/local/include/libprotobuf-mutator -I/src/libprotobuf-mutator/external.protobuf/include
LOCAL_CXXFLAGS+=-g -Wall -Werror -pthread -Wno-deprecated-builtins
LOCAL_LDFLAGS+=
LOCAL_LIBADD+=$(LIB_FUZZING_ENGINE) -lssl -lcrypto ${R}/libcommon/libmosquitto_common.a -Wl,-Bstatic -largon2 -Wl,-Bdynamic
PROTOBUF_LIBS=/usr/local/lib/libprotobuf-mutator-libfuzzer.a /usr/local/lib/libprotobuf-mutator.a /src/libprotobuf-mutator/external.protobuf/lib/libprotobufd.a
PROTOC?=/src/libprotobuf-mutator/external.protobuf/bin/protoc

all: $(FUZZERS)

libcommon_fuzz_pub_topic_check2 : libcommon_fuzz_pub_topic_check2.cpp
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)
	install $@ ${OUT}/$@

libcommon_fuzz_sub_topic_check2 : libcommon_fuzz_sub_topic_check2.cpp
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)
	install $@ ${OUT}/$@

libcommon_fuzz_topic_matching : libcommon_fuzz_topic_matching.cpp libcommon_fuzz_topic_matching.pb.cc
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD) $(PROTOBUF_LIBS)
	install $@ ${OUT}/$@

libcommon_fuzz_topic_matching.pb.cc : libcommon_fuzz_topic_matching.proto
	$(PROTOC) --cpp_out=. $^

libcommon_fuzz_topic_tokenise : libcommon_fuzz_topic_tokenise.cpp
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)
	install $@ ${OUT}/$@

libcommon_fuzz_utf8 : libcommon_fuzz_utf8.cpp
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD)
	install $@ ${OUT}/$@

clean:
	rm -f *.o $(FUZZERS)
