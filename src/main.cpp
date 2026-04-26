#include <Encoder.h>
#include <HID-Project.h>

constexpr uint8_t  PIN_ENC_A        = 3;
constexpr uint8_t  PIN_ENC_B        = 2;
constexpr uint8_t  PIN_BTN          = 4;

constexpr uint16_t DEBOUNCE_MS      = 20;
constexpr uint16_t LONG_PRESS_MS    = 600;
constexpr uint16_t DOUBLE_CLICK_MS  = 350;
constexpr int8_t   TICKS_PER_DETENT = 4;
constexpr uint8_t  ACCEL_DETENTS    = 3;
constexpr uint16_t ACCEL_WINDOW_MS  = 200;

// ── Encoder ──────────────────────────────────────────────────────────────────

Encoder myEncoder(PIN_ENC_A, PIN_ENC_B);
long          encLast    = 0;
uint8_t       accelCount = 0;
unsigned long accelLast  = 0;

void handleEncoder() {
    long raw  = myEncoder.read();
    long diff = raw - encLast;
    if (abs(diff) < TICKS_PER_DETENT) return;

    int detents = (int)(diff / TICKS_PER_DETENT);
    encLast += (long)detents * TICKS_PER_DETENT;

    unsigned long now = millis();
    if (now - accelLast < ACCEL_WINDOW_MS)
        accelCount += (uint8_t)abs(detents);
    else
        accelCount = (uint8_t)abs(detents);
    accelLast = now;

    int step  = (accelCount >= ACCEL_DETENTS) ? 2 : 1;
    int count = abs(detents) * step;

    ConsumerKeycode key = (detents > 0) ? MEDIA_VOLUME_UP : MEDIA_VOLUME_DOWN;
    for (int i = 0; i < count; i++) Consumer.write(key);

    #ifdef DEBUG
    Serial.print(detents > 0 ? "UP x" : "DOWN x");
    Serial.println(count);
    #endif
}

// ── Button ───────────────────────────────────────────────────────────────────

enum BtnState : uint8_t { BTN_IDLE, BTN_PRESSED, BTN_WAIT_DOUBLE, BTN_LONG_FIRED, BTN_CONSUMED };

BtnState      btnState   = BTN_IDLE;
bool          btnRaw     = HIGH;
bool          btnStable  = HIGH;
bool          btnPrev    = HIGH;
unsigned long debounceAt = 0;
unsigned long btnAt      = 0;

bool debounce() {
    bool raw = (bool)digitalRead(PIN_BTN);
    if (raw != btnRaw) { btnRaw = raw; debounceAt = millis(); }
    if (millis() - debounceAt >= DEBOUNCE_MS) btnStable = raw;
    return btnStable;
}

void handleButton() {
    bool cur       = debounce();
    bool onPress   = (cur == LOW)  && (btnPrev == HIGH);
    bool onRelease = (cur == HIGH) && (btnPrev == LOW);
    btnPrev = cur;

    unsigned long now = millis();

    switch (btnState) {
        case BTN_IDLE:
            if (onPress) { btnState = BTN_PRESSED; btnAt = now; }
            break;

        case BTN_PRESSED:
            if (now - btnAt >= LONG_PRESS_MS) {
                Consumer.write(MEDIA_PLAY_PAUSE);
                btnState = BTN_LONG_FIRED;
                #ifdef DEBUG
                Serial.println("LONG: play/pause");
                #endif
            } else if (onRelease) {
                btnState = BTN_WAIT_DOUBLE;
                btnAt = now;
            }
            break;

        case BTN_LONG_FIRED:
            if (onRelease) btnState = BTN_IDLE;
            break;

        case BTN_WAIT_DOUBLE:
            if (onPress) {
                Consumer.write(MEDIA_NEXT);
                btnState = BTN_CONSUMED;
                #ifdef DEBUG
                Serial.println("DOUBLE: next track");
                #endif
            } else if (now - btnAt >= DOUBLE_CLICK_MS) {
                Consumer.write(MEDIA_VOL_MUTE);
                btnState = BTN_IDLE;
                #ifdef DEBUG
                Serial.println("SHORT: mute");
                #endif
            }
            break;

        case BTN_CONSUMED:
            if (onRelease) btnState = BTN_IDLE;
            break;
    }
}

// ── Setup & Loop ─────────────────────────────────────────────────────────────

void setup() {
    #ifdef DEBUG
    Serial.begin(9600);
    #endif

    pinMode(PIN_BTN, INPUT_PULLUP);
    Consumer.begin();
    delay(500); // wait for USB enumeration before accepting input
}

void loop() {
    handleEncoder();
    handleButton();
}
