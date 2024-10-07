#pragma once

#include <Arduino.h>
#include <stdint.h>

#include <map>

static const uint8_t MAX_UNIT_LENGTH = 10;
static const uint8_t MAX_COMMAND_LENGTH = 9;
static const uint8_t MAX_CAPTION_LENGTH = 30;

enum parameter_id {
    SOOT_MASS_CALCULATED = 1,
    SOOT_MASS_MEASURED,
    OIL_ASH_RESIDUE,
    TIME_SINCE_LAST_REGENERATION,
    DISTANCE_SINCE_LAST_REGENERATION,
    DPF_INPUT_TEMPERATURE,
    DPF_OUTPUT_TEMPERATURE,
    REGENERATION_DURATION,
    SOOT_LOAD,
    POST_INJECTION_2,
    POST_INJECTION_3,
    DIFFERENTIAL_PRESSURE,
    DISTANCE_DRIVEN,
    LOW_PRESSURE_EGR_CLOSING
};

struct measurement_t {
    uint8_t id;                            /*Index*/
    char caption[MAX_CAPTION_LENGTH];      /*Caption*/
    char shortCaption[MAX_CAPTION_LENGTH]; /*Short caption (OLED)*/
    uint32_t command;                      /*Command to send*/
    uint8_t expected;                      /*Number of expected bytes*/
    char unit[MAX_UNIT_LENGTH];            /*Unit [g,km,min]*/
    double value;                          /*Calculated value*/
    uint8_t precision;
    double min;                                            /*Minimum value*/
    double max;                                            /*Maximum value*/
    bool (*calcFunPtr)(double* value, void* calcFunParam); /*A pointer to a function to retrieve and calculate values*/
    double divider;
    double bias;        /*Parameter to calc_fun*/
    void* calcFunParam; /*Parameter to calc_fun*/
    bool enabled;       /*Is measurement on?*/
    bool signedValue;
    void (*dataReadFunPtr)(float value); /*Function executed after reading the value*/

    bool operator==(const measurement_t& other) const {
        return id == other.id;
    }
};

class Measurements {
   public:
    static const String MEASUREMENTS_LOG;

    static void init();

    static std::map<parameter_id, measurement_t>& getActual();
    static std::map<parameter_id, measurement_t>& getStart();
    static std::map<parameter_id, measurement_t>& getLast();

    static const bool isEnabled(std::map<parameter_id, measurement_t>& measurement, const parameter_id id);
    static String getCaption(std::map<parameter_id, measurement_t>& measurement, const parameter_id id);
    static double& getValue(std::map<parameter_id, measurement_t>& measurement, const parameter_id id);
    static String getUnit(std::map<parameter_id, measurement_t>& measurement, const parameter_id id);
    static uint8_t getPrecision(std::map<parameter_id, measurement_t>& measurement, const parameter_id id);

    static void setValue(std::map<parameter_id, measurement_t>& measurement, const parameter_id id, const double value);

    static bool measure(measurement_t& measurement, const bool randomData = false);
    static float diff(const parameter_id id);

    static void log(std::map<parameter_id, measurement_t>& measurement);

    static String toString(std::map<parameter_id, measurement_t>& measurement);

   protected:
    static std::map<parameter_id, measurement_t> m_start;
    static std::map<parameter_id, measurement_t> m_actual;
    static std::map<parameter_id, measurement_t> m_last;
    static bool m_testmode;
};

class Regeneration {
   public:
    static bool isRegenerating();
    static bool check();

    static void onRegenerationStart();
    static void onRegenerationEnd();

   protected:
    static bool m_regeneration;
};
