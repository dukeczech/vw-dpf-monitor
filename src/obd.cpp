#include "obd.h"

#include <float.h>

#include "config.h"

ELM327 myELM327;

void OBD::init() {
    // Try to disable expected number of responses
    myELM327.specifyNumResponses = false;

    myELM327.sendCommand_Blocking("ATPPFFOFF");
    myELM327.sendCommand_Blocking("ATZ");
    myELM327.sendCommand_Blocking("ATD");
    myELM327.sendCommand_Blocking("ATPC");
    myELM327.sendCommand_Blocking("ATE0");
    myELM327.sendCommand_Blocking("ATS0");
    myELM327.sendCommand_Blocking("ATL0");
    myELM327.sendCommand_Blocking("ATAT1");
    myELM327.sendCommand_Blocking("ATM0");
    myELM327.sendCommand_Blocking("ATH1");
    myELM327.sendCommand_Blocking("ATSP6");
    myELM327.sendCommand_Blocking("ATSH7E0");
}

double OBD::getPID(const uint8_t& service, const uint16_t& pid, const uint8_t& numResponses, const uint8_t& numExpectedBytes,
                   const double& scaleFactor, const float& bias, const bool signedValue) {
    double value = myELM327.processPID(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
    while (myELM327.nb_rx_state != ELM_SUCCESS) {
        // Waiting for response
        value = myELM327.processPID(service, pid, numResponses, numExpectedBytes, scaleFactor, bias);
        if (myELM327.nb_rx_state != ELM_GETTING_MSG && myELM327.nb_rx_state != ELM_SUCCESS) {
            // Fatal error
            myELM327.printError();
            // TODO: Should reconnect if there is an error
            return DBL_MAX;
        }
    }
    if (myELM327.nb_rx_state == ELM_SUCCESS) {
        if (signedValue) {
            // We expect a signed value, convert the value
            switch (numExpectedBytes) {
                case 2: {
                    const int16_t result = (int16_t)myELM327.response;
                    if (scaleFactor == 1 && bias == 0)  // No scale/bias needed
                    {
                        return result;
                    } else {
                        return (result * scaleFactor) + bias;
                    }
                }
                case 4: {
                    const int32_t result = (int32_t)myELM327.response;
                    if (scaleFactor == 1 && bias == 0)  // No scale/bias needed
                    {
                        return result;
                    } else {
                        return ((double)result * (double)scaleFactor) + (double)bias;
                    }
                }
                default: {
                    break;
                }
            }
        }
        return value;
    }

    return -1.0;
}