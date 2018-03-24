#include <Joystick.h>

#define BTN_SELECT 2
#define BTN_START 3
#define BTN_X 4
#define BTN_A 5
#define BTN_B 6
#define BTN_Y 7
#define BTN_RS 8
#define BTN_LS 9
#define BTN_HOME A3
#define ANALOG_LH A1
#define ANALOG_LV A0
#define DPAD_UP 16
#define DPAD_DOWN 14
#define DPAD_LEFT 15
#define DPAD_RIGHT 10
#define VBAT A2

// Index is joystick button number, value is corresponding arduino pin #
char Buttons[] = { BTN_SELECT, BTN_START, BTN_B, BTN_Y, BTN_A, BTN_X, BTN_LS, BTN_RS, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, BTN_HOME };

#define NBUTTONS (sizeof(Buttons))
unsigned long ButtonTimes[NBUTTONS];

#ifdef _SAM3XA_     // Due
#define LED 13
#define Serial SerialUSB
#else
#define LED 17
#endif

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK, 
    NBUTTONS, // # of buttons
    0, // # of hatswitches
    true, // X axis
    true, // Y axis
    // All other axes off
    false, false, false, false, false, false, false, false, false);

#define LVC 492
#define LHC 466
// Delay for repeating keyboard presses, such as volume up/down
#define REPEAT_DELAY 250
// General debounce delay
#define DEBOUNCE_DELAY 20
// Delay before sending shutdown when power is down
//#define POWER_KEY_DELAY 1250
// Auto shutdown after 15 mins
#define AUTO_SHUTDOWN_TIME 15 * 60 * 1000L

unsigned long volumeUpTime = 0;
unsigned long volumeDownTime = 0;
unsigned long lastInputTime = 0;

void setup() {
  Serial.begin(9600);

  // Initialize all buttons
  for (int i = 0; i < NBUTTONS; i++)
      pinMode(Buttons[i], INPUT_PULLUP);

  pinMode(ANALOG_LV, INPUT);     //ANALOG LEFT VERTICAL
  pinMode(ANALOG_LH, INPUT);     //ANALOG LEFT HORIZONTAL
  pinMode(VBAT, INPUT);     // Battery voltage
  pinMode(LED, OUTPUT);     // Indicator LED
  digitalWrite(LED, HIGH);  // Turn off LED

  Joystick.setXAxisRange(0, 1023);
  Joystick.setYAxisRange(0, 1023);
  Joystick.begin(false);

  lastInputTime = millis();     // Avoid immediate shutdown
}

void loop() {
     bool buttonPressed = false;
//due     long analogLV = 512 - ((long)analogRead(ANALOG_LV) - LVC) * 512 / 330;
     long analogLV = ((long)analogRead(ANALOG_LV) - LVC) * 512 / 320 + 512;
     analogLV = constrain(analogLV, 0, 1023);
     Joystick.setYAxis(analogLV);
     if (abs(analogLV - 512) > 256)
        buttonPressed = true;
     
// due     long analogLH = ((long)analogRead(ANALOG_LH) - LHC) * 512 / 330 + 512;
     long analogLH = 512 - ((long)analogRead(ANALOG_LH) - LHC) * 512 / 320;
     analogLH = constrain(analogLH, 0, 1023);
     Joystick.setXAxis(analogLH);
     if (abs(analogLH - 512) > 256)
        buttonPressed = true;

     //Serial.print("X="); Serial.print(analogLH); Serial.print(" Y="); Serial.println(analogLV);

     const char selectDown = digitalRead(BTN_SELECT) == LOW;
     
     for (int i = 0; i < NBUTTONS; i++) {
        if (millis() - ButtonTimes[i] < DEBOUNCE_DELAY)
            continue;
        if (digitalRead(Buttons[i]) == LOW) {
            if (Buttons[i] == DPAD_UP && selectDown) {
                if (volumeUpTime == 0 || millis() - volumeUpTime >= REPEAT_DELAY) {
                    volumeUpTime = millis();
                    volumeDownTime = 0;
                    Serial.println("VOLUME+");
                }
            } 
            else if (Buttons[i] == DPAD_DOWN && selectDown) {
                if (volumeDownTime == 0 || millis() - volumeDownTime >= REPEAT_DELAY) {
                    volumeUpTime = 0;
                    volumeDownTime = millis();
                    Serial.println("VOLUME-");
                }
            } else {
                Joystick.pressButton(i);
            }
            ButtonTimes[i] = millis();
            buttonPressed = true;
        }
        else {
            if (Buttons[i] == DPAD_UP && millis() - volumeUpTime >= DEBOUNCE_DELAY) {
                volumeUpTime = 0;
            }
            else if (Buttons[i] == DPAD_DOWN && millis() - volumeDownTime >= DEBOUNCE_DELAY) { 
                volumeDownTime = 0;
            }
            Joystick.releaseButton(i);
        }
    }

    digitalWrite(LED, !buttonPressed);
    Joystick.sendState();
    readBattery();

    if (buttonPressed)
        lastInputTime = millis();
    else if (millis() - lastInputTime >= AUTO_SHUTDOWN_TIME) {
        shutdown();
    }

}

void shutdown() {
    Serial.println("POWEROFF");
    lastInputTime = millis();
}

int16_t batteryVoltage[10];
char vbatIndex = 0;
unsigned long lastBatteryRead = 0;

void readBattery() {
    const int nsamples = sizeof(batteryVoltage)/sizeof(int16_t);
    // Send value every second so measure "nsamples" times per second
    if (millis() - lastBatteryRead < 1000 / nsamples)
        return;
    lastBatteryRead = millis();
        
    int vbat = analogRead(VBAT);
    batteryVoltage[vbatIndex++] = vbat;
    if (vbatIndex >= nsamples) {
        vbatIndex = 0;
        float avgVBat = 0;
        for (int i = 0; i < nsamples; i++)
            avgVBat += batteryVoltage[i];
        avgVBat /= nsamples;
        Serial.print("VBAT=");
        Serial.println(avgVBat*5/1024);
    }
}

