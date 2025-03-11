import struct

PROP_PAYLOAD_FORMAT_INDICATOR = 1
PROP_MESSAGE_EXPIRY_INTERVAL = 2
PROP_CONTENT_TYPE = 3
PROP_RESPONSE_TOPIC = 8
PROP_CORRELATION_DATA = 9
PROP_SUBSCRIPTION_IDENTIFIER = 11
PROP_SESSION_EXPIRY_INTERVAL = 17
PROP_ASSIGNED_CLIENT_IDENTIFIER = 18
PROP_SERVER_KEEP_ALIVE = 19
PROP_AUTHENTICATION_METHOD = 21
PROP_AUTHENTICATION_DATA = 22
PROP_REQUEST_PROBLEM_INFO = 23
PROP_WILL_DELAY_INTERVAL = 24
PROP_REQUEST_RESPONSE_INFO = 25
PROP_RESPONSE_INFO = 26
PROP_SERVER_REFERENCE = 28
PROP_REASON_STRING = 31
PROP_RECEIVE_MAXIMUM = 33
PROP_TOPIC_ALIAS_MAXIMUM = 34
PROP_TOPIC_ALIAS = 35
PROP_MAXIMUM_QOS = 36
PROP_RETAIN_AVAILABLE = 37
PROP_USER_PROPERTY = 38
PROP_MAXIMUM_PACKET_SIZE = 39
PROP_WILDCARD_SUB_AVAILABLE = 40
PROP_SUBSCRIPTION_ID_AVAILABLE = 41
PROP_SHARED_SUB_AVAILABLE = 42

def gen_byte_prop(identifier, byte):
    prop = struct.pack('BB', identifier, byte)
    return prop

def gen_uint16_prop(identifier, word):
    prop = struct.pack('!BH', identifier, word)
    return prop

def gen_uint32_prop(identifier, word):
    prop = struct.pack('!BI', identifier, word)
    return prop

def gen_string_prop(identifier, s):
    s = s.encode("utf-8")
    prop = struct.pack('!BH%ds'%(len(s)), identifier, len(s), s)
    return prop

def gen_string_pair_prop(identifier, s1, s2):
    s1 = s1.encode("utf-8")
    s2 = s2.encode("utf-8")
    prop = struct.pack('!BH%dsH%ds'%(len(s1), len(s2)), identifier, len(s1), s1, len(s2), s2)
    return prop

def gen_varint_prop(identifier, val):
    v = pack_varint(val)
    return struct.pack("!B"+str(len(v))+"s", identifier, v)

def pack_varint(varint):
    s = b""
    while True:
        byte = varint % 128
        varint = varint // 128
        # If there are more digits to encode, set the top bit of this digit
        if varint > 0:
            byte = byte | 0x80

        s = s + struct.pack("!B", byte)
        if varint == 0:
            return s

def prop_finalise(props):
    return pack_varint(len(props)) + props

