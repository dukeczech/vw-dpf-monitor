#include "measurement.h"

#include <Arduino.h>
#include <float.h>

#include "config.h"
#include "gui/sound.h"
#include "obd.h"
#include "storage.h"

bool calculateEGRClosing(double* value, void* calcFunParam = NULL) {
    *value = 100.0 - *value;
    return true;
}

const String Measurements::MEASUREMENTS_LOG = "/measurements.log";
std::map<parameter_id, measurement_t> Measurements::m_start;
std::map<parameter_id, measurement_t> Measurements::m_actual;

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

    if (testMode) {
        Storage::remove(MEASUREMENTS_LOG);
    }
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

String Measurements::getCaption(const parameter_id id) {
    return String(m_actual[id].caption);
}

double& Measurements::getValue(const parameter_id id) {
    return m_actual[id].value;
}

String Measurements::getUnit(const parameter_id id) {
    return String(m_actual[id].unit);
}

uint8_t Measurements::getPrecision(const parameter_id id) {
    return m_actual[id].precision;
}

void Measurements::setValue(const parameter_id id, const double value) {
    m_actual[id].value = value;
}

bool Measurements::measure(measurement_t& measurement, const bool randomData) {
    if (measurement.enabled) {
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

void Measurements::log() {
    // Log the measurements to the file
    if (!Storage::exists(MEASUREMENTS_LOG)) {
        // Create a log file header
        String header;
        header += getCaption(DISTANCE_DRIVEN) + " (" + getUnit(DISTANCE_DRIVEN) + ")\t";
        header += getCaption(SOOT_MASS_CALCULATED) + " (" + getUnit(SOOT_MASS_CALCULATED) + ")\t";
        header += getCaption(REGENERATION_DURATION) + " (" + getUnit(REGENERATION_DURATION) + ")\t";
        header += getCaption(DISTANCE_SINCE_LAST_REGENERATION) + " (" + getUnit(DISTANCE_SINCE_LAST_REGENERATION) + ")\t";
        header += getCaption(TIME_SINCE_LAST_REGENERATION) + " (" + getUnit(TIME_SINCE_LAST_REGENERATION) + ")\t";
        header += getCaption(OIL_ASH_RESIDUE) + " (" + getUnit(OIL_ASH_RESIDUE) + ")\t";
        header += getCaption(LOW_PRESSURE_EGR_CLOSING) + " (" + getUnit(LOW_PRESSURE_EGR_CLOSING) + ")\t";
        header += getCaption(DPF_INPUT_TEMPERATURE) + " (" + getUnit(DPF_INPUT_TEMPERATURE) + ")\t";
        header += getCaption(DPF_OUTPUT_TEMPERATURE) + " (" + getUnit(DPF_OUTPUT_TEMPERATURE) + ")\t";
        header += getCaption(SOOT_MASS_MEASURED) + " (" + getUnit(SOOT_MASS_MEASURED) + ")\t";
        header += getCaption(POST_INJECTION_2) + " (" + getUnit(POST_INJECTION_2) + ")\t";
        header += getCaption(POST_INJECTION_3) + " (" + getUnit(POST_INJECTION_3) + ")\t";
        header += getCaption(DIFFERENTIAL_PRESSURE) + " (" + getUnit(DIFFERENTIAL_PRESSURE) + ")";
        header += "\n";

        // Write the header
        Storage::write(MEASUREMENTS_LOG, header);
    }

    // Write the measurements
    Storage::append(MEASUREMENTS_LOG, Measurements::toString());

    Serial.printf("Regeneration started, measurements: %d\n", Storage::countLines(MEASUREMENTS_LOG));
    Serial.printf("%s\n", Storage::read(MEASUREMENTS_LOG));
}

String Measurements::toString() {
    String output;

    output += String(getValue(DISTANCE_DRIVEN), (unsigned int)getPrecision(DISTANCE_DRIVEN)) + "\t";
    output += String(getValue(SOOT_MASS_CALCULATED), (unsigned int)getPrecision(SOOT_MASS_CALCULATED)) + "\t";
    output += String(getValue(REGENERATION_DURATION), (unsigned int)getPrecision(REGENERATION_DURATION)) + "\t";
    output += String(getValue(DISTANCE_SINCE_LAST_REGENERATION), (unsigned int)getPrecision(DISTANCE_SINCE_LAST_REGENERATION)) + "\t";
    output += String(getValue(TIME_SINCE_LAST_REGENERATION), (unsigned int)getPrecision(TIME_SINCE_LAST_REGENERATION)) + "\t";
    output += String(getValue(OIL_ASH_RESIDUE), (unsigned int)getPrecision(OIL_ASH_RESIDUE)) + "\t";
    output += String(getValue(LOW_PRESSURE_EGR_CLOSING), (unsigned int)getPrecision(LOW_PRESSURE_EGR_CLOSING)) + "\t";
    output += String(getValue(DPF_INPUT_TEMPERATURE), (unsigned int)getPrecision(DPF_INPUT_TEMPERATURE)) + "\t";
    output += String(getValue(DPF_OUTPUT_TEMPERATURE), (unsigned int)getPrecision(DPF_OUTPUT_TEMPERATURE)) + "\t";
    output += String(getValue(SOOT_MASS_MEASURED), (unsigned int)getPrecision(SOOT_MASS_MEASURED)) + "\t";
    output += String(getValue(POST_INJECTION_2), (unsigned int)getPrecision(POST_INJECTION_2)) + "\t";
    output += String(getValue(POST_INJECTION_3), (unsigned int)getPrecision(POST_INJECTION_3)) + "\t";
    output += String(getValue(DIFFERENTIAL_PRESSURE), (unsigned int)getPrecision(DIFFERENTIAL_PRESSURE));
    output += "\n";

    return output;
}

bool Regeneration::m_regeneration = false;

bool Regeneration::isRegenerating() {
    return m_regeneration;
}

bool Regeneration::check() {
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
        if (Measurements::getValue(REGENERATION_DURATION) == 0.0 && !testMode) {
            m_regeneration = false;
            Regeneration::onRegenerationEnd();
        }

        if (Measurements::getValue(DPF_INPUT_TEMPERATURE) < 380.0 &&
            (Measurements::getValue(POST_INJECTION_2) + Measurements::getValue(POST_INJECTION_3) == 0.0)) {
            m_regeneration = false;
            Regeneration::onRegenerationEnd();
        }
    } else {
        // Try to guess the regeneration process start
        // Turns out that regeneration started when the regeneration duration is positive
        if (Measurements::getValue(REGENERATION_DURATION) > 0.0 && !testMode) {
            // In the test mode this causes the regeneration starts immediatelly
            m_regeneration = true;
            Regeneration::onRegenerationStart();
        }

        if (Measurements::getValue(SOOT_MASS_CALCULATED) > 24.0 && Measurements::getValue(DPF_INPUT_TEMPERATURE) >= 380.0 &&
            (Measurements::getValue(POST_INJECTION_2) + Measurements::getValue(POST_INJECTION_3) > 0.0)) {
            m_regeneration = true;
            Regeneration::onRegenerationStart();
        }
    }

    return m_regeneration;
}

void Regeneration::onRegenerationStart() {
    Sound::beep3long();

    // Log the regeneration start values
    Measurements::log();
}

void Regeneration::onRegenerationEnd() {
    Sound::beep1long();

    // Use the new start values
    Measurements::copy();

    Serial.println(F("Regeneration ended"));
}
