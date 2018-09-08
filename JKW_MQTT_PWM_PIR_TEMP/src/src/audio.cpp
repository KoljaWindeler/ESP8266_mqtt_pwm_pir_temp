#include <audio.h>

// simply the constructor
audio::audio(){
	buffer8b = NULL;
	sprintf((char*)key,"AUD");
};

// simply the destructor
audio::~audio(){
	if(buffer8b){
		delete [] buffer8b;
		buffer8b = NULL;
	}
	logger.println(TOPIC_GENERIC_INFO, F("audio deleted"), COLOR_YELLOW);
};

// helper function that is really just calling another function .. somewhat useless at the moment
// it is in charge of deciding if the object is loaded or not, but as of now it is just formwarding the
// capability parse response. but you can override it, e.g. to return "true" everytime and your component
// will be loaded under all circumstances
bool audio::parse(uint8_t* config){
	return cap.parse(config,get_key());
}

// the will be requested to check if the key is in the config strim
uint8_t* audio::get_key(){
	return key;
}

// will be callen if the key is part of the config
bool audio::init(){
	/*
  timer0_isr_init();
  timer0_attachInterrupt(statusLED_ISR);
  timer0_write(ESP.getCycleCount() + 160000000);
  interrupts();
	*/

  i2s_begin();
  i2s_set_rate(96000);      // 33 ksps

  pinMode(AMP_ENABLE_PIN, OUTPUT);
  digitalWrite(AMP_ENABLE_PIN, LOW);

	server = new WiFiServer(AUDIO_PORT);
	server->begin();
	buffer8b = new uint8_t[BUFFER_SIZE];
	logger.println(TOPIC_GENERIC_INFO, F("Audio init"), COLOR_GREEN);
	if(buffer8b){
		return true;
	}
	return false;
}

// return how many value you want to publish per minute
// e.g. DHT22: Humidity + Temp = 2
uint8_t audio::count_intervall_update(){
	return 0;
}

// will be called in loop, if you return true here, every else will be skipped !!
// so you CAN run uninterrupted by returning true, but you shouldn't do that for
// a long time, otherwise nothing else will be executed
bool audio::loop(){
	WiFiClient client = server->available();
  // new client?
  if (client)
  {
    startStreaming(&client);
    client.stop();
		return true; // i did nothing
  }
	return false; // i did nothing
}

// will be callen as often as count_intervall_update() returned, "slot" will help
// you to identify if its the first / call or whatever
// slots are per unit, so you will receive 0,1,2,3 ...
// return is ignored
bool audio::intervall_update(uint8_t slot){
	/*if(slot%count_intervall_update()==0){
		logger.print(TOPIC_MQTT_PUBLISH, F(""));
		dtostrf(audio.getSomething(), 3, 2, m_msg_buffer);
		logger.p(F("audio "));
		logger.pln(m_msg_buffer);
		return network.publish(build_topic(MQTT_audio_TOPIC,UNIT_TO_PC), m_msg_buffer, true);
	}*/
	return false;
}

// will be called everytime the controller reconnects to the MQTT broker,
// this is the chance to fire some subsctions
// return is ignored
bool audio::subscribe(){
	//network.subscribe(build_topic(MQTT_audio_TOPIC,PC_TO_UNIT)); // simple rainbow  topic
	//logger.println(TOPIC_MQTT_SUBSCIBED, build_topic(MQTT_audio_TOPIC,PC_TO_UNIT), COLOR_GREEN);
	return true;
}

// will be called everytime a MQTT message is received, if it is for you, return true. else other will be asked.
bool audio::receive(uint8_t* p_topic, uint8_t* p_payload){
	/*if (!strcmp((const char *) p_topic, build_topic(MQTT_audio_TOPIC,PC_TO_UNIT))) { // on / off with dimming
		logger.println(TOPIC_MQTT, F("received audio command"),COLOR_PURPLE);
		// do something
		return true;
	}*/
	return false; // not for me
}

// if you have something very urgent, do this in this method and return true
// will be checked on every main loop, so make sure you don't do this to often
bool audio::publish(){
	return false;
}

// **************************************************
// **************************************************
// **************************************************
// ctrl = 0 : reset the PWM
// ctrl = 1 : normal playback
// ctrl = 2 : direct 8-bit value output
//
inline void audio::doPWM(uint8_t ctrl, uint8_t value8b)
{
  static uint8_t dly = 0;
  static uint8_t sample = 0;
  static uint8_t dWordNr = 0;
  static uint32_t i2sbuf32b[4];
  static uint8_t prng;
  uint8_t shift;

  // reset
  if (ctrl == PWM_RESET)
  {
    dWordNr = 0;
    sample = 0;
    i2sbuf32b[0] = 0;
    i2sbuf32b[1] = 0;
    i2sbuf32b[2] = 0;
    i2sbuf32b[3] = 0;
    return;
  }

  // playback
  if (i2s_write_sample_nb(i2sbuf32b[dWordNr]))
  {
    dWordNr = (dWordNr + 1) & 0x03;

    // previous 4 DWORDS written?
    if (dWordNr == 0x00)
    {
      yield();

      // normal playback
      if (ctrl == PWM_NORMAL)
      {
        // new data in ring-buffer?
        if (bufferPtrOut != bufferPtrIn)
        {
          sample = buffer8b[bufferPtrOut];
          bufferPtrOut = (bufferPtrOut + 1) & (BUFFER_SIZE - 1);
        }
      } else if (ctrl == PWM_DIRECT)
      {
        // direct output mode
        sample = value8b;
      }

      shift = sample >> 1;

      if (!(sample & 0x01))
      {
        shift -= (prng & 0x01);     // subtract 0 or 1 from "shift" for dithering
        prng ^= 0x01;
      }

      yield();

      shift = 0x80 - shift;     // inverse shift

      if (shift < 0x20)
      {
        i2sbuf32b[0] = 0xFFFFFFFF >> shift;
        i2sbuf32b[1] = 0xFFFFFFFF;
        i2sbuf32b[2] = 0xFFFFFFFF;
        i2sbuf32b[3] = 0xFFFFFFFF;
      } else if (shift < 0x40)
      {
        i2sbuf32b[0] = 0x00000000;
        i2sbuf32b[1] = 0xFFFFFFFF >> (shift - 0x20);
        i2sbuf32b[2] = 0xFFFFFFFF;
        i2sbuf32b[3] = 0xFFFFFFFF;
      } else if (shift < 0x60)
      {
        i2sbuf32b[0] = 0x00000000;
        i2sbuf32b[1] = 0x00000000;
        i2sbuf32b[2] = 0xFFFFFFFF >> (shift - 0x40);
        i2sbuf32b[3] = 0xFFFFFFFF;
      } else if (shift < 0x80)
      {
        i2sbuf32b[0] = 0x00000000;
        i2sbuf32b[1] = 0x00000000;
        i2sbuf32b[2] = 0x00000000;
        i2sbuf32b[3] = 0xFFFFFFFF >> (shift - 0x60);
      } else {
        i2sbuf32b[0] = 0x00000000;
        i2sbuf32b[1] = 0x00000000;
        i2sbuf32b[2] = 0x00000000;
        i2sbuf32b[3] = 0x00000000;
      }
    }
  }
}

// **************************************************
// **************************************************
// **************************************************
void audio::rampPWM(uint8_t direction)
{
  uint8_t dl = 0;

  for (uint8_t value = 0; value < 0x80; value++)
  {
    while (dl++ < 250)
    {
      if (direction == UP)
        doPWM(PWM_DIRECT, value);
      else
        doPWM(PWM_DIRECT, 0x80 - value);
    }
    dl = 0;
  }
}

// **************************************************
// **************************************************
// **************************************************
inline void audio::startStreaming(WiFiClient *client)
{
  uint8_t i;
  uint8_t dl;

  bufferPtrIn = 0;
  bufferPtrOut = 0;

  digitalWrite(AMP_ENABLE_PIN, HIGH);

  // ===================================================================================
  // fill buffer

  ultimeout = millis() + 500;
  do
  {
    // yield();

    if (client->available())
    {
      buffer8b[bufferPtrIn] = client->read();
      bufferPtrIn = (bufferPtrIn + 1) & (BUFFER_SIZE - 1);
      ultimeout = millis() + 500;
    }
  } while ((bufferPtrIn < (BUFFER_SIZE - 1)) && (client->connected()) && (millis() < ultimeout));

  if ((!client->connected()) || (millis() >= ultimeout))
    return;

  // ===================================================================================
  // ramp-up PWM to 50% (=Vspeaker/2) to avoid "blops"

  rampPWM(UP);

  // ===================================================================================
  // start playback

  ultimeout = millis() + 500;
  do
  {
    doPWM(PWM_NORMAL,0);

    // new data in wifi rx-buffer?
    if (client->available())
    {
      // ring-buffer free?
      if (((bufferPtrIn + 1) & (BUFFER_SIZE - 1)) != bufferPtrOut)
      {
        buffer8b[bufferPtrIn] = client->read();
        bufferPtrIn = (bufferPtrIn + 1) & (BUFFER_SIZE - 1);
      }
    }

    if (bufferPtrOut != bufferPtrIn)
      ultimeout = millis() + 500;

  } while (client->available() || (millis() < ultimeout) || (bufferPtrOut != bufferPtrIn));

  // disabling the amplifier does not "plop", so it's better to do it before ramping down
  digitalWrite(AMP_ENABLE_PIN, LOW);

  // ===================================================================================
  // ramp-down PWM to 0% (=0Vdc) to avoid "blops"
  rampPWM(DOWN);
}
