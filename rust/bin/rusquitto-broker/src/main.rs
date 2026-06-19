use std::collections::HashMap;
use std::env;
use std::fs;
use std::io::{self, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::mpsc::{self, Sender};
use std::sync::{Arc, Mutex};
use std::thread;

use rusquitto_core::BrokerState;
use rusquitto_protocol::{
    decode_frame, encode_connack, encode_disconnect, encode_pingresp, encode_puback,
    encode_pubcomp, encode_publish, encode_pubrec, encode_suback, encode_unsuback, read_frame,
    MqttPacket, ProtocolVersion, Publication,
};

const MQTT_RC_MALFORMED_PACKET: u8 = 0x81;
const MQTT_RC_NOT_AUTHORIZED: u8 = 0x87;

type OutboundMap = Arc<Mutex<HashMap<String, Sender<Vec<u8>>>>>;
type SharedBroker = Arc<Mutex<BrokerState>>;

#[derive(Debug, Clone)]
struct Settings {
    port: u16,
    verbose: bool,
    allow_anonymous: bool,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            port: 1883,
            verbose: false,
            allow_anonymous: true,
        }
    }
}

fn main() {
    if let Err(err) = run() {
        eprintln!("Error: {err}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), String> {
    let settings = parse_settings(env::args().skip(1).collect())?;
    let listener = bind_listener(settings.port)?;
    if settings.verbose {
        eprintln!("rusquitto version 2.1.2 starting");
        eprintln!("Opening ipv4 listen socket on port {}.", settings.port);
    }

    let broker = Arc::new(Mutex::new(BrokerState::new()));
    let outbound = Arc::new(Mutex::new(HashMap::new()));
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let broker = Arc::clone(&broker);
                let outbound = Arc::clone(&outbound);
                let settings = settings.clone();
                thread::spawn(move || {
                    if let Err(err) = handle_client(stream, broker, outbound, settings) {
                        eprintln!("Client error: {err}");
                    }
                });
            }
            Err(err) => {
                eprintln!("Accept error: {err}");
            }
        }
    }
    Ok(())
}

fn bind_listener(port: u16) -> Result<TcpListener, String> {
    TcpListener::bind(("::", port))
        .or_else(|_| TcpListener::bind(("0.0.0.0", port)))
        .map_err(|e| e.to_string())
}

fn parse_settings(args: Vec<String>) -> Result<Settings, String> {
    let mut settings = Settings::default();
    let mut config_path = None;
    let mut cli_port = None;
    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "-h" | "--help" => {
                println!("mosquitto [-c config file] [-p port] [-v]");
                std::process::exit(0);
            }
            "-c" => {
                i += 1;
                config_path = args.get(i).cloned();
            }
            "-p" => {
                i += 1;
                let port = args
                    .get(i)
                    .ok_or_else(|| "-p requires a port".to_owned())?
                    .parse::<u16>()
                    .map_err(|_| "-p requires a numeric port".to_owned())?;
                cli_port = Some(port);
            }
            "-v" => settings.verbose = true,
            other => return Err(format!("unsupported option {other}")),
        }
        i += 1;
    }

    let mut config_declared_listener = false;
    let mut explicit_allow = None;
    let mut listener_allow = None;
    if let Some(path) = config_path {
        let contents =
            fs::read_to_string(&path).map_err(|e| format!("unable to read config {path}: {e}"))?;
        for raw_line in contents.lines() {
            let line = raw_line.split('#').next().unwrap_or("").trim();
            if line.is_empty() {
                continue;
            }
            let mut parts = line.split_whitespace();
            let key = parts.next().unwrap_or("");
            let value = parts.next();
            match key {
                "port" | "listener" => {
                    config_declared_listener = true;
                    if let Some(value) = value {
                        if let Ok(port) = value.parse::<u16>() {
                            settings.port = port;
                        }
                    }
                }
                "allow_anonymous" => {
                    explicit_allow = parse_bool(value);
                }
                "listener_allow_anonymous" => {
                    listener_allow = parse_bool(value);
                }
                _ => {}
            }
        }
    }

    if let Some(port) = cli_port {
        settings.port = port;
    }
    settings.allow_anonymous = if let Some(value) = listener_allow {
        value
    } else if let Some(value) = explicit_allow {
        value
    } else {
        !config_declared_listener
    };

    Ok(settings)
}

fn parse_bool(value: Option<&str>) -> Option<bool> {
    match value {
        Some("true") | Some("1") => Some(true),
        Some("false") | Some("0") => Some(false),
        _ => None,
    }
}

fn handle_client(
    mut stream: TcpStream,
    broker: SharedBroker,
    outbound: OutboundMap,
    settings: Settings,
) -> io::Result<()> {
    let first = match read_frame(&mut stream) {
        Ok(frame) => frame,
        Err(_) => return Ok(()),
    };
    let connect = match decode_frame(&first, None) {
        Ok(MqttPacket::Connect {
            protocol,
            clean_start,
            client_id,
            username,
            will,
            ..
        }) => (protocol, clean_start, client_id, username, will),
        _ => return Ok(()),
    };

    let (protocol, clean_start, mut client_id, username, will) = connect;
    if client_id.is_empty() {
        client_id = format!("auto-{}", unique_id());
    }
    if !settings.allow_anonymous && username.is_none() {
        let rc = if protocol == ProtocolVersion::V5 {
            MQTT_RC_NOT_AUTHORIZED
        } else {
            5
        };
        stream.write_all(&encode_connack(protocol, false, rc))?;
        return Ok(());
    }

    let session_present = broker
        .lock()
        .expect("broker lock poisoned")
        .connect(client_id.clone(), clean_start, will)
        .session_present;
    stream.write_all(&encode_connack(protocol, session_present, 0))?;

    let (tx, rx) = mpsc::channel::<Vec<u8>>();
    outbound
        .lock()
        .expect("outbound lock poisoned")
        .insert(client_id.clone(), tx.clone());

    let mut writer = stream.try_clone()?;
    thread::spawn(move || {
        while let Ok(packet) = rx.recv() {
            if writer.write_all(&packet).is_err() {
                break;
            }
        }
    });

    let mut topic_aliases: HashMap<u16, String> = HashMap::new();
    let mut graceful = false;
    while let Ok(frame) = read_frame(&mut stream) {
        let packet = match decode_frame(&frame, Some(protocol)) {
            Ok(packet) => packet,
            Err(_) => {
                if protocol == ProtocolVersion::V5 {
                    let _ = tx.send(encode_disconnect(protocol, MQTT_RC_MALFORMED_PACKET));
                }
                break;
            }
        };

        match packet {
            MqttPacket::Publish(mut publication) => {
                if !resolve_topic_alias(&mut publication, &mut topic_aliases) {
                    if protocol == ProtocolVersion::V5 {
                        let _ = tx.send(encode_disconnect(protocol, MQTT_RC_MALFORMED_PACKET));
                    }
                    break;
                }

                if publication.qos == 1 {
                    if let Some(packet_id) = publication.packet_id {
                        let _ = tx.send(encode_puback(protocol, packet_id));
                    }
                } else if publication.qos == 2 {
                    if let Some(packet_id) = publication.packet_id {
                        let _ = tx.send(encode_pubrec(protocol, packet_id));
                    }
                }

                let result = broker
                    .lock()
                    .expect("broker lock poisoned")
                    .publish(&client_id, publication);
                if !result.accepted {
                    if protocol == ProtocolVersion::V5 {
                        let _ = tx.send(encode_disconnect(protocol, MQTT_RC_MALFORMED_PACKET));
                    }
                    break;
                }
                send_deliveries(protocol, &outbound, result.deliveries);
            }
            MqttPacket::PubRel { packet_id } => {
                let _ = tx.send(encode_pubcomp(protocol, packet_id));
            }
            MqttPacket::Subscribe { packet_id, filters } => {
                let result = broker
                    .lock()
                    .expect("broker lock poisoned")
                    .subscribe(&client_id, filters);
                let _ = tx.send(encode_suback(protocol, packet_id, &result.reason_codes));
                send_deliveries(protocol, &outbound, result.retained);
            }
            MqttPacket::Unsubscribe { packet_id, filters } => {
                broker
                    .lock()
                    .expect("broker lock poisoned")
                    .unsubscribe(&client_id, &filters);
                let _ = tx.send(encode_unsuback(protocol, packet_id, filters.len()));
            }
            MqttPacket::PingReq => {
                let _ = tx.send(encode_pingresp());
            }
            MqttPacket::Disconnect { .. } => {
                graceful = true;
                break;
            }
            MqttPacket::PubAck { .. } | MqttPacket::PubRec { .. } | MqttPacket::PubComp { .. } => {}
            MqttPacket::Connect { .. } => break,
        }
    }

    outbound
        .lock()
        .expect("outbound lock poisoned")
        .remove(&client_id);
    let deliveries = broker
        .lock()
        .expect("broker lock poisoned")
        .disconnect(&client_id, graceful);
    send_deliveries(protocol, &outbound, deliveries);
    Ok(())
}

fn resolve_topic_alias(
    publication: &mut Publication,
    topic_aliases: &mut HashMap<u16, String>,
) -> bool {
    if let Some(alias) = publication.topic_alias {
        if !publication.topic.is_empty() {
            topic_aliases.insert(alias, publication.topic.clone());
            true
        } else if let Some(topic) = topic_aliases.get(&alias) {
            publication.topic.clone_from(topic);
            true
        } else {
            false
        }
    } else {
        !publication.topic.is_empty()
    }
}

fn send_deliveries(
    protocol: ProtocolVersion,
    outbound: &OutboundMap,
    deliveries: Vec<rusquitto_core::Delivery>,
) {
    let map = outbound.lock().expect("outbound lock poisoned");
    for delivery in deliveries {
        if let Some(sender) = map.get(&delivery.client_id) {
            let _ = sender.send(encode_publish(protocol, &delivery.publication));
        }
    }
}

fn unique_id() -> u128 {
    use std::time::{SystemTime, UNIX_EPOCH};
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|duration| duration.as_nanos())
        .unwrap_or(0)
}
