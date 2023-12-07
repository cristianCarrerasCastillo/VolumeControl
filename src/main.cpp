// code by Chatgpt
#include <Encoder.h>
#include <HID-Project.h>

// Definir los pines del encoder
#define encoderPinA 3
#define encoderPinB 2
#define encoderBtn 4

Encoder myEncoder(encoderPinA, encoderPinB);
int lastValueEncoder = 0;

void setup() {
  Serial.begin(9600);
  pinMode(encoderPinA, INPUT_PULLUP);
  pinMode(encoderPinB, INPUT_PULLUP);
  pinMode(encoderBtn, INPUT_PULLUP);

  Consumer.begin(); // Inicializa el emulador de dispositivo de control de volumen
}

void loop() {
  // Lee el encoder
  int encoderValue = myEncoder.read();
  int btn = digitalRead(encoderBtn);

  
  // Emula el control de volumen
  if (encoderValue > lastValueEncoder) {
    Consumer.write(MEDIA_VOLUME_UP);
    Serial.println("subir volumen");
  } else if (encoderValue < lastValueEncoder) {
    Consumer.write(MEDIA_VOLUME_DOWN);
    Serial.println("bajar volumen");
  }
  if(!btn){
    Consumer.write(MEDIA_VOL_MUTE);
    Serial.println("mute");
    delay(50); // para mejor experiencia de usuario
  }
  lastValueEncoder = encoderValue;
  // Espera un breve momento para evitar lecturas demasiado rápidas
  delay(50);
}
