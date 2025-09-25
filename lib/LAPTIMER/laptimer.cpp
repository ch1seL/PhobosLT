#include "laptimer.h"
#include "debug.h"

const uint16_t rssi_filter_q = 2000; //  0.01 - 655.36
const uint16_t rssi_filter_r = 40;   // 0.0001 - 65.536

void LapTimer::init(Config *config, RX5808 *rx5808, Buzzer *buzzer, Led *l) {
    conf = config;
    rx = rx5808;
    buz = buzzer;
    led = l;

    filter.setMeasurementNoise(rssi_filter_q * 0.01f);
    filter.setProcessNoise(rssi_filter_r * 0.0001f);

    stop();
}

void LapTimer::start() {
    DEBUG("LapTimer started\n");
    state = RUNNING;
    buz->beep(500);
    led->on(500);
}

void LapTimer::stop() {
    DEBUG("LapTimer stopped\n");
    state = STOPPED;
    lapCount = 0;
    rssiCount = 0;
    memset(lapTimes, 0, sizeof(lapTimes));
    buz->beep(500);
    led->on(500);
}

void LapTimer::handleLapTimerUpdate(uint32_t currentTimeMs) {
    // always read RSSI
    rssi = round(filter.filter(rx->readRssi(), 0));
    // DEBUG("RSSI: %u\n", rssi);
    if (rssi > rssiChartMax) {
        rssiChartMax = rssi;
    }

    switch (state) {
    case STOPPED:
        break;
    case WAITING:
        // detect hole shot
        lapPeakCapture();
        if (lapPeakCaptured()) {
            state = RUNNING;
            startLap();
        }
        break;
    case RUNNING:
        // Check if timer min has elapsed, start capturing peak
        if ((currentTimeMs - startTimeMs) > conf->getMinLapMs()) {
            lapPeakCapture();
        }

        if (lapPeakCaptured()) {
            finishLap();
            startLap();
        }
        break;
    default:
        break;
    }
}

void LapTimer::lapPeakCapture() {
    // Check if RSSI is on or post threshold, update RSSI peak
    if (rssi >= conf->getEnterRssi()) {
        // Check if RSSI is greater than the previous detected peak
        if (rssi > rssiPeak) {
            rssiPeak = rssi;
            rssiPeakTimeMs = millis();
        }
    }
}

bool LapTimer::lapPeakCaptured() {
    return (rssi < rssiPeak) && (rssi < conf->getExitRssi());
}

void LapTimer::startLap() {
    DEBUG("Lap started\n");
    startTimeMs = rssiPeakTimeMs;
    rssiPeak = 0;
    rssiPeakTimeMs = 0;
    buz->beep(200);
    led->on(200);
}

void LapTimer::finishLap() {
    lapTimes[lapCount] = rssiPeakTimeMs - startTimeMs;
    DEBUG("Lap finished, lap time = %u\n", lapTimes[lapCount]);
    lapCount = (lapCount + 1) % LAPTIMER_LAP_HISTORY;
    lapAvailable = true;
}

uint8_t LapTimer::getChartRssi() {
    uint8_t max = rssiChartMax;
    rssiChartMax = rssi;
    return max;
}

uint32_t LapTimer::getLapTime() {
    uint32_t lapTime;
    lapAvailable = false;
    if (lapCount == 0) {
        lapTime = lapTimes[LAPTIMER_LAP_HISTORY - 1];
    } else {
        lapTime = lapTimes[lapCount - 1];
    }
    return lapTime;
}

bool LapTimer::isLapAvailable() {
    return lapAvailable;
}
