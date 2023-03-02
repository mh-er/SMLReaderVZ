#ifndef SML_HTTP_H
#define SML_HTTP_H
#include <sml/sml_file.h>
#include <Sensor.h>

#ifndef DEBUG_TRACE
    #define DEBUG_TRACE(trace, format, ...) if(trace) {printf(format, ##__VA_ARGS__); fflush(stdout); Serial.println();}
#endif

#define N_UUID_VALUE 5          // adapt if enum is changed.
enum UuidValueName
{
    vzENERGY_IN,
    vzENERGY_OUT,
    vzPOWER_IN,
    vzTEST,
    vzSML_HEART_BEAT
};

#define sizeOfUUID 48
struct SmlHttpConfig
{
  char vzServer[64] = VZ_SERVER;
  char vzMiddleware[64] = VZ_MIDDLEWARE;
  char uuidValue[N_UUID_VALUE][sizeOfUUID] = {"0","1","2","3","4",};
};

class SmlHttp
{
public:
    SmlHttp();
    void init(SmlHttpConfig &config);
    void setServerName(String serverName);
    void setMiddlewareName(String middlewareName);
    void testHttp();
    int postHttp(String vzUUID, String timeStamp, double value);
    void publish(Sensor *sensor, sml_file *file);
    String getTimeStamp();
    double getValue(UuidValueName select);

private:
    String _TimeStamp="0";          // ms
    String _serverName="";
    String _middlewareName="";
    char* _uuid[N_UUID_VALUE];
    double _value[N_UUID_VALUE];
};
#endif // SML_HTTP_H