#!/usr/bin/env python3

# Test whether a client subscribed to a topic receives its own message sent to that topic.
# Does the Payload Format Indicator property get sent through?
# MQTT v5

import prop_subpub_helper as helper
from mosq_test_helper import *

def do_test():
    props_out = mqtt5_props.gen_byte_prop(mqtt5_props.PAYLOAD_FORMAT_INDICATOR, 0xed)
    props_out = props_out+mqtt5_props.gen_uint16_prop(mqtt5_props.TOPIC_ALIAS, 1)

    props_in = mqtt5_props.gen_byte_prop(mqtt5_props.PAYLOAD_FORMAT_INDICATOR, 0xed)

    return helper.prop_subpub_helper("12-prop-subpub-payload-format", props_out, props_in, expect_proto_error=True)


if __name__ == '__main__':
    do_test()
