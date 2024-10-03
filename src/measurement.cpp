#include "measurement.h"

#include <Arduino.h>
#include <float.h>

#include "gui/sound.h"
#include "obd.h"

bool calculateEGRClosing(double* value, void* calcFunParam = NULL) {
    *value = 100.0 - *value;
    return true;
}

std::map<parameter_id, measurement_t> Measurements::m_start;
std::map<parameter_id, measurement_t> Measurements::m_actual;
bool Measurements::m_regeneration = false;
bool Measurements::m_testmode = false;

void Measurements::init() {
    m_actual.insert({SOOT_MASS_MEASURED, (measurement_t){1, "Soot mass measured", "Soot mass(m)", 0x221ABE, 2, "g", 0.0, 2, -15.0, 30.0, NULL, 0.01, 0, NULL, true, true, NULL}});
    m_actual.insert({SOOT_MASS_CALCULATED, (measurement_t){2, "Soot mass calculated", "Soot mass(c)", 0x222609, 2, "g", 0.0, 2, 0.0, 26.0, NULL, 0.01, 0, NULL, true, false, NULL}});
    m_actual.insert({OIL_ASH_RESIDUE, (measurement_t){7, "Oil ash residue", "Ash residue", 0x221ABD, 4, "g", 0.0, 2, 0.0, 80.0, NULL, 0.000000119, 0, NULL, true, true, NULL}});
    m_actual.insert({DISTANCE_SINCE_LAST_REGENERATION, (measurement_t){3, "Distance since last regen.", "Last regen", 0x221ABA, 2, "km", 0.0, 1, 0.0, 800.0, NULL, 0.1, 0, NULL, true, false, NULL}});
    m_actual.insert({TIME_SINCE_LAST_REGENERATION, (measurement_t){4, "Time since last regen", "Last regen", 0x221AC3, 2, "min", 0.0, 1, 0.0, 1000.0, NULL, 1 / 8.53333, 0, NULL, true, false, NULL}});
    m_actual.insert({DPF_INPUT_TEMPERATURE, (measurement_t){5, "Input temperature", "Input temp", 0x2211B2, 2, "`C", 0.0, 1, 0.0, 720.0, NULL, 0.1, -273.1, NULL, true, false, NULL}});
    m_actual.insert({DPF_OUTPUT_TEMPERATURE, (measurement_t){6, "Output temperature", "Ouput temp", 0x2210F9, 2, "`C", 0.0, 1, 0.0, 650.0, NULL, 0.1, -273.1, NULL, true, false, NULL}});
    m_actual.insert({REGENERATION_DURATION, (measurement_t){7, "Regeneration duration", "Duration", 0x221AC0, 2, "min", 0.0, 1, 0.0, 100.0, NULL, /*1 / 94*/ 0.01, 0, NULL, true, false, NULL}});
    m_actual.insert({SOOT_LOAD, (measurement_t){8, "Soot load", "Soot load", 0, 0, "%", 0.0, 0, 0.0, 100.0, NULL, 1.0, 0, NULL, false, false, NULL}});
    m_actual.insert({POST_INJECTION_2, (measurement_t){9, "Post injection 2", "Inj(2)", 0x22163B, 2, "mg/str", 0.0, 2, 0.0, 30.0, NULL, 0.01, 0, NULL, true, false, NULL}});
    m_actual.insert({POST_INJECTION_3, (measurement_t){10, "Post injection 3", "Inj(3)", 0x22167D, 2, "mg/str", 0.0, 2, 0.0, 30.0, NULL, 0.01, 0, NULL, true, false, NULL}});
    m_actual.insert({DIFFERENTIAL_PRESSURE, (measurement_t){11, "Differential pressure", "Diff. press", 0x2214F5, 2, "hPa", 0.0, 0, 0.0, 250.0, NULL, 1.0, 0, NULL, true, false, NULL}});
    m_actual.insert({DISTANCE_DRIVEN, (measurement_t){12, "Distance driven", "Distance", 0x2216A9, 4, "km", 0.0, 0, 0.0, 400000.0, NULL, 1.0, 0, NULL, true, false, NULL}});
    m_actual.insert({LOW_PRESSURE_EGR_CLOSING, (measurement_t){13, "Low press. EGR closing", "EGR", 0x2217F4, 2, "%", 0.0, 0, -100.0, 100.0, &calculateEGRClosing, 1 / 81.92, 0, NULL, true, false, NULL}});
}

void Measurements::copy() {
    m_start = m_actual;
}

std::map<parameter_id, measurement_t>& Measurements::getActual() {
    return m_actual;
}

std::map<parameter_id, measurement_t>& Measurements::getStart() {
    return m_start;
}

const bool Measurements::isEnabled(const parameter_id id) {
    return m_actual[id].enabled;
}

double& Measurements::getValue(const parameter_id id) {
    return m_actual[id].value;
}

String Measurements::getUnit(const parameter_id id) {
    return m_actual[id].unit;
}

void Measurements::setValue(const parameter_id id, const double value) {
    m_actual[id].value = value;
}

bool Measurements::measure(measurement_t& measurement, const bool randomData) {
    if (measurement.enabled) {
        if (m_testmode) {
            static double MASS[] = {19.0, 20.31, 21.87, 22.66, 23.82, 24.08, 23.28, 21.74, 20.33, 18.47, 11.33, 5.3};
            static double TEMP[] = {278.6, 299.1, 380.47, 402.66, 453.12, 527.1, 512.3, 500.0, 499.3, 480.3, 402.2, 320.3};
            static double INJECTION2[] = {0.0, 2.42, 2.42, 6.6, 4.53, 5.27, 5.3, 0.0, 6.6, 22.4, 0.0, 0.0};
            static uint8_t i = 0;

            if (measurement.id == SOOT_MASS_CALCULATED) {
                measurement.value = MASS[i];
                return true;
            } else if (measurement.id == DPF_INPUT_TEMPERATURE) {
                measurement.value = TEMP[i];
                return true;
            } else if (measurement.id == POST_INJECTION_2) {
                measurement.value = INJECTION2[i];
                if (i >= sizeof(MASS) - 1)
                    i = 0;
                else
                    i++;

                return true;
            }
        }

        if (randomData) {
            // Just simulate a real data
            delay(50);
            measurement.value = (std::fmod((double)random(), (100.0 * measurement.max - 100.0 * measurement.min + 1) + 100.0 * measurement.min)) / 100.0;
            return true;
        }

        double value = 0.0;
        if (((measurement.command & 0xff0000) >> 16) > 0) {
            // 3-bytes command
            // Returns DBL_MAX if send/receive error
            value = OBD::getPID((measurement.command & 0xff0000) >> 16, measurement.command & 0xffff, 1, measurement.expected,
                                measurement.divider, measurement.bias, measurement.signedValue);
        } else if (((measurement.command & 0xff00) >> 8) > 0) {
            // 2-bytes command
            value = OBD::getPID((measurement.command & 0xff00) >> 8, measurement.command & 0xff, 1, measurement.expected,
                                measurement.divider, measurement.bias, measurement.signedValue);
        }

        if (value == DBL_MAX) {
            return false;
        }

        if (value < measurement.min || value > measurement.max) {
            Serial.printf("Measurement value is out of range: %f [%f - %f]\n", measurement.value, measurement.min, measurement.max);

            return false;
        }

        // The value seems to be correct, assing to the measurement
        measurement.value = value;

        if (measurement.calcFunPtr != NULL) {
            // Call the custom function
            measurement.calcFunPtr(&measurement.value, measurement.calcFunParam);
        }
    }
    return true;
}

float Measurements::diff(const parameter_id id) {
    if (m_start.empty()) return 0.0;
    return m_actual[id].value - m_start[id].value;
}

bool Measurements::regeneration() {
    // How to check the regeneration start?
    // Mass calculated > 24g
    // Mass calculated decreases
    // Post injection is > 0
    // Input temp > 380C

    // How to check the regeneration end?
    // Regeneration duration is 0
    // Last regen. distance is 0

    if (m_regeneration) {
        // Try to guess the regeneration process end
        // Turns out that regeneration stopped when the regeneration duration is zero
        if (getValue(REGENERATION_DURATION) == 0.0 && !m_testmode) {
            Sound::beep1long();
            m_regeneration = false;

            // Use the new start values
            Measurements::copy();
        }

        if (getValue(DPF_INPUT_TEMPERATURE) < 380.0 && (getValue(POST_INJECTION_2) + getValue(POST_INJECTION_3) == 0.0)) {
            Sound::beep1long();
            m_regeneration = false;

            // Use the new start values
            Measurements::copy();
        }
    } else {
        // Try to guess the regeneration process start
        // Turns out that regeneration started when the regeneration duration is positive
        if (getValue(REGENERATION_DURATION) > 0.0 && !m_testmode) {
            // In the test mode this causes the regeneration starts immediatelly
            Sound::beep3long();
            m_regeneration = true;
        }

        if (getValue(SOOT_MASS_CALCULATED) > 24.0 && getValue(DPF_INPUT_TEMPERATURE) >= 380.0 &&
            (getValue(POST_INJECTION_2) + getValue(POST_INJECTION_3) > 0.0)) {
            Sound::beep3long();
            m_regeneration = true;
        }
    }

    return m_regeneration;
}

void Measurements::startTest() {
    m_testmode = true;
}

void Measurements::stopTest() {
    m_testmode = false;
}
