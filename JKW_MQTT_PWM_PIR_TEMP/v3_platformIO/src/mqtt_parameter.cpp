#include "mqtt_parameter.h"

mqtt_parameter_8::mqtt_parameter_8(){
	value = false;
	update_required = false;
};
void mqtt_parameter_8::check_set(uint8_t input){
	if(input != value){
		set(input);
	}
}
void mqtt_parameter_8::set(uint8_t input){
	value = input;
	update_required = true;
}




mqtt_parameter_16::mqtt_parameter_16(){
	value = false;
	update_required = false;
};
void mqtt_parameter_16::check_set(uint16_t input){
	if(input != value){
		set(input);
	}
}
void mqtt_parameter_16::set(uint16_t input){
	value = input;
	update_required = true;
}



mqtt_parameter_32::mqtt_parameter_32(){
	value = false;
	update_required = false;
};
void mqtt_parameter_32::check_set(uint32_t input){
	if(input != value){
		set(input);
	}
}
void mqtt_parameter_32::set(uint32_t input){
	value = input;
	update_required = true;
}
