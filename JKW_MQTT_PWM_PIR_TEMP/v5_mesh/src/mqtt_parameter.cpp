#include "mqtt_parameter.h"

mqtt_parameter_8::mqtt_parameter_8(){
	_value = false;
	_update_required = false;
};
void mqtt_parameter_8::check_set(uint8_t input){
	if(input != _value){
		set(input);
	}
}
void mqtt_parameter_8::set(uint8_t input){
	set(input,true);
}
void mqtt_parameter_8::set(uint8_t input, bool update){
	_value = input;
	_update_required = update;
}
uint8_t mqtt_parameter_8::get_value(){
	return _value;
}
void mqtt_parameter_8::outdated(){
	outdated(true);
}
void mqtt_parameter_8::outdated(bool update_required){
	_update_required = update_required;
}
bool mqtt_parameter_8::get_outdated(){
	return _update_required;
}


mqtt_parameter_16::mqtt_parameter_16(){
	_value = false;
	_update_required = false;
};
void mqtt_parameter_16::check_set(uint16_t input){
	if(input != _value){
		set(input);
	}
}
void mqtt_parameter_16::set(uint16_t input){
	set(input,true);
}
void mqtt_parameter_16::set(uint16_t input, bool update){
	_value = input;
	_update_required = update;
}
uint16_t mqtt_parameter_16::get_value(){
	return _value;
}
void mqtt_parameter_16::outdated(){
	outdated(true);
}
void mqtt_parameter_16::outdated(bool update_required){
	_update_required = update_required;
}
bool mqtt_parameter_16::get_outdated(){
	return _update_required;
}


mqtt_parameter_32::mqtt_parameter_32(){
	_value = false;
	_update_required = false;
};
void mqtt_parameter_32::check_set(uint32_t input){
	if(input != _value){
		set(input);
	}
}
void mqtt_parameter_32::set(uint32_t input){
	set(input,true);
}
void mqtt_parameter_32::set(uint32_t input, bool update){
	_value = input;
	_update_required = update;
}
uint32_t mqtt_parameter_32::get_value(){
	return _value;
}
void mqtt_parameter_32::outdated(){
	outdated(true);
}
void mqtt_parameter_32::outdated(bool update_required){
	_update_required = update_required;
}
bool mqtt_parameter_32::get_outdated(){
	return _update_required;
}
