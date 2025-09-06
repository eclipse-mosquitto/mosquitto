#include "src/libfuzzer/libfuzzer_macro.h"

#include "libcommon_fuzz_topic_matching.pb.h"
#include "mosquitto.h"

DEFINE_PROTO_FUZZER(const fuzz_topic_matches_sub::FuzzerInput& fuzzer_input)
{
	bool result;
	const char *string1 = fuzzer_input.string1().c_str();
	const char *string2 = fuzzer_input.string2().c_str();
	const char *username = nullptr;
	const char *clientid = nullptr;

	if(fuzzer_input.has_username()){
		username = fuzzer_input.username().c_str();
	}
	if(fuzzer_input.has_clientid()){
		clientid = fuzzer_input.clientid().c_str();
	}

		//targeted_function_1(fuzzer_input.arg1(), fuzzer_input.arg2(), fuzzer_input.arg3());
	mosquitto_topic_matches_sub(string1, string2, &result);
	mosquitto_topic_matches_sub2(string1, strlen(string1), string2, strlen(string2), &result);
	mosquitto_topic_matches_sub_with_pattern(string1, string2, clientid, username, &result);

	mosquitto_sub_matches_acl(string1, string2, &result);
	mosquitto_sub_matches_acl_with_pattern(string1, string2, clientid, username, &result);
}
