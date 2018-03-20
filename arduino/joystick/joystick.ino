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
//#define POWER_KEY A2
#define DPAD_UP 16
#define DPAD_DOWN 14
#define DPAD_LEFT 15
#define DPAD_RIGHT 10
#define VBAT A2

char Buttons[] = { BTN_SELECT, BTN_START, BTN_B, BTN_Y, BTN_A, BTN_X, BTN_LS, BTN_RS, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, BTN_HOME };

#ifdef _SAM3XA_     // Due
#define LED 13
#define Serial SerialUSB
#else
#define LED 17
#endif

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, JOYSTICK_TYPE_JOYSTICK, 
    sizeof(Buttons), // # of buttons
    0, // # of hatswitches
    true, // X axis
    true, // Y axis
    // All other axes off
    false, false, false, false, false, false, false, false, false);

#define LVC 506
#define LHC 466
// Delay for repeating keyboard presses, such as volume up/down
#define REPEAT_DELAY 250
// General debounce delay
#define DEBOUNCE_DELAY 20
// Delay before sending shutdown when power is down
//#define POWER_KEY_DELAY 1250
// Auto shutdown after 15 mins (disabled for now)
//#define AUTO_SHUTDOWN_TIME 15 * 60 * 1000L
// Grace delay for shutdown before power is cut
//#define POWEROFF_DELAY 6 * 1000L

unsigned long volumeUpTime = 0;
unsigned long volumeDownTime = 0;
//unsigned long powerKeyTime = 0;
unsigned long lastInputTime = 0;
//unsigned long shutdownTime = 0;

void setup() {
  Serial.begin(9600);

  // Initialize all buttons
  for (int i = 0; i < sizeof(Buttons); i++)
      pinMode(Buttons[i], INPUT_PULLUP);

  pinMode(ANALOG_LV, INPUT);     //ANALOG LEFT VERTICAL
  pinMode(ANALOG_LH, INPUT);     //ANALOG LEFT HORIZONTAL
//  pinMode(POWER_KEY, INPUT);     // PowerBank power button
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
     long analogLV = 512 - ((long)analogRead(ANALOG_LV) - LVC) * 512 / 330;
     analogLV = constrain(analogLV, 0, 1023);
     Joystick.setYAxis(analogLV);
//FIXME     if (abs(analogLV - 512) > 256)
//        buttonPressed = true;
     
     long analogLH = ((long)analogRead(ANALOG_LH) - LHC) * 512 / 330 + 512;
     analogLH = constrain(analogLH, 0, 1023);
     Joystick.setXAxis(analogLH);
//FIXME     if (abs(analogLH - 512) > 256)
//        buttonPressed = true;

     //Serial.print("X="); Serial.print(analogLH); Serial.print(" Y="); Serial.println(analogLV);

     for (int i = 0; i < sizeof(Buttons); i++) {
        if (digitalRead(Buttons[i]) == LOW) {
            Joystick.pressButton(i);
            buttonPressed = true;
        }
        else
            Joystick.releaseButton(i);
     }

    if (digitalRead(BTN_SELECT) == LOW) {
        if (digitalRead(DPAD_UP) == LOW && (volumeUpTime == 0 || millis() - volumeUpTime >= REPEAT_DELAY)) {
            volumeUpTime = millis();
            volumeDownTime = 0;
            Serial.println("VOLUME+");
        }
        else if (digitalRead(DPAD_DOWN) == LOW && (volumeDownTime == 0 || millis() - volumeDownTime >= REPEAT_DELAY)) {
            volumeUpTime = 0;
            volumeDownTime = millis();
            Serial.println("VOLUME-");
        }
    }
    if (digitalRead(DPAD_UP) == HIGH && millis() - volumeUpTime >= DEBOUNCE_DELAY) {
        volumeUpTime = 0;
    }
    if (digitalRead(DPAD_DOWN) == HIGH && millis() - volumeDownTime >= DEBOUNCE_DELAY) { 
        volumeDownTime = 0;
    }

    /*
    if (analogRead(POWER_KEY) < 50) {
        if (powerKeyTime == 0)
            powerKeyTime = millis();
        else if (millis() - powerKeyTime >= POWER_KEY_DELAY) {
            shutdown();
            powerKeyTime = 0;
        }
        buttonPressed = true;
    } else if (millis() - powerKeyTime >= DEBOUNCE_DELAY) {
        powerKeyTime = 0;
    }
    */
    digitalWrite(LED, !buttonPressed);
    Joystick.sendState();
    readBattery();

    if (buttonPressed)
        lastInputTime = millis();
//    else if (millis() - lastInputTime >= AUTO_SHUTDOWN_TIME) {
//        shutdown();
//        lastInputTime += POWER_KEY_DELAY;       // Repeat as if the power key was held down
//    }

    /*
    if (shutdownTime > 0 && millis() - shutdownTime >= POWEROFF_DELAY) {
        // Send double press on power key to cut power off entirely
        digitalWrite(LED, HIGH);
        pinMode(POWER_KEY, OUTPUT);
        digitalWrite(POWER_KEY, LOW);
        delay(100);
        digitalWrite(LED, LOW);
        pinMode(POWER_KEY,INPUT);
        delay(200);
        digitalWrite(LED, HIGH);
        pinMode(POWER_KEY, OUTPUT);
        digitalWrite(POWER_KEY, LOW);
        delay(100);
        digitalWrite(LED, LOW);
        pinMode(POWER_KEY,INPUT);
        shutdownTime = 0;
    }
    */
}

/*
void shutdown() {
    shutdownTime = millis();
    // TODO? Serial.println("POWEROFF");
}
*/

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

