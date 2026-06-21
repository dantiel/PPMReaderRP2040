#include "PPMReaderRP2040.h"

// Initialize the static instance to nullptr
PPMReader* PPMReader::_instance = nullptr;

// The C-style ISR wrapper
void ppm_isr_handler() {
    if (PPMReader::_instance) {
        PPMReader::_instance->handleInterrupt();
    }
}

PPMReader::PPMReader(byte interruptPin, byte channelAmount) {
    // Check for valid parameters
    if (interruptPin > 0 && channelAmount > 0) {
        // Setup an array for storing channel values
        this->channelAmount = channelAmount;
        rawValues = new unsigned long[channelAmount];
        validValues = new unsigned long[channelAmount];
        for (int i = 0; i < channelAmount; ++i) {
            rawValues[i] = 0;
            validValues[i] = 0;
        }
        // Set the static instance
        _instance = this;
        // Attach an interrupt to the pin
        this->interruptPin = interruptPin;
        pinMode(interruptPin, INPUT); // Consider INPUT_PULLUP if the signal source needs it
        attachInterrupt(digitalPinToInterrupt(interruptPin), ppm_isr_handler, RISING);
    }
}

PPMReader::~PPMReader() {
    detachInterrupt(digitalPinToInterrupt(interruptPin));
    delete [] rawValues;
    delete [] validValues;
    _instance = nullptr; // Clear the static instance
}

void PPMReader::onConnect(PPMConnectCallback callback) {
    _onConnectCallback = callback;
}

void PPMReader::onDisconnect(PPMDisconnectCallback callback) {
    _onDisconnectCallback = callback;
}

void PPMReader::onNewData(PPMNewDataCallback callback) {
    _onNewDataCallback = callback;
}

void PPMReader::loop() {
    unsigned long currentMicros = micros();

    // Check for connection loss
    if (_isConnected && (currentMicros - _lastPPMActivityMicros > _connectionTimeoutMicros)) {
        _isConnected = false;
        if (_onDisconnectCallback) {
            _onDisconnectCallback();
        }
    }
}

void PPMReader::handleInterrupt() {
    // Remember the current micros() and calculate the time since the last pulseReceived()
    unsigned long previousMicros = microsAtLastPulse;
    microsAtLastPulse = micros();
    unsigned long time = microsAtLastPulse - previousMicros;

    // Update last activity time for connection tracking
    _lastPPMActivityMicros = microsAtLastPulse;

    if (!_isConnected) {
        _isConnected = true;
        if (_onConnectCallback) {
            _onConnectCallback();
        }
    }

    if (time > blankTime) {
        /* If the time between pulses was long enough to be considered an end
         * of a signal frame, prepare to read channel values from the next pulses */
        // If a full frame was received, trigger new data callback
        if (pulseCounter == channelAmount && _onNewDataCallback) {
            // Copy valid values to a temporary array to ensure consistency for the callback
            unsigned long tempValues[channelAmount];
            noInterrupts(); // Temporarily disable interrupts for safe copy
            for (int i = 0; i < channelAmount; ++i) {
                tempValues[i] = validValues[i];
            }
            interrupts();
            _onNewDataCallback(tempValues, channelAmount);
        }
        pulseCounter = 0;
    }
    else {
        // Store times between pulses as channel values
        if (pulseCounter < channelAmount) {
            rawValues[pulseCounter] = time;
            if (time >= minChannelValue - channelValueMaxError && time <= maxChannelValue + channelValueMaxError) {
                validValues[pulseCounter] = constrain(time, minChannelValue, maxChannelValue);
            }
        }
        ++pulseCounter;
    }
}

unsigned long PPMReader::rawChannelValue(byte channel) {
    // Check for channel's validity and return the latest raw channel value or 0
    unsigned long value = 0;
    if (channel >= 1 && channel <= channelAmount) {
        noInterrupts();
        value = rawValues[channel-1];
        interrupts();
    }
    return value;
}

unsigned long PPMReader::latestValidChannelValue(byte channel, unsigned long defaultValue) {
    // Check for channel's validity and return the latest valid channel value or defaultValue.
    unsigned long value = defaultValue;
    if (channel >= 1 && channel <= channelAmount) {
        noInterrupts();
        value = validValues[channel-1];
        interrupts();
    }
    return value;
}
