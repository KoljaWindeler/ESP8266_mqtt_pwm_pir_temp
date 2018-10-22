#ifndef no_mesh_h
#define no_mesh_h

#include "main.h"

class no_mesh : public capability {
	public:
		no_mesh();
		~no_mesh();
		bool init();
		// bool loop();
		// bool intervall_update(uint8_t slot);
		// bool subscribe();
		bool parse(uint8_t* config);
		// bool receive(uint8_t* p_topic, uint8_t* p_payload);
		// bool publish();
		uint8_t* get_key();
	private:
		uint8_t key[3];
};


#endif
