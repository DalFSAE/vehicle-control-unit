 #ifndef SENSOR_CONTROL
 #define SENSOR_CONTROL


typedef enum {
    PDP_OKAY    = 0x00U,
    PDP_ERROR   = 0x01U,
    PDP_TIMEOUT = 0x03U,
    PDP_LATCH   = 0x04U
} PDP_StatusTypeDef;





/**
 * @struct SensorInfo_s
 * @brief Structure to hold ADC and voltage information for a sensor.
 *
 * This structure contains the ADC range, the nominal voltage range, the current
 * ADC value, and the normalized value for a sensor.
 */
typedef struct {
    const char* name;      /** Name of the sensor */
    float voltageMin;      /** Minimum nominal voltage */
    float voltageMax;      /** Maximum nominal voltage */
    int currentAdcValue;   /** Current ADC value */
    float normalizedValue; /** Normalized value (0.0 - 1.0) */
} SensorInfo_t;


typedef enum {
    APPS1,
    APPS2,
    FBPS,
    RBPS,
    NUM_SENSORS  // This is optional, but useful to denote the number of sensors
} SensorType_t;


/// @brief Enable or disable the throttle output
/// @param enable true = output enabled 
void enable_throttle(bool enable);

void sensor_init();

void set_dac_out(uint32_t dacOut);

 #endif