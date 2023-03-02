#include "Sensor.h"
#include "smlDebug.h"

/* *** Sensor.cpp implementing Sensor class to receive sml data via a serial input pin and stor it in a buffer

2023-01-25   mh
- disables namespace std; added std:: to unique_ptr<SoftwareSerial>
  reason: byte was ambiguous

2022-12-07   mh
- added sensor state in callback function
- return to main loop when entering state WAIT_FOR_START_SEQUENCE to allow processing there

2022-11-30   mh
- adapted for own use based on https://github.com/mruettgers/SMLReader
- modularization of code (own cpp instead of monolithic include of code into main.cpp)
- removal of jled use

(C) M. Herbert, 2022.
Licensed under the GNU General Public License v3.0

*** end change log *** */

/* ***
# Description Class Sensor #
Class Sensor initializes the serial input PIN and transfers the serial input into a buffer.

## Usage ##
in Setup():
Create a Sensor object, providing the configuration structure and a call back function to process the message  

	Sensor \*sensor = new Sensor(config, process_message);

in Loop():  

	sensor->loop()

## Implementation ##
A state machine is used to
- wait for incoming data by checking for the SML start sequence
- transfer data to the buffer until the end sequence is recognized
- handle the CRC data
- initiate processing of the data using the callback function.

## Used libs ##
SoftwareSerial  
  

  *** end description *** */


//using namespace std;


uint64_t millis64()
{
    static uint32_t low32, high32;
    uint32_t new_low32 = millis();
    if (new_low32 < low32)
        high32++;
    low32 = new_low32;
    return (uint64_t)high32 << 32 | low32;
}

// public:

    Sensor::Sensor(const SensorConfig *config, void (*callback)(byte *buffer, size_t len, Sensor *sensor, State sensorState))
    {
        this->config = config;
        DEBUG("Initializing sensor %s...", this->config->name);
        this->callback = callback;
        this->serial = std::unique_ptr<SoftwareSerial>(new SoftwareSerial());
        this->serial->begin(9600, SWSERIAL_8N1, this->config->pin, -1, false);
        this->serial->enableTx(false);
        this->serial->enableRx(true);
        DEBUG("Initialized sensor %s.", this->config->name);

        this->init_state();
    }

    // loop ---------------------------------------------------------------------------------------
    void Sensor::loop()
    {
        this->run_current_state();
        yield();
    }

// private:

    // state machine ------------------------------------------------------------------------------
    void Sensor::run_current_state()
    {
        if (this->state != INIT)
        {
            if (this->state != STANDBY && ((millis() - this->last_state_reset) > (READ_TIMEOUT * 1000)))
            {
                DEBUG("Did not receive an SML message within %d seconds, starting over.", READ_TIMEOUT);
                this->reset_state();
            }
            switch (this->state)
            {
            case STANDBY:
                this->standby();
                break;
            case WAIT_FOR_START_SEQUENCE:
                this->wait_for_start_sequence();
                break;
            case READ_MESSAGE:
                this->read_message();
                break;
            case PROCESS_MESSAGE:
                this->process_message();
                break;
            case READ_CHECKSUM:
                this->read_checksum();
                break;
            default:
                break;
            }
        }
    }

    // Wrappers for sensor access -----------------------------------------------------------------
    int Sensor::data_available()
    {
        return this->serial->available();
    }
    int Sensor::data_read()
    {
        return this->serial->read();
    }

    // Set new state, debug messages, update some attributes---------------------------------------
    void Sensor::set_state(State new_state)
    {
        // call back to main() to update state
        if(new_state != PROCESS_MESSAGE)
        {
            this->callback(this->buffer, this->position, this, new_state);
        }
        if (new_state == STANDBY)
        {
            DEBUG("State of sensor %s is 'STANDBY'.", this->config->name);
        }
        else if (new_state == WAIT_FOR_START_SEQUENCE)
        {
            DEBUG("State of sensor %s is 'WAIT_FOR_START_SEQUENCE'.", this->config->name);
            this->last_state_reset = millis();
            this->position = 0;
            this->state = new_state;
            return;     // return to loop()
        }
        else if (new_state == READ_MESSAGE)
        {
            DEBUG("State of sensor %s is 'READ_MESSAGE'.", this->config->name);
        }
        else if (new_state == READ_CHECKSUM)
        {
            DEBUG("State of sensor %s is 'READ_CHECKSUM'.", this->config->name);
            this->bytes_until_checksum = 3;
        }
        else if (new_state == PROCESS_MESSAGE)
        {
            DEBUG("State of sensor %s is 'PROCESS_MESSAGE'.", this->config->name);
        };
        this->state = new_state;
    }

    // Initialize state machine -------------------------------------------------------------------
    void Sensor::init_state()
    {
        this->set_state(WAIT_FOR_START_SEQUENCE);
    }

    // reset: start over and wait for the start sequence ------------------------------------------
    void Sensor::reset_state(const char *message)
    {
        if (message != NULL && strlen(message) > 0)
        {
            DEBUG(message);
        }
        this->init_state();
    }

    // standby, wait interval before next sensor access ------------------------------------------
    void Sensor::standby()
    {
        // Keep buffers clean
        while (this->data_available())
        {
            this->data_read();
            yield();
        }

        if (millis64() >= this->standby_until)
        {
            this->reset_state();
        }
    }

    // Wait for the start_sequence to appear ------------------------------------------------------
    void Sensor::wait_for_start_sequence()
    {
        while (this->data_available())
        {
            this->buffer[this->position] = this->data_read();
            yield();

            this->position = (this->buffer[this->position] == START_SEQUENCE[this->position]) ? (this->position + 1) : 0;
            if (this->position == sizeof(START_SEQUENCE))
            {
                // Start sequence has been found
                DEBUG("Start sequence found.");
                this->set_state(READ_MESSAGE);
                return;
            }
        }
    }

    // Read the rest of the message ---------------------------------------------------------------
    void Sensor::read_message()
    {
        while (this->data_available())
        {
            // Check whether the buffer is still big enough to hold the number of fill bytes (1 byte) and the checksum (2 bytes)
            if ((this->position + 3) == BUFFER_SIZE)
            {
                this->reset_state("Buffer will overflow, starting over.");
                return;
            }
            this->buffer[this->position++] = this->data_read();
            yield();

            // Check for end sequence
            int last_index_of_end_seq = sizeof(END_SEQUENCE) - 1;
            for (int i = 0; i <= last_index_of_end_seq; i++)
            {
                if (END_SEQUENCE[last_index_of_end_seq - i] != this->buffer[this->position - (i + 1)])
                {
                    break;
                }
                if (i == last_index_of_end_seq)
                {
                    DEBUG("End sequence found.");
                    this->set_state(READ_CHECKSUM);
                    return;
                }
            }
        }
    }

    // Read the number of fillbytes and the checksum  ---------------------------------------------
    void Sensor::read_checksum()
    {
        while (this->bytes_until_checksum > 0 && this->data_available())
        {
            this->buffer[this->position++] = this->data_read();
            this->bytes_until_checksum--;
            yield();
        }

        if (this->bytes_until_checksum == 0)
        {
            DEBUG("Message has been read. Lenght=%d", this->position);
            DEBUG_DUMP_BUFFER(this->buffer, this->position);
            this->set_state(PROCESS_MESSAGE);
        }
    }

    // Process message by callback function -------------------------------------------------------
    void Sensor::process_message()
    {
        DEBUG("Message is being processed.");

        if (this->config->interval > 0)
        {
            this->standby_until = millis64() + (this->config->interval * 1000);
        }

        // Call listener
        if (this->callback != NULL)
        {
            this->callback(this->buffer, this->position, this, this->state);
        }

        // Go to standby mode, if throttling is enabled
        if (this->config->interval > 0)
        {
            this->set_state(STANDBY);
            return;
        }

        // Start over if throttling is disabled
        this->reset_state();
    }