#ifndef SENSOR_H
#define SENSOR_H

#include <SoftwareSerial.h>

// SML constants
const byte START_SEQUENCE[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01};
const byte END_SEQUENCE[] = {0x1B, 0x1B, 0x1B, 0x1B, 0x1A};
const size_t BUFFER_SIZE = 3840; // Max datagram duration 400ms at 9600 Baud
const uint8_t READ_TIMEOUT = 30;

// States
enum State
{
    INIT,
    STANDBY,
    WAIT_FOR_START_SEQUENCE,
    READ_MESSAGE,
    PROCESS_MESSAGE,
    READ_CHECKSUM
};

uint64_t millis64();

class SensorConfig
{
public:
    const uint8_t pin;
    const char *name;
    const bool numeric_only;
    const uint8_t interval;
};

class Sensor
{
public:
    const SensorConfig *config;
    Sensor(const SensorConfig *config, void (*callback)(byte *buffer, size_t len, Sensor *sensor, State sensorState));
    void loop();

private:
    std::unique_ptr<SoftwareSerial> serial;
    byte buffer[BUFFER_SIZE];
    size_t position = 0;
    unsigned long last_state_reset = 0;
    uint64_t standby_until = 0;
    uint8_t bytes_until_checksum = 0;
    uint8_t loop_counter = 0;
    State state = INIT;
    void (*callback)(byte *buffer, size_t len, Sensor *sensor, State sensorState) = NULL;

    void run_current_state();

    // Wrappers for sensor access
    int data_available();
    int data_read();

    // Set state
    void set_state(State new_state);

    // Initialize state machine
    void init_state();

    // Start over and wait for the start sequence
    void reset_state(const char *message = NULL);

    void standby();

    // Wait for the start_sequence to appear
    void wait_for_start_sequence();

    // Read the rest of the message
    void read_message();

    // Read the number of fillbytes and the checksum
    void read_checksum();

    void process_message();
};
#endif  //SENSOR_H