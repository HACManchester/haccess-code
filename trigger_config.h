

extern class input_trigger *in_rfid;
extern class input_trigger *in_rfid_auth;

extern class input_trigger *in_button1;
extern class input_trigger *in_button2;
extern class input_trigger *in_opto;

extern class output_trigger *out_opto;

struct mqtt_trigger_data {
  char *topic;
};

extern void setup_triggers(void);

