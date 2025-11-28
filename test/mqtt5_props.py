import struct

PAYLOAD_FORMAT_INDICATOR = 1
MESSAGE_EXPIRY_INTERVAL = 2
CONTENT_TYPE = 3
RESPONSE_TOPIC = 8
CORRELATION_DATA = 9
SUBSCRIPTION_IDENTIFIER = 11
SESSION_EXPIRY_INTERVAL = 17
ASSIGNED_CLIENT_IDENTIFIER = 18
SERVER_KEEP_ALIVE = 19
AUTHENTICATION_METHOD = 21
AUTHENTICATION_DATA = 22
REQUEST_PROBLEM_INFO = 23
WILL_DELAY_INTERVAL = 24
REQUEST_RESPONSE_INFO = 25
RESPONSE_INFO = 26
SERVER_REFERENCE = 28
REASON_STRING = 31
RECEIVE_MAXIMUM = 33
TOPIC_ALIAS_MAXIMUM = 34
TOPIC_ALIAS = 35
MAXIMUM_QOS = 36
RETAIN_AVAILABLE = 37
USER_PROPERTY = 38
MAXIMUM_PACKET_SIZE = 39
WILDCARD_SUB_AVAILABLE = 40
SUBSCRIPTION_ID_AVAILABLE = 41
SHARED_SUB_AVAILABLE = 42

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


def gen_properties(properties_dict: dict) -> bytes:
    props = b""
    if properties_dict is None:
        return props
    for prop in properties_dict:
        id = prop.get("identifier")
        value = prop.get("value")
        if id in (
            PAYLOAD_FORMAT_INDICATOR,
            REQUEST_PROBLEM_INFO,
            REQUEST_RESPONSE_INFO,
            MAXIMUM_QOS,
            RETAIN_AVAILABLE,
            WILDCARD_SUB_AVAILABLE,
            SUBSCRIPTION_ID_AVAILABLE,
            SHARED_SUB_AVAILABLE,
        ):
            props += gen_byte_prop(id, value)
        elif id in (
            MESSAGE_EXPIRY_INTERVAL,
            SESSION_EXPIRY_INTERVAL,
            WILL_DELAY_INTERVAL,
            MAXIMUM_PACKET_SIZE,
        ):
            props += gen_uint32_prop(id, value)
        elif id in (
            CONTENT_TYPE,
            RESPONSE_TOPIC,
            CORRELATION_DATA,
            ASSIGNED_CLIENT_IDENTIFIER,
            AUTHENTICATION_METHOD,
            AUTHENTICATION_DATA,
            RESPONSE_INFO,
            SERVER_REFERENCE,
            REASON_STRING,
        ):
            props += gen_string_prop(id, value)
        elif id == SUBSCRIPTION_IDENTIFIER:
            props += gen_varint_prop(id, value)
        elif id in (
            SERVER_KEEP_ALIVE,
            RECEIVE_MAXIMUM,
            TOPIC_ALIAS_MAXIMUM,
            TOPIC_ALIAS,
        ):
            props += gen_uint16_prop(id, value)
        elif id == USER_PROPERTY:
            props += gen_string_pair_prop(id, prop["name"], value)
    return props


def unpack_varint(b: bytes):
    def decode_len(b: bytes):
        for i in range(len(b)):
            if b[i] & 0x80 == 0:
                return i + 1
        return 0

    var_len = decode_len(b)
    variant = 0
    for i in range(var_len - 1, -1, -1):
        variant = 0x80 * variant + (struct.unpack("!B", b[i : i + 1])[0] & 0x7F)
    return variant, var_len


def unpack_string(b: bytes):
    str_len = struct.unpack("!B", b[0:1])[0]
    str_value = struct.unpack(f"!{str_len}s", b[1 : str_len + 1])[0]
    return str_value, str_len + 1


def unpack_property(b: bytes):
    id = struct.unpack("!B", b[0:1])[0]
    if id == PAYLOAD_FORMAT_INDICATOR:
        return "PAYLOAD_FORMAT_INDICATOR", struct.unpack("!B", b[1:2])[0], 2
    elif id == PAYLOAD_FORMAT_INDICATOR:
        return "PAYLOAD_FORMAT_INDICATOR", struct.unpack("!B", b[1:2])[0], 2
    elif id == REQUEST_PROBLEM_INFO:
        return "REQUEST_PROBLEM_INFO", struct.unpack("!B", b[1:2])[0], 2
    elif id == REQUEST_RESPONSE_INFO:
        return "REQUEST_RESPONSE_INFO", struct.unpack("!B", b[1:2])[0], 2
    elif id == MAXIMUM_QOS:
        return "MAXIMUM_QOS", struct.unpack("!B", b[1:2])[0], 2
    elif id == RETAIN_AVAILABLE:
        return "RETAIN_AVAILABLE", struct.unpack("!B", b[1:2])[0], 2
    elif id == WILDCARD_SUB_AVAILABLE:
        return "WILDCARD_SUB_AVAILABLE", struct.unpack("!B", b[1:2])[0], 2
    elif id == SUBSCRIPTION_ID_AVAILABLE:
        return "SUBSCRIPTION_ID_AVAILABLE", struct.unpack("!B", b[1:2])[0], 2
    elif id == SHARED_SUB_AVAILABLE:
        return "SHARED_SUB_AVAILABLE", struct.unpack("!B", b[1:2])[0], 2
    elif id == SERVER_KEEP_ALIVE:
        return "SERVER_KEEP_ALIVE", struct.unpack("!H", b[1:3])[0], 3
    elif id == RECEIVE_MAXIMUM:
        return "RECEIVE_MAXIMUM", struct.unpack("!H", b[1:3])[0], 3
    elif id == TOPIC_ALIAS_MAXIMUM:
        return "TOPIC_ALIAS_MAXIMUM", struct.unpack("!H", b[1:3])[0], 3
    elif id == TOPIC_ALIAS:
        return "TOPIC_ALIAS", struct.unpack("!H", b[1:3])[0], 3
    elif id == MESSAGE_EXPIRY_INTERVAL:
        return "MESSAGE_EXPIRY_INTERVAL", struct.unpack("!B", b[1:5])[0], 5
    elif id == SESSION_EXPIRY_INTERVAL:
        return "SESSION_EXPIRY_INTERVAL", struct.unpack("!B", b[1:5])[0], 5
    elif id == WILL_DELAY_INTERVAL:
        return "WILL_DELAY_INTERVAL", struct.unpack("!B", b[1:5])[0], 5
    elif id == MAXIMUM_PACKET_SIZE:
        return "MAXIMUM_PACKET_SIZE", struct.unpack("!B", b[1:5])[0], 5
    elif id == CONTENT_TYPE:
        value, pack_len = unpack_string(b[1:])
        return "CONTENT_TYPE", value, pack_len + 1
    elif id == RESPONSE_TOPIC:
        value, pack_len = unpack_string(b[1:])
        return "RESPONSE_TOPIC", value, pack_len + 1
    elif id == CORRELATION_DATA:
        value, pack_len = unpack_string(b[1:])
        return "CORRELATION_DATA", value, pack_len + 1
    elif id == ASSIGNED_CLIENT_IDENTIFIER:
        value, pack_len = unpack_string(b[1:])
        return "ASSIGNED_CLIENT_IDENTIFIER", value, pack_len + 1
    elif id == AUTHENTICATION_METHOD:
        value, pack_len = unpack_string(b[1:])
        return "AUTHENTICATION_METHOD", value, pack_len + 1
    elif id == AUTHENTICATION_DATA:
        value, pack_len = unpack_string(b[1:])
        return "AUTHENTICATION_DATA", value, pack_len + 1
    elif id == RESPONSE_INFO:
        value, pack_len = unpack_string(b[1:])
        return "RESPONSE_INFO", value, pack_len + 1
    elif id == SERVER_REFERENCE:
        value, pack_len = unpack_string(b[1:])
        return "SERVER_REFERENCE", value, pack_len + 1
    elif id == REASON_STRING:
        value, pack_len = unpack_string(b[1:])
        return "REASON_STRING", value, pack_len + 1
    elif id == SUBSCRIPTION_IDENTIFIER:
        value, pack_len = unpack_varint(b[1:])
        return "SUBSCRIPTION_IDENTIFIER", value, pack_len + 1
    elif id == PROP_USER_PROPERTY:
        name, pack_name_len = unpack_string(b[1:])
        value, pack_value_len = unpack_varint(b[1 + pack_name_len :])
        return (
            f"USER_PROPERTY:{name}",
            value,
            1 + pack_name_len + pack_value_len + pack_len,
        )
    else:
        return f"<Unknown property ID={id}>", f"not decoded (len<={len(b)-1})", len(b)


def print_properties(b: bytes):
    _, offset = unpack_varint(b)
    props = []
    while offset < len(b):
        try:
            key, value, prop_len = unpack_property(b[offset:])
            offset += prop_len
            props.append(f"{key}:{value}")
        except struct.error:
            props.append(f"decode error at offset {offset}: {b}")
            break
    return f"[{','.join(props)}]"
