#include "main.h"


capability *p_adc;
capability *p_bOne;
capability *p_button;
capability *p_neo;
capability *p_pir;
capability *p_pwm;
capability *p_pwm2;
capability *p_pwm3;
capability *p_shellyDimmer;
capability *p_ads1015;
capability *p_rssi;
capability *p_simple_light;
capability *p_remote_simple_light;
capability *p_dht;
capability *p_ds;
capability *p_ai;
capability *p_light;
capability *p_hlw;
capability *p_nl;
capability *p_ir;
capability *p_rfb;
capability *p_gpio;
//capability *p_husqvarna;
capability *p_no_mesh;
capability *p_uptime;
capability *p_play;
capability *p_freq;
capability *p_rec;
capability *p_em;
capability *p_em_bin;
capability *p_SerSer;
capability *p_hx711;
capability *p_crash;
capability *p_count;

connection_relay network;
WiFiClient wifiClient;
WiFiManager wifiManager;
PubSubClient client(wifiClient);
mqtt_data mqtt;
capability_parser cap;
logging logger;
uint32_t timer_connected_start;

char m_topic_buffer[TOPIC_BUFFER_SIZE];
char m_msg_buffer[MSG_BUFFER_SIZE];

// converts from half log to linear .. the human eye is a smartass
const uint8_t intens[100] =
{ 0,     1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
	 23,   24,  25,  26,  27,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  38,  39,  40,  41,  43,  44,  45,  47,
	 48,   50,  51,  53,  55,  57,  58,  60,  62,  64,  66,  68,  70,  73,  75,  77,  80,  82,  85,  88,  91,  93,  96,
	 99,  103, 106, 109, 113, 116, 120, 124, 128, 132, 136, 140, 145, 150, 154, 159, 164, 170, 175, 181, 186, 192, 198,
	 205, 211, 218, 225, 232, 239, 247, 255 };
