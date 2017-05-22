/* mqtt wrapper, for test/use */

#ifdef __TEST

static inline void mqtt_publish(const char *topic, const char *value)
{
  printf("mqtt: publish topic '%s' value '%s'\n", topic, value);
}
#else

#include <PubSubClient.h>

extern PubSubClient mqtt;

static inline void mqtt_publish(const char *topic, const char *value)
{
  if (mqtt.connected())
    mqtt.publish(topic, value);
}
#endif
