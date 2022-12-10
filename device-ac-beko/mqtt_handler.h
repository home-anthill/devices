extern PubSubClient mqtt_client;

void mqtt_init(Client& wifi_client, std::function<void (char *, uint8_t *, unsigned int)> callback);

void mqtt_connect(String uuid);