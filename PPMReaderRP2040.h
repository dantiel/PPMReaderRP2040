#ifndef PPM_READER_RP2040_CALLBACKS
#define PPM_READER_RP2040_CALLBACKS

#include <Arduino.h>

// Forward declaration of the ISR
void ppm_isr_handler();

// Define function pointer types for callbacks
typedef void (*PPMConnectCallback)();
typedef void (*PPMDisconnectCallback)();
typedef void (*PPMNewDataCallback)(const unsigned long* channelValues, byte numChannels);

class PPMReader {

public:
    // The range of a channel's possible values
    unsigned long minChannelValue = 1000;
    unsigned long maxChannelValue = 2000;

    /* The maximum error (in either direction) in channel value
     * with which the channel value is still considered valid */
    unsigned long channelValueMaxError = 10;

    /* The minimum value (time) after which the signal frame is considered to
     * be finished and we can start to expect a new signal frame. */
    unsigned long blankTime = 2100;

    // A time variable to remember when the last pulse was read
    volatile unsigned long microsAtLastPulse = 0;

    friend void ppm_isr_handler();
    PPMReader(byte interruptPin, byte channelAmount);
    ~PPMReader();

    /* Returns the latest raw (not necessarily valid) value for the
     * channel (starting from 1). */
    unsigned long rawChannelValue(byte channel);

    /* Returns the latest received value that was considered valid for the channel (starting from 1).
     * Returns defaultValue if the given channel hasn't received any valid values yet. */
    unsigned long latestValidChannelValue(byte channel, unsigned long defaultValue);

    /* Sets the callback function to be called when a PPM connection is established. */
    void onConnect(PPMConnectCallback callback);

    /* Sets the callback function to be called when the PPM connection is lost. */
    void onDisconnect(PPMDisconnectCallback callback);

    /* Sets the callback function to be called when new valid channel values are available. */
    void onNewData(PPMNewDataCallback callback);

    /* Must be called periodically in the loop() to check for connection status changes. */
    void loop();

private:
    // The pin from which to listen for interrupts
    byte interruptPin = 0;

    // The amount of channels to be expected from the PPM signal.
    byte channelAmount = 0;

    // Arrays for keeping track of channel values
    volatile unsigned long *rawValues = NULL;
    volatile unsigned long *validValues = NULL;

    // A counter variable for determining which channel is being read next
    volatile byte pulseCounter = 0;

    // Callbacks
    PPMConnectCallback _onConnectCallback = nullptr;
    PPMDisconnectCallback _onDisconnectCallback = nullptr;
    PPMNewDataCallback _onNewDataCallback = nullptr;

    // Connection status tracking
    volatile bool _isConnected = false;
    unsigned long _lastPPMActivityMicros = 0; // To track last valid frame time
    unsigned long _connectionTimeoutMicros = 500000; // 500ms timeout for connection loss

    // The ISR implementation
    void handleInterrupt();

    // Static instance for the ISR to access member functions
    static PPMReader* _instance;

    // Private helper to call onNewData callback safely
    void _callNewDataCallback();
};

#endif

