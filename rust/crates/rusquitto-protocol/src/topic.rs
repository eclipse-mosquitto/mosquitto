const TOPIC_HIERARCHY_LIMIT: usize = 200;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TopicError {
    Empty,
    TooLong,
    TooDeep,
    WildcardInPublish,
    InvalidWildcard,
}

pub fn check_publish_topic(topic: &str) -> Result<(), TopicError> {
    if topic.is_empty() {
        return Err(TopicError::Empty);
    }
    if topic.len() > 65_535 {
        return Err(TopicError::TooLong);
    }
    if topic.bytes().any(|b| b == b'+' || b == b'#') {
        return Err(TopicError::WildcardInPublish);
    }
    if topic.bytes().filter(|b| *b == b'/').count() > TOPIC_HIERARCHY_LIMIT {
        return Err(TopicError::TooDeep);
    }
    Ok(())
}

pub fn check_subscribe_topic(filter: &str) -> Result<(), TopicError> {
    if filter.is_empty() {
        return Err(TopicError::Empty);
    }
    if filter.len() > 65_535 {
        return Err(TopicError::TooLong);
    }
    let bytes = filter.as_bytes();
    for (idx, byte) in bytes.iter().copied().enumerate() {
        match byte {
            b'+' => {
                if idx > 0 && bytes[idx - 1] != b'/' {
                    return Err(TopicError::InvalidWildcard);
                }
                if idx + 1 < bytes.len() && bytes[idx + 1] != b'/' {
                    return Err(TopicError::InvalidWildcard);
                }
            }
            b'#' => {
                if idx > 0 && bytes[idx - 1] != b'/' {
                    return Err(TopicError::InvalidWildcard);
                }
                if idx + 1 != bytes.len() {
                    return Err(TopicError::InvalidWildcard);
                }
            }
            _ => {}
        }
    }
    if bytes.iter().filter(|b| **b == b'/').count() > TOPIC_HIERARCHY_LIMIT {
        return Err(TopicError::TooDeep);
    }
    Ok(())
}

pub fn matches(filter: &str, topic: &str) -> bool {
    if filter.is_empty() || topic.is_empty() {
        return false;
    }
    if filter.starts_with('$') != topic.starts_with('$') {
        return false;
    }

    let mut filter_levels = filter.split('/').peekable();
    let mut topic_levels = topic.split('/').peekable();

    while let Some(filter_level) = filter_levels.next() {
        match filter_level {
            "#" => return filter_levels.peek().is_none(),
            "+" => {
                if topic_levels.next().is_none() {
                    return false;
                }
            }
            literal => {
                if Some(literal) != topic_levels.next() {
                    return false;
                }
            }
        }
    }

    topic_levels.next().is_none()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn validates_publish_topics() {
        assert_eq!(check_publish_topic("a/b"), Ok(()));
        assert_eq!(
            check_publish_topic("a/+/b"),
            Err(TopicError::WildcardInPublish)
        );
        assert_eq!(check_publish_topic(""), Err(TopicError::Empty));
        assert_eq!(
            check_publish_topic(&"/".repeat(201)),
            Err(TopicError::TooDeep)
        );
    }

    #[test]
    fn validates_subscribe_topics() {
        assert_eq!(check_subscribe_topic("a/+/b"), Ok(()));
        assert_eq!(check_subscribe_topic("a/#"), Ok(()));
        assert_eq!(
            check_subscribe_topic("a#"),
            Err(TopicError::InvalidWildcard)
        );
        assert_eq!(
            check_subscribe_topic("a/#/b"),
            Err(TopicError::InvalidWildcard)
        );
        assert_eq!(check_subscribe_topic("+/b"), Ok(()));
    }

    #[test]
    fn matches_mqtt_filters() {
        assert!(matches("sensors/+/temp", "sensors/room/temp"));
        assert!(matches("sensors/#", "sensors/room/temp"));
        assert!(matches("/finance", "/finance"));
        assert!(!matches("sensors/+/temp", "sensors/room/humidity"));
        assert!(!matches("#", "$SYS/broker/uptime"));
    }
}
