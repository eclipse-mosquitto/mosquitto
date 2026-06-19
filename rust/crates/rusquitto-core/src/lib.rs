use std::collections::HashMap;

use rusquitto_protocol::topic;
use rusquitto_protocol::{Publication, SubscriptionRequest, Will};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Subscription {
    pub filter: String,
    pub qos: u8,
    pub no_local: bool,
    pub retain_as_published: bool,
    pub retain_handling: u8,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ClientSession {
    pub client_id: String,
    pub subscriptions: HashMap<String, Subscription>,
    pub will: Option<Will>,
    pub online: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Delivery {
    pub client_id: String,
    pub publication: Publication,
}

#[derive(Debug, Clone, Default)]
pub struct BrokerState {
    clients: HashMap<String, ClientSession>,
    retained: HashMap<String, Publication>,
    next_outgoing_mid: u16,
}

impl BrokerState {
    pub fn new() -> Self {
        Self {
            next_outgoing_mid: 1,
            ..Self::default()
        }
    }

    pub fn connect(
        &mut self,
        client_id: String,
        clean_start: bool,
        will: Option<Will>,
    ) -> ConnectResult {
        let session_present = !clean_start && self.clients.contains_key(&client_id);
        if clean_start || !self.clients.contains_key(&client_id) {
            self.clients.insert(
                client_id.clone(),
                ClientSession {
                    client_id: client_id.clone(),
                    subscriptions: HashMap::new(),
                    will,
                    online: true,
                },
            );
        } else if let Some(session) = self.clients.get_mut(&client_id) {
            session.will = will;
            session.online = true;
        }
        ConnectResult { session_present }
    }

    pub fn disconnect(&mut self, client_id: &str, graceful: bool) -> Vec<Delivery> {
        let will = self.clients.get_mut(client_id).and_then(|session| {
            session.online = false;
            if graceful {
                session.will = None;
                None
            } else {
                session.will.take()
            }
        });

        if let Some(will) = will {
            let publication = Publication {
                topic: will.topic,
                payload: will.payload,
                qos: will.qos,
                retain: will.retain,
                packet_id: None,
                dup: false,
                topic_alias: None,
            };
            self.publish(client_id, publication).deliveries
        } else {
            Vec::new()
        }
    }

    pub fn subscribe(
        &mut self,
        client_id: &str,
        filters: Vec<SubscriptionRequest>,
    ) -> SubscribeResult {
        let mut reason_codes = Vec::with_capacity(filters.len());
        let mut retained = Vec::new();
        let mut replay_filters = Vec::new();

        let Some(session) = self.clients.get_mut(client_id) else {
            return SubscribeResult {
                reason_codes,
                retained,
            };
        };

        for request in filters {
            let valid = topic::check_subscribe_topic(&request.filter).is_ok() && request.qos <= 2;
            if !valid {
                reason_codes.push(0x80);
                continue;
            }

            let existed = session.subscriptions.contains_key(&request.filter);
            let subscription = Subscription {
                filter: request.filter.clone(),
                qos: request.qos,
                no_local: request.no_local,
                retain_as_published: request.retain_as_published,
                retain_handling: request.retain_handling,
            };
            session
                .subscriptions
                .insert(request.filter.clone(), subscription.clone());
            reason_codes.push(request.qos);

            let send_retained = match request.retain_handling {
                0 => true,
                1 => !existed,
                _ => false,
            };
            if send_retained {
                replay_filters.push(subscription);
            }
        }

        for subscription in replay_filters {
            let matching: Vec<Publication> = self
                .retained
                .values()
                .filter(|publication| topic::matches(&subscription.filter, &publication.topic))
                .cloned()
                .collect();

            for publication in matching {
                let mut retained_publication = publication.clone();
                retained_publication.qos = retained_publication.qos.min(subscription.qos);
                retained_publication.retain =
                    !subscription.retain_as_published || publication.retain;
                if retained_publication.qos > 0 {
                    retained_publication.packet_id = Some(self.next_packet_id());
                }
                retained.push(Delivery {
                    client_id: client_id.to_owned(),
                    publication: retained_publication,
                });
            }
        }

        SubscribeResult {
            reason_codes,
            retained,
        }
    }

    pub fn unsubscribe(&mut self, client_id: &str, filters: &[String]) {
        if let Some(session) = self.clients.get_mut(client_id) {
            for filter in filters {
                session.subscriptions.remove(filter);
            }
        }
    }

    pub fn publish(
        &mut self,
        source_client_id: &str,
        mut publication: Publication,
    ) -> PublishResult {
        let mut accepted = topic::check_publish_topic(&publication.topic).is_ok();
        if publication.qos > 2 {
            accepted = false;
        }
        if !accepted {
            return PublishResult {
                accepted: false,
                deliveries: Vec::new(),
            };
        }

        if publication.retain {
            if publication.payload.is_empty() {
                self.retained.remove(&publication.topic);
            } else {
                let mut retained = publication.clone();
                retained.packet_id = None;
                self.retained.insert(publication.topic.clone(), retained);
            }
        }

        let mut deliveries = Vec::new();
        let mut delivery_specs = Vec::new();
        for (client_id, session) in &self.clients {
            if !session.online {
                continue;
            }
            for subscription in session.subscriptions.values() {
                if subscription.no_local && client_id == source_client_id {
                    continue;
                }
                if topic::matches(&subscription.filter, &publication.topic) {
                    delivery_specs.push((
                        client_id.clone(),
                        subscription.qos.min(publication.qos),
                        subscription.retain_as_published,
                    ));
                    break;
                }
            }
        }

        for (client_id, qos, retain_as_published) in delivery_specs {
            publication.qos = qos;
            let mut outgoing = publication.clone();
            outgoing.retain = retain_as_published && outgoing.retain;
            outgoing.packet_id = (outgoing.qos > 0).then(|| self.next_packet_id());
            deliveries.push(Delivery {
                client_id,
                publication: outgoing,
            });
        }

        PublishResult {
            accepted: true,
            deliveries,
        }
    }

    fn next_packet_id(&mut self) -> u16 {
        let id = self.next_outgoing_mid;
        self.next_outgoing_mid = if self.next_outgoing_mid == u16::MAX {
            1
        } else {
            self.next_outgoing_mid + 1
        };
        id
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ConnectResult {
    pub session_present: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SubscribeResult {
    pub reason_codes: Vec<u8>,
    pub retained: Vec<Delivery>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct PublishResult {
    pub accepted: bool,
    pub deliveries: Vec<Delivery>,
}

#[cfg(test)]
mod tests {
    use super::*;

    fn publication(topic: &str, retain: bool) -> Publication {
        Publication {
            topic: topic.to_owned(),
            payload: b"payload".to_vec(),
            qos: 0,
            retain,
            packet_id: None,
            dup: false,
            topic_alias: None,
        }
    }

    #[test]
    fn routes_matching_subscriptions() {
        let mut broker = BrokerState::new();
        broker.connect("sub".into(), true, None);
        broker.subscribe(
            "sub",
            vec![SubscriptionRequest {
                filter: "a/+/c".into(),
                qos: 0,
                no_local: false,
                retain_as_published: false,
                retain_handling: 0,
            }],
        );
        let result = broker.publish("pub", publication("a/b/c", false));
        assert!(result.accepted);
        assert_eq!(result.deliveries.len(), 1);
        assert_eq!(result.deliveries[0].client_id, "sub");
    }

    #[test]
    fn retains_and_replays_messages() {
        let mut broker = BrokerState::new();
        broker.connect("pub".into(), true, None);
        broker.publish("pub", publication("retain/topic", true));
        broker.connect("sub".into(), true, None);
        let result = broker.subscribe(
            "sub",
            vec![SubscriptionRequest {
                filter: "retain/#".into(),
                qos: 0,
                no_local: false,
                retain_as_published: false,
                retain_handling: 0,
            }],
        );
        assert_eq!(result.reason_codes, vec![0]);
        assert_eq!(result.retained.len(), 1);
        assert!(result.retained[0].publication.retain);
    }

    #[test]
    fn publishes_will_on_ungraceful_disconnect() {
        let mut broker = BrokerState::new();
        broker.connect("sub".into(), true, None);
        broker.subscribe(
            "sub",
            vec![SubscriptionRequest {
                filter: "will/topic".into(),
                qos: 0,
                no_local: false,
                retain_as_published: false,
                retain_handling: 0,
            }],
        );
        broker.connect(
            "will".into(),
            true,
            Some(Will {
                topic: "will/topic".into(),
                payload: b"gone".to_vec(),
                qos: 0,
                retain: false,
            }),
        );
        let deliveries = broker.disconnect("will", false);
        assert_eq!(deliveries.len(), 1);
        assert_eq!(deliveries[0].publication.payload, b"gone");
    }
}
