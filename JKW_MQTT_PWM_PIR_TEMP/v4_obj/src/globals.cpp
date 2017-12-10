#include "main.h"


peripheral *p_adc;
peripheral *p_animaton;
peripheral *p_bOne;
peripheral *p_button;
peripheral *p_dimmer;
peripheral *p_neo;
peripheral *p_pir;
peripheral *p_pir2;
peripheral *p_pwm;
peripheral *p_pwm2;
peripheral *p_pwm3;
peripheral *p_rssi;
peripheral *p_simple_light;
peripheral *p_dht;
peripheral *p_ds;
peripheral *p_ai;
peripheral *p_light;

WiFiClient wifiClient;
WiFiManager wifiManager;
PubSubClient client(wifiClient);
mqtt_data mqtt;
capability cap;
logging logger;

char m_topic_buffer[TOPIC_BUFFER_SIZE];
char m_msg_buffer[MSG_BUFFER_SIZE];

// converts from half log to linear .. the human eye is a smartass
const uint8_t intens[100] =
{ 0,     1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
	 23,   24,  25,  26,  27,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  38,  39,  40,  41,  43,  44,  45,  47,
	 48,   50,  51,  53,  55,  57,  58,  60,  62,  64,  66,  68,  70,  73,  75,  77,  80,  82,  85,  88,  91,  93,  96,
	 99,  103, 106, 109, 113, 116, 120, 124, 128, 132, 136, 140, 145, 150, 154, 159, 164, 170, 175, 181, 186, 192, 198,
	 205, 211, 218, 225, 232, 239, 247, 255 };
