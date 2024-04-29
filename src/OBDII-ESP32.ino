#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;

#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial


ELM327 myELM327;


uint32_t rpm = 0;

// Indirizzo MAC del dispositivo Bluetooth a cui ci si desidera connettere
// Nel nostro caso OBDII: 1C:A1:35:69:8D:C5
uint8_t remoteAddress[] = {0x1C, 0xA1, 0x35, 0x69, 0x8D, 0xC5};

void setup()
{
#if LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  DEBUG_PORT.begin(115200);
  //SerialBT.setPin("1234");
  ELM_PORT.begin("ESP_BLUETOOTH", true); // Master 
  DEBUG_PORT.println("Attempting to connect to ELM327...");
  
  if (!ELM_PORT.connect(remoteAddress))
  { 
    DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
    while(1);
  }

  if (!myELM327.begin(ELM_PORT, true, 2000))
  {
    Serial.println("Couldn't connect to OBD scanner - Phase 2");
    while (1);
  }

  Serial.println("Connected to ELM327");
}


void loop()
{
  float tempRPM = myELM327.rpm();

  if (myELM327.nb_rx_state == ELM_SUCCESS)
  {
    rpm = (uint32_t)tempRPM;
    Serial.print("RPM: "); Serial.println(rpm);
  }
  else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    myELM327.printError();

}
