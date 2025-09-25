#include "RX5808.h"
#include "buzzer.h"
#include "config.h"
#include "kalman.h"
#include "led.h"

typedef enum {
    STOPPED,
    WAITING,
    RUNNING
} laptimer_state_e;

#define LAPTIMER_LAP_HISTORY 50

class LapTimer {
  public:
    void init(Config *config, RX5808 *rx5808, Buzzer *buzzer, Led *l);
    void start();
    void stop();
    void handleLapTimerUpdate(uint32_t currentTimeMs);
    uint8_t getChartRssi();
    uint32_t getLapTime();
    bool isLapAvailable();

  private:
    laptimer_state_e state = STOPPED;
    RX5808 *rx;
    Config *conf;
    Buzzer *buz;
    Led *led;
    KalmanFilter filter;
    uint32_t startTimeMs;
    uint8_t lapCount;
    uint8_t rssiCount;
    uint32_t lapTimes[LAPTIMER_LAP_HISTORY];
    uint8_t rssi;
    uint8_t rssiChartMax;
    uint8_t rssiPeak;
    uint32_t rssiPeakTimeMs;

    bool lapAvailable = false;

    void lapPeakCapture();
    bool lapPeakCaptured();
    void lapPeakReset();

    void startLap();
    void finishLap();
};
