use std::fmt;
use std::io::{self, Read};

const CMD_CONNECT: u8 = 0x10;
const CMD_CONNACK: u8 = 0x20;
const CMD_PUBLISH: u8 = 0x30;
const CMD_PUBACK: u8 = 0x40;
const CMD_PUBREC: u8 = 0x50;
const CMD_PUBREL: u8 = 0x60;
const CMD_PUBCOMP: u8 = 0x70;
const CMD_SUBSCRIBE: u8 = 0x80;
const CMD_SUBACK: u8 = 0x90;
const CMD_UNSUBSCRIBE: u8 = 0xA0;
const CMD_UNSUBACK: u8 = 0xB0;
const CMD_PINGREQ: u8 = 0xC0;
const CMD_PINGRESP: u8 = 0xD0;
const CMD_DISCONNECT: u8 = 0xE0;

const PROP_PAYLOAD_FORMAT_INDICATOR: u32 = 0x01;
const PROP_MESSAGE_EXPIRY_INTERVAL: u32 = 0x02;
const PROP_CONTENT_TYPE: u32 = 0x03;
const PROP_RESPONSE_TOPIC: u32 = 0x08;
const PROP_CORRELATION_DATA: u32 = 0x09;
const PROP_SUBSCRIPTION_IDENTIFIER: u32 = 0x0B;
const PROP_SESSION_EXPIRY_INTERVAL: u32 = 0x11;
const PROP_ASSIGNED_CLIENT_IDENTIFIER: u32 = 0x12;
const PROP_SERVER_KEEP_ALIVE: u32 = 0x13;
const PROP_AUTHENTICATION_METHOD: u32 = 0x15;
const PROP_AUTHENTICATION_DATA: u32 = 0x16;
const PROP_REQUEST_PROBLEM_INFORMATION: u32 = 0x17;
const PROP_WILL_DELAY_INTERVAL: u32 = 0x18;
const PROP_REQUEST_RESPONSE_INFORMATION: u32 = 0x19;
const PROP_RESPONSE_INFORMATION: u32 = 0x1A;
const PROP_SERVER_REFERENCE: u32 = 0x1C;
const PROP_REASON_STRING: u32 = 0x1F;
const PROP_RECEIVE_MAXIMUM: u32 = 0x21;
const PROP_TOPIC_ALIAS_MAXIMUM: u32 = 0x22;
const PROP_TOPIC_ALIAS: u32 = 0x23;
const PROP_MAXIMUM_QOS: u32 = 0x24;
const PROP_RETAIN_AVAILABLE: u32 = 0x25;
const PROP_USER_PROPERTY: u32 = 0x26;
const PROP_MAXIMUM_PACKET_SIZE: u32 = 0x27;
const PROP_WILDCARD_SUB_AVAILABLE: u32 = 0x28;
const PROP_SUBSCRIPTION_ID_AVAILABLE: u32 = 0x29;
const PROP_SHARED_SUB_AVAILABLE: u32 = 0x2A;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProtocolVersion {
    V31,
    V311,
    V5,
}

impl ProtocolVersion {
    pub fn level(self) -> u8 {
        match self {
            Self::V31 => 3,
            Self::V311 => 4,
            Self::V5 => 5,
        }
    }

    fn from_level(level: u8) -> Result<Self, ProtocolError> {
        match level {
            3 => Ok(Self::V31),
            4 => Ok(Self::V311),
            5 => Ok(Self::V5),
            _ => Err(ProtocolError::UnsupportedProtocolVersion(level)),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Frame {
    pub command: u8,
    pub flags: u8,
    pub body: Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Will {
    pub topic: String,
    pub payload: Vec<u8>,
    pub qos: u8,
    pub retain: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Publication {
    pub topic: String,
    pub payload: Vec<u8>,
    pub qos: u8,
    pub retain: bool,
    pub packet_id: Option<u16>,
    pub dup: bool,
    pub topic_alias: Option<u16>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SubscriptionRequest {
    pub filter: String,
    pub qos: u8,
    pub no_local: bool,
    pub retain_as_published: bool,
    pub retain_handling: u8,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum MqttPacket {
    Connect {
        protocol: ProtocolVersion,
        clean_start: bool,
        keep_alive: u16,
        client_id: String,
        username: Option<String>,
        password: Option<Vec<u8>>,
        will: Option<Will>,
    },
    Publish(Publication),
    PubAck {
        packet_id: u16,
    },
    PubRec {
        packet_id: u16,
    },
    PubRel {
        packet_id: u16,
    },
    PubComp {
        packet_id: u16,
    },
    Subscribe {
        packet_id: u16,
        filters: Vec<SubscriptionRequest>,
    },
    Unsubscribe {
        packet_id: u16,
        filters: Vec<String>,
    },
    PingReq,
    Disconnect {
        reason_code: Option<u8>,
    },
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ProtocolError {
    Io(String),
    MalformedPacket(&'static str),
    UnsupportedProtocolVersion(u8),
}

impl fmt::Display for ProtocolError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Io(msg) => write!(f, "I/O error: {msg}"),
            Self::MalformedPacket(msg) => write!(f, "malformed MQTT packet: {msg}"),
            Self::UnsupportedProtocolVersion(level) => {
                write!(f, "unsupported MQTT protocol level {level}")
            }
        }
    }
}

impl std::error::Error for ProtocolError {}

impl From<io::Error> for ProtocolError {
    fn from(value: io::Error) -> Self {
        Self::Io(value.to_string())
    }
}

pub fn read_frame<R: Read>(reader: &mut R) -> Result<Frame, ProtocolError> {
    let mut first = [0_u8; 1];
    reader.read_exact(&mut first)?;
    let remaining_len = read_remaining_length(reader)?;
    let mut body = vec![0_u8; remaining_len as usize];
    reader.read_exact(&mut body)?;
    Ok(Frame {
        command: first[0] & 0xF0,
        flags: first[0] & 0x0F,
        body,
    })
}

fn read_remaining_length<R: Read>(reader: &mut R) -> Result<u32, ProtocolError> {
    let mut multiplier = 1_u32;
    let mut value = 0_u32;
    for _ in 0..4 {
        let mut byte = [0_u8; 1];
        reader.read_exact(&mut byte)?;
        value += ((byte[0] & 127) as u32) * multiplier;
        if (byte[0] & 128) == 0 {
            return Ok(value);
        }
        multiplier = multiplier
            .checked_mul(128)
            .ok_or(ProtocolError::MalformedPacket("remaining length overflow"))?;
    }
    Err(ProtocolError::MalformedPacket(
        "remaining length is too long",
    ))
}

pub fn decode_frame(
    frame: &Frame,
    current_protocol: Option<ProtocolVersion>,
) -> Result<MqttPacket, ProtocolError> {
    let mut cursor = Cursor::new(&frame.body);
    match frame.command {
        CMD_CONNECT => decode_connect(&mut cursor),
        CMD_PUBLISH => decode_publish(frame.flags, &mut cursor, current_protocol),
        CMD_PUBACK => Ok(MqttPacket::PubAck {
            packet_id: read_packet_id(&mut cursor)?,
        }),
        CMD_PUBREC => Ok(MqttPacket::PubRec {
            packet_id: read_packet_id(&mut cursor)?,
        }),
        CMD_PUBREL => Ok(MqttPacket::PubRel {
            packet_id: read_packet_id(&mut cursor)?,
        }),
        CMD_PUBCOMP => Ok(MqttPacket::PubComp {
            packet_id: read_packet_id(&mut cursor)?,
        }),
        CMD_SUBSCRIBE => decode_subscribe(&mut cursor, current_protocol),
        CMD_UNSUBSCRIBE => decode_unsubscribe(&mut cursor, current_protocol),
        CMD_PINGREQ => Ok(MqttPacket::PingReq),
        CMD_DISCONNECT => decode_disconnect(&mut cursor, current_protocol),
        _ => Err(ProtocolError::MalformedPacket("unsupported command")),
    }
}

fn decode_connect(cursor: &mut Cursor<'_>) -> Result<MqttPacket, ProtocolError> {
    let protocol_name = cursor.read_utf8()?;
    let level = cursor.read_u8()?;
    let protocol = ProtocolVersion::from_level(level)?;
    if (protocol == ProtocolVersion::V31 && protocol_name != "MQIsdp")
        || (protocol != ProtocolVersion::V31 && protocol_name != "MQTT")
    {
        return Err(ProtocolError::MalformedPacket("invalid protocol name"));
    }

    let flags = cursor.read_u8()?;
    let clean_start = (flags & 0x02) != 0;
    let will_flag = (flags & 0x04) != 0;
    let will_qos = (flags & 0x18) >> 3;
    let will_retain = (flags & 0x20) != 0;
    let password_flag = (flags & 0x40) != 0;
    let username_flag = (flags & 0x80) != 0;
    let keep_alive = cursor.read_u16()?;

    if protocol == ProtocolVersion::V5 {
        cursor.skip_properties()?;
    }

    let client_id = cursor.read_utf8()?;
    let will = if will_flag {
        if protocol == ProtocolVersion::V5 {
            cursor.skip_properties()?;
        }
        Some(Will {
            topic: cursor.read_utf8()?,
            payload: cursor.read_binary()?,
            qos: will_qos,
            retain: will_retain,
        })
    } else {
        None
    };
    let username = if username_flag {
        Some(cursor.read_utf8()?)
    } else {
        None
    };
    let password = if password_flag {
        Some(cursor.read_binary()?)
    } else {
        None
    };

    Ok(MqttPacket::Connect {
        protocol,
        clean_start,
        keep_alive,
        client_id,
        username,
        password,
        will,
    })
}

fn decode_publish(
    flags: u8,
    cursor: &mut Cursor<'_>,
    current_protocol: Option<ProtocolVersion>,
) -> Result<MqttPacket, ProtocolError> {
    let dup = (flags & 0x08) != 0;
    let qos = (flags & 0x06) >> 1;
    let retain = (flags & 0x01) != 0;
    if qos == 3 {
        return Err(ProtocolError::MalformedPacket("invalid publish qos"));
    }
    let topic = cursor.read_utf8()?;
    let packet_id = if qos > 0 {
        Some(cursor.read_u16()?)
    } else {
        None
    };
    let topic_alias = if current_protocol == Some(ProtocolVersion::V5) {
        cursor.read_publish_properties()?
    } else {
        None
    };
    let payload = cursor.read_remaining().to_vec();
    Ok(MqttPacket::Publish(Publication {
        topic,
        payload,
        qos,
        retain,
        packet_id,
        dup,
        topic_alias,
    }))
}

fn decode_subscribe(
    cursor: &mut Cursor<'_>,
    current_protocol: Option<ProtocolVersion>,
) -> Result<MqttPacket, ProtocolError> {
    let packet_id = cursor.read_u16()?;
    if current_protocol == Some(ProtocolVersion::V5) {
        cursor.skip_properties()?;
    }
    let mut filters = Vec::new();
    while cursor.remaining() > 0 {
        let filter = cursor.read_utf8()?;
        let options = cursor.read_u8()?;
        filters.push(SubscriptionRequest {
            filter,
            qos: options & 0x03,
            no_local: (options & 0x04) != 0,
            retain_as_published: (options & 0x08) != 0,
            retain_handling: (options & 0x30) >> 4,
        });
    }
    if filters.is_empty() {
        return Err(ProtocolError::MalformedPacket("empty subscribe"));
    }
    Ok(MqttPacket::Subscribe { packet_id, filters })
}

fn decode_unsubscribe(
    cursor: &mut Cursor<'_>,
    current_protocol: Option<ProtocolVersion>,
) -> Result<MqttPacket, ProtocolError> {
    let packet_id = cursor.read_u16()?;
    if current_protocol == Some(ProtocolVersion::V5) {
        cursor.skip_properties()?;
    }
    let mut filters = Vec::new();
    while cursor.remaining() > 0 {
        filters.push(cursor.read_utf8()?);
    }
    if filters.is_empty() {
        return Err(ProtocolError::MalformedPacket("empty unsubscribe"));
    }
    Ok(MqttPacket::Unsubscribe { packet_id, filters })
}

fn decode_disconnect(
    cursor: &mut Cursor<'_>,
    current_protocol: Option<ProtocolVersion>,
) -> Result<MqttPacket, ProtocolError> {
    if cursor.remaining() == 0 || current_protocol != Some(ProtocolVersion::V5) {
        return Ok(MqttPacket::Disconnect { reason_code: None });
    }
    let reason_code = cursor.read_u8()?;
    if cursor.remaining() > 0 {
        cursor.skip_properties()?;
    }
    Ok(MqttPacket::Disconnect {
        reason_code: Some(reason_code),
    })
}

fn read_packet_id(cursor: &mut Cursor<'_>) -> Result<u16, ProtocolError> {
    cursor.read_u16()
}

pub fn encode_frame(command: u8, flags: u8, body: &[u8]) -> Vec<u8> {
    let mut packet = Vec::with_capacity(1 + 4 + body.len());
    packet.push(command | flags);
    write_varint(body.len() as u32, &mut packet);
    packet.extend_from_slice(body);
    packet
}

pub fn encode_connack(
    protocol: ProtocolVersion,
    session_present: bool,
    reason_code: u8,
) -> Vec<u8> {
    let mut body = vec![u8::from(session_present), reason_code];
    if protocol == ProtocolVersion::V5 {
        body.push(0);
    }
    encode_frame(CMD_CONNACK, 0, &body)
}

pub fn encode_suback(protocol: ProtocolVersion, packet_id: u16, reason_codes: &[u8]) -> Vec<u8> {
    let mut body = Vec::with_capacity(4 + reason_codes.len());
    write_u16(packet_id, &mut body);
    if protocol == ProtocolVersion::V5 {
        body.push(0);
    }
    body.extend_from_slice(reason_codes);
    encode_frame(CMD_SUBACK, 0, &body)
}

pub fn encode_unsuback(protocol: ProtocolVersion, packet_id: u16, count: usize) -> Vec<u8> {
    let mut body = Vec::new();
    write_u16(packet_id, &mut body);
    if protocol == ProtocolVersion::V5 {
        body.push(0);
        body.extend(std::iter::repeat(0).take(count));
    }
    encode_frame(CMD_UNSUBACK, 0, &body)
}

pub fn encode_publish(protocol: ProtocolVersion, publication: &Publication) -> Vec<u8> {
    let mut body = Vec::new();
    write_utf8(&publication.topic, &mut body);
    if publication.qos > 0 {
        write_u16(publication.packet_id.unwrap_or(1), &mut body);
    }
    if protocol == ProtocolVersion::V5 {
        body.push(0);
    }
    body.extend_from_slice(&publication.payload);
    let flags = (if publication.dup { 0x08 } else { 0 })
        | ((publication.qos & 0x03) << 1)
        | u8::from(publication.retain);
    encode_frame(CMD_PUBLISH, flags, &body)
}

pub fn encode_puback(protocol: ProtocolVersion, packet_id: u16) -> Vec<u8> {
    encode_ack(CMD_PUBACK, 0, protocol, packet_id)
}

pub fn encode_pubrec(protocol: ProtocolVersion, packet_id: u16) -> Vec<u8> {
    encode_ack(CMD_PUBREC, 0, protocol, packet_id)
}

pub fn encode_pubcomp(protocol: ProtocolVersion, packet_id: u16) -> Vec<u8> {
    encode_ack(CMD_PUBCOMP, 0, protocol, packet_id)
}

fn encode_ack(command: u8, flags: u8, protocol: ProtocolVersion, packet_id: u16) -> Vec<u8> {
    let mut body = Vec::new();
    write_u16(packet_id, &mut body);
    if protocol == ProtocolVersion::V5 {
        body.push(0);
        body.push(0);
    }
    encode_frame(command, flags, &body)
}

pub fn encode_pingresp() -> Vec<u8> {
    encode_frame(CMD_PINGRESP, 0, &[])
}

pub fn encode_disconnect(protocol: ProtocolVersion, reason_code: u8) -> Vec<u8> {
    let body = if protocol == ProtocolVersion::V5 {
        vec![reason_code]
    } else {
        Vec::new()
    };
    encode_frame(CMD_DISCONNECT, 0, &body)
}

fn write_varint(mut value: u32, out: &mut Vec<u8>) {
    loop {
        let mut encoded = (value % 128) as u8;
        value /= 128;
        if value > 0 {
            encoded |= 128;
        }
        out.push(encoded);
        if value == 0 {
            break;
        }
    }
}

fn write_u16(value: u16, out: &mut Vec<u8>) {
    out.extend_from_slice(&value.to_be_bytes());
}

fn write_utf8(value: &str, out: &mut Vec<u8>) {
    write_u16(value.len() as u16, out);
    out.extend_from_slice(value.as_bytes());
}

struct Cursor<'a> {
    bytes: &'a [u8],
    pos: usize,
}

impl<'a> Cursor<'a> {
    fn new(bytes: &'a [u8]) -> Self {
        Self { bytes, pos: 0 }
    }

    fn remaining(&self) -> usize {
        self.bytes.len().saturating_sub(self.pos)
    }

    fn read_remaining(&mut self) -> &'a [u8] {
        let result = &self.bytes[self.pos..];
        self.pos = self.bytes.len();
        result
    }

    fn read_u8(&mut self) -> Result<u8, ProtocolError> {
        if self.remaining() < 1 {
            return Err(ProtocolError::MalformedPacket("unexpected end of packet"));
        }
        let value = self.bytes[self.pos];
        self.pos += 1;
        Ok(value)
    }

    fn read_u16(&mut self) -> Result<u16, ProtocolError> {
        let bytes = self.read_bytes(2)?;
        Ok(u16::from_be_bytes([bytes[0], bytes[1]]))
    }

    fn read_utf8(&mut self) -> Result<String, ProtocolError> {
        let len = self.read_u16()? as usize;
        let bytes = self.read_bytes(len)?;
        std::str::from_utf8(bytes)
            .map(str::to_owned)
            .map_err(|_| ProtocolError::MalformedPacket("invalid utf-8 string"))
    }

    fn read_binary(&mut self) -> Result<Vec<u8>, ProtocolError> {
        let len = self.read_u16()? as usize;
        Ok(self.read_bytes(len)?.to_vec())
    }

    fn read_bytes(&mut self, len: usize) -> Result<&'a [u8], ProtocolError> {
        if self.remaining() < len {
            return Err(ProtocolError::MalformedPacket("unexpected end of packet"));
        }
        let start = self.pos;
        self.pos += len;
        Ok(&self.bytes[start..self.pos])
    }

    fn read_varint(&mut self) -> Result<u32, ProtocolError> {
        let mut multiplier = 1_u32;
        let mut value = 0_u32;
        for _ in 0..4 {
            let byte = self.read_u8()?;
            value += ((byte & 127) as u32) * multiplier;
            if (byte & 128) == 0 {
                return Ok(value);
            }
            multiplier = multiplier
                .checked_mul(128)
                .ok_or(ProtocolError::MalformedPacket("varint overflow"))?;
        }
        Err(ProtocolError::MalformedPacket("varint is too long"))
    }

    fn skip_properties(&mut self) -> Result<(), ProtocolError> {
        let len = self.read_varint()? as usize;
        self.read_bytes(len)?;
        Ok(())
    }

    fn read_publish_properties(&mut self) -> Result<Option<u16>, ProtocolError> {
        let len = self.read_varint()? as usize;
        if self.remaining() < len {
            return Err(ProtocolError::MalformedPacket("truncated properties"));
        }
        let end = self.pos + len;
        let mut topic_alias = None;
        while self.pos < end {
            let identifier = self.read_varint()?;
            match identifier {
                PROP_PAYLOAD_FORMAT_INDICATOR
                | PROP_REQUEST_PROBLEM_INFORMATION
                | PROP_REQUEST_RESPONSE_INFORMATION
                | PROP_MAXIMUM_QOS
                | PROP_RETAIN_AVAILABLE
                | PROP_WILDCARD_SUB_AVAILABLE
                | PROP_SUBSCRIPTION_ID_AVAILABLE
                | PROP_SHARED_SUB_AVAILABLE => {
                    self.read_u8()?;
                }
                PROP_SERVER_KEEP_ALIVE | PROP_RECEIVE_MAXIMUM | PROP_TOPIC_ALIAS_MAXIMUM => {
                    self.read_u16()?;
                }
                PROP_TOPIC_ALIAS => {
                    topic_alias = Some(self.read_u16()?);
                }
                PROP_MESSAGE_EXPIRY_INTERVAL
                | PROP_SESSION_EXPIRY_INTERVAL
                | PROP_WILL_DELAY_INTERVAL
                | PROP_MAXIMUM_PACKET_SIZE => {
                    self.read_bytes(4)?;
                }
                PROP_SUBSCRIPTION_IDENTIFIER => {
                    self.read_varint()?;
                }
                PROP_CONTENT_TYPE
                | PROP_RESPONSE_TOPIC
                | PROP_ASSIGNED_CLIENT_IDENTIFIER
                | PROP_AUTHENTICATION_METHOD
                | PROP_RESPONSE_INFORMATION
                | PROP_SERVER_REFERENCE
                | PROP_REASON_STRING => {
                    let skip = self.read_u16()? as usize;
                    self.read_bytes(skip)?;
                }
                PROP_CORRELATION_DATA | PROP_AUTHENTICATION_DATA => {
                    let skip = self.read_u16()? as usize;
                    self.read_bytes(skip)?;
                }
                PROP_USER_PROPERTY => {
                    let name_len = self.read_u16()? as usize;
                    self.read_bytes(name_len)?;
                    let value_len = self.read_u16()? as usize;
                    self.read_bytes(value_len)?;
                }
                _ => return Err(ProtocolError::MalformedPacket("unknown property")),
            }
        }
        if self.pos != end {
            return Err(ProtocolError::MalformedPacket("property length mismatch"));
        }
        Ok(topic_alias)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn encodes_remaining_length_boundaries() {
        assert_eq!(&encode_frame(CMD_PINGREQ, 0, &[]), &[0xC0, 0x00]);
        assert_eq!(
            &encode_frame(CMD_PUBLISH, 0, &[0; 128])[0..3],
            &[0x30, 0x80, 0x01]
        );
    }

    #[test]
    fn decodes_mqtt_v5_connect() {
        let body = [
            0, 4, b'M', b'Q', b'T', b'T', 5, 2, 0, 60, 0, 0, 6, b'c', b'l', b'i', b'e', b'n', b't',
        ];
        let packet = decode_frame(
            &Frame {
                command: CMD_CONNECT,
                flags: 0,
                body: body.to_vec(),
            },
            None,
        )
        .unwrap();
        match packet {
            MqttPacket::Connect {
                protocol,
                client_id,
                clean_start,
                ..
            } => {
                assert_eq!(protocol, ProtocolVersion::V5);
                assert_eq!(client_id, "client");
                assert!(clean_start);
            }
            other => panic!("unexpected packet: {other:?}"),
        }
    }

    #[test]
    fn decodes_topic_alias_property() {
        let body = [
            0, 1, b'a', 3, 0x23, 0, 7, b'p', b'a', b'y', b'l', b'o', b'a', b'd',
        ];
        let packet = decode_frame(
            &Frame {
                command: CMD_PUBLISH,
                flags: 0,
                body: body.to_vec(),
            },
            Some(ProtocolVersion::V5),
        )
        .unwrap();
        match packet {
            MqttPacket::Publish(publication) => {
                assert_eq!(publication.topic, "a");
                assert_eq!(publication.topic_alias, Some(7));
                assert_eq!(publication.payload, b"payload");
            }
            other => panic!("unexpected packet: {other:?}"),
        }
    }
}
