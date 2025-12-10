R=../..
include ${R}/fuzzing/config.mk

.PHONY: all clean

FUZZERS:= \
	libcommon_fuzz_property \
	libcommon_fuzz_pub_topic_check2 \
	libcommon_fuzz_sub_topic_check2 \
	libcommon_fuzz_topic_matching \
	libcommon_fuzz_topic_tokenise \
	libcommon_fuzz_utf8

LOCAL_CPPFLAGS+=-I${R}/include/ -I${SRC}/libprotobuf-mutator -I${SRC}/LPM/external.protobuf/include
LOCAL_CXXFLAGS+=-g -Wall -Werror -pthread -Wno-deprecated-builtins
LOCAL_LDFLAGS+=
LOCAL_LIBADD+=$(LIB_FUZZING_ENGINE) -lssl -lcrypto ${R}/libcommon/libmosquitto_common.a -Wl,-Bstatic -largon2 -Wl,-Bdynamic
PROTOBUF_LIBS=${SRC}/LPM/src/libfuzzer/libprotobuf-mutator-libfuzzer.a \
			  ${SRC}/LPM/src/libprotobuf-mutator.a \
			  -Wl,--start-group \
			  ${SRC}/LPM/external.protobuf/lib/lib*.a \
			  -Wl,--end-group
PROTOC?=/src/libprotobuf-mutator/external.protobuf/bin/protoc

all: $(FUZZERS)

libcommon_fuzz_property.pb.cc : libcommon_fuzz_property.proto
	$(PROTOC) --cpp_out=. $^

libcommon_fuzz_property : libcommon_fuzz_property.cpp libcommon_fuzz_property.pb.cc
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) $(LOCAL_LDFLAGS) -o $@ $^ $(LOCAL_LIBADD) $(PROTOBUF_LIBS)
	install $@ ${OUT}/$@

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
