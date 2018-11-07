#ifndef cap_h
#define cap_h

class capability
{
public:
	capability(){ }
	// prototyp available
	virtual bool reg_provider(capability * p, uint8_t *source){ return false; }; // don't need provider
	virtual uint8_t* get_dep(){ return (uint8_t*)""; }; // no dpenency
	virtual uint8_t count_intervall_update(){ return 0; }; // 0 regular updates
	virtual bool intervall_update(uint8_t slot){ return false; }; // no updates
	virtual bool publish(){ return false; }; // nothing send
	virtual bool loop(){ return false; }; // nothing done
	virtual bool subscribe() { return true; }; // subscription send (ignored)
	virtual bool receive(uint8_t * p_topic, uint8_t * p_payload){ return false; }; // not for me

	virtual ~capability(){ }
	virtual bool init() = 0;
	virtual bool parse(uint8_t * config) = 0;
	virtual uint8_t * get_key() = 0;
};

#endif
