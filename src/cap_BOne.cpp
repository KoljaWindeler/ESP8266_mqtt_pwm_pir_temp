#include <cap_BOne.h>
#ifdef WITH_BONE

BOne::BOne(){
		m_discovery_pub = false;
};

BOne::~BOne(){
#ifdef WITH_DISCOVERY
	if(m_discovery_pub & (timer_connected_start>0)){
		char* t = discovery_topic_bake(MQTT_DISCOVERY_DIMM_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
		logger.print(TOPIC_MQTT_PUBLISH, F("Erasing Bone config "), COLOR_YELLOW);
		logger.pln(t);
		network.publish(t,(char*)"");
		m_discovery_pub = false;
		delete[] t;
	}
#endif
	logger.println(TOPIC_GENERIC_INFO, F("B1 deleted"), COLOR_YELLOW);
};

bool BOne::parse(uint8_t* config){
	return cap.parse(config,get_key(),get_dep());
}

bool BOne::init(){
	_my92x1_b1.init(true,MY9291_DI_PIN, MY9291_DCKI_PIN,6); // true = B1
	logger.println(TOPIC_GENERIC_INFO, F("B1 init"), COLOR_GREEN);
	return true;
}

uint8_t* BOne::get_dep(){
	return (uint8_t*)"LIG";
}

uint8_t* BOne::get_key(){
	return (uint8_t*)"BONE";
}

my92x1* BOne::getmy929x1(){
	return &_my92x1_b1;
}

// bool BOne::loop(){
// 	return false; // i did nothing
// }
//
// bool BOne::intervall_update(uint8_t slot){
// 	return false;
// }

uint8_t BOne::getState(led* color){
	color->r = m_light_current.r;
	color->g = m_light_current.g;
	color->b = m_light_current.b;
	return m_state.get_value();
}

uint8_t BOne::get_modes(){
	return (1<<SUPPORTS_PWM) | (1<<SUPPORTS_RGB); 
};

void BOne::print_name(){
	logger.pln(F("B1 bulb"));
};


void BOne::set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t px){
	uint8_t w=0;
	if(r==b && b==g){
		w=r;
		r=0;
		g=0;
		b=0;
	}
	_my92x1_b1.setColor((my92x1_color_t) { r, g, b, w, 0 });          // last two: warm white, cold white
}

// bool BOne::subscribe(){
// 	return true;
// }
//
//
// bool BOne::receive(uint8_t* p_topic, uint8_t* p_payload){
// 	return false; // not for me
// }
//
//
bool BOne::publish(){
#ifdef WITH_DISCOVERY
	if(!m_discovery_pub){
		if(millis()-timer_connected_start>NETWORK_SUBSCRIPTION_DELAY){
			char* t = discovery_topic_bake(MQTT_DISCOVERY_DIMM_TOPIC,mqtt.dev_short); // don't forget to "delete[] t;" at the end of usage;
			char* m = new char[strlen(MQTT_DISCOVERY_DIMM_MSG)+5*strlen(mqtt.dev_short)];
			sprintf(m, MQTT_DISCOVERY_DIMM_MSG, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short, mqtt.dev_short);
			logger.println(TOPIC_MQTT_PUBLISH, F("Bone discovery"), COLOR_GREEN);
			//logger.p(t);
			//logger.p(" -> ");
			//logger.pln(m);
			m_discovery_pub = network.publish(t,m);
			delete[] m;
			delete[] t;
			return true;
		}
	}
#endif
	return false; // i did notihgin
}
#endif
