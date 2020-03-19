#ifndef ads1015_h
#define ads1015_h

#include "main.h"
#include <Wire.h>

#ifdef WITH_DISCOVERY
	static constexpr char MQTT_DISCOVERY_ADS_CHi_TOPIC[]      = "homeassistant/sensor/%s_ads%i/config";
	static constexpr char MQTT_DISCOVERY_ADS_CHi_MSG[]      = "{\"name\":\"%s_ads_ch%i\", \"stat_t\": \"%s/r/ads_a%i\"}";
#endif


static constexpr char MQTT_ADS1015_A_TOPIC[]   = "ads_a%i";

#define SAMPLE_COUNT 		200
#define ADS_DATA_RDY_PIN 	12 // D6 == gpio 12
#define ADS_SDA_PIN 		5 // SDA (D1=gpio5)
#define ADS_SCL_PIN 		13 // SCL (D7=gpio13)

#define ADS_DC_MODE 		1
#define ADS_AC_MODE 		2

#define ADS1015_ADDRESS (0x48) ///< 1001 000 (ADDR = GND)
#define ADS1115_CONVERSIONDELAY (8) ///< Conversion delay
#define ADS1015_REG_POINTER_MASK (0x03)      ///< Point mask
#define ADS1015_REG_POINTER_CONVERT (0x00)   ///< Conversion
#define ADS1015_REG_POINTER_CONFIG (0x01)    ///< Configuration
#define ADS1015_REG_POINTER_LOWTHRESH (0x02) ///< Low threshold
#define ADS1015_REG_POINTER_HITHRESH (0x03)  ///< High threshold
/*=========================================================================*/

/*=========================================================================
    CONFIG REGISTER
    -----------------------------------------------------------------------*/
#define ADS1015_REG_CONFIG_OS_SINGLE  (0x8000) ///< Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_OS_BUSY    (0x0000) ///< Read: Bit = 0 when conversion is in progress
#define ADS1015_REG_CONFIG_OS_NOTBUSY (0x8000) ///< Read: Bit = 1 when device is not performing a conversion

#define ADS1015_REG_CONFIG_MUX_DIFF_0_1 (0x0000) ///< Differential P = AIN0, N = AIN1 (default)
#define ADS1015_REG_CONFIG_MUX_DIFF_0_3 (0x1000) ///< Differential P = AIN0, N = AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_1_3 (0x2000) ///< Differential P = AIN1, N = AIN3
#define ADS1015_REG_CONFIG_MUX_DIFF_2_3 (0x3000) ///< Differential P = AIN2, N = AIN3
#define ADS1015_REG_CONFIG_MUX_SINGLE_0 (0x4000) ///< Single-ended AIN0
#define ADS1015_REG_CONFIG_MUX_SINGLE_1 (0x5000) ///< Single-ended AIN1
#define ADS1015_REG_CONFIG_MUX_SINGLE_2 (0x6000) ///< Single-ended AIN2
#define ADS1015_REG_CONFIG_MUX_SINGLE_3 (0x7000) ///< Single-ended AIN3

#define ADS1015_REG_CONFIG_PGA_0_256V (0x0A00) ///< +/-0.256V range = Gain 16
#define ADS1015_REG_CONFIG_PGA_0_512V (0x0800) ///< +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_1_024V (0x0600) ///< +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_2_048V (0x0400) ///< +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_4_096V (0x0200) ///< +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_6_144V (0x0000) ///< +/-6.144V range = Gain 2/3

#define ADS1015_REG_CONFIG_MODE_CONTIN (0x0000) ///< Continuous conversion mode
#define ADS1015_REG_CONFIG_MODE_SINGLE (0x0100) ///< Power-down single-shot mode (default)

#define ADS1015_REG_CONFIG_DR_128SPS_ADS1115_REG_CONFIG_DR_8SPS (0x0000) 	///< 0128/008 samples per second
#define ADS1015_REG_CONFIG_DR_250SPS_ADS1115_REG_CONFIG_DR_16SPS (0x0020) 	///< 0250/016 samples per second
#define ADS1015_REG_CONFIG_DR_490SPS_ADS1115_REG_CONFIG_DR_32SPS (0x0040)	///< 0490/032 samples per second
#define ADS1015_REG_CONFIG_DR_920SPS_ADS1115_REG_CONFIG_DR_64SPS (0x0060)	///< 0920/064 samples per second
#define ADS1015_REG_CONFIG_DR_1600SPS_ADS1115_REG_CONFIG_DR_128SPS (0x0080)	///< 1600/128 samples per second (default)
#define ADS1015_REG_CONFIG_DR_2400SPS_ADS1115_REG_CONFIG_DR_250SPS (0x00A0)	///< 2400/250 samples per second
#define ADS1015_REG_CONFIG_DR_3300SPS_ADS1115_REG_CONFIG_DR_475SPS (0x00C0)	///< 2400/475 samples per second
#define ADS1015_REG_CONFIG_DR_3300SPS_ADS1115_REG_CONFIG_DR_860SPS (0x00E0)	///< 3300/860 samples per second

#define ADS1015_REG_CONFIG_CMODE_TRAD (0x0000) ///< Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_CMODE_WINDOW (0x0010) ///< Window comparator

#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000) ///< ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVHI (0x0008) ///< ALERT/RDY pin is high when active

#define ADS1015_REG_CONFIG_CLAT_NONLAT (0x0000) ///< Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CLAT_LATCH (0x0004) ///< Latching comparator

#define ADS1015_REG_CONFIG_CQUE_1CONV (0x0000) ///< Assert ALERT/RDY after one conversions
#define ADS1015_REG_CONFIG_CQUE_2CONV (0x0001) ///< Assert ALERT/RDY after two conversions
#define ADS1015_REG_CONFIG_CQUE_4CONV (0x0002) ///< Assert ALERT/RDY after four conversions
#define ADS1015_REG_CONFIG_CQUE_NONE  (0x0003) ///< Disable the comparator and put ALERT/RDY in high state (default)
/*=========================================================================*/


class ads1015 : public capability {
	public:
		ads1015();
		~ads1015();
		bool init();
		bool parse(uint8_t* config);
		uint8_t* get_key();

		//bool loop();

		uint8_t count_intervall_update();
		bool intervall_update(uint8_t slot);

		//bool subscribe();
		//bool receive(uint8_t* p_topic, uint8_t* p_payload);
		bool publish();

		//uint8_t* get_dep();
		//bool reg_provider(peripheral * p, uint8_t *source);

		uint16_t readADC_SingleEnded(uint8_t ads_dev, uint8_t channel);
		float readADC_continues_ac_mV(uint8_t ads_dev, uint8_t channel, uint16_t sample);
		bool configureADC_continues(uint8_t ads_dev, uint8_t channel);
		int16_t readADC_Differential_0_1(uint8_t ads_dev);
		int16_t readADC_Differential_2_3(uint8_t ads_dev);
		void startComparator_SingleEnded(uint8_t ads_dev, uint8_t channel, int16_t threshold);
		int16_t getLastConversionResults(uint8_t ads_dev);
		void setGain(uint16_t gain);
		uint16_t getGain(void);
	private:
		void writeRegister(uint8_t i2cAddress, uint8_t reg, uint16_t value);
		uint16_t readRegister(uint8_t i2cAddress, uint8_t reg);
		// Instance-specific properties
		uint8_t m_conversionDelay; ///< conversion deay
		uint8_t m_bitShift;        ///< bit shift amount
		uint16_t m_gain;          ///< ADC gain
		uint8_t m_acdc_mode;
		bool m_discovery_pub;
		uint8_t m_accel;
		uint8_t m_nDevices;
		uint8_t m_nDevicesAdr[4];
		uint8_t m_devRdyPin[4];
		uint8_t m_intervall_call;
};


#endif