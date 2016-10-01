/* mqtt wrapper, for test/use */

#ifdef __TEST

static inline void mqtt_publish(char *topic, char *value)
{
  printf("mqtt: publish topic '%s' value '%s'\n", topic, value);
}
#else

extern PubSubClient mqtt;

static inline void mqtt_publish(char *topic, char *value)
{
  mqtt.publish(topic, value);
}
#endif
