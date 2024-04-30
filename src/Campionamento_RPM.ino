#include "BluetoothSerial.h"
#include "ELMduino.h"
#include <TaskScheduler.h>

BluetoothSerial SerialBT;
Scheduler scheduler;

#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial

const int THESHOLD = 3000;

void rilevoVelCallback();

Task rilevoVelTask(10000, TASK_FOREVER, &rilevoVelCallback);

ELM327 myELM327;

float rpm = 0;
float registeredRPM = 0;
// Numero di volte in cui eccedo i 500 g/min
int countSuperior = 0;
int countTOT = 0;

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
  
  scheduler.init();
  scheduler.addTask(rilevoVelTask);
  rilevoVelTask.enable();
}


void loop()
{
  rpm = myELM327.rpm();
    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
      registeredRPM = rpm;
      DEBUG_PORT.println(rpm);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
      myELM327.printError();
    }
    scheduler.execute();
}

void rilevoVelCallback() {
    countTOT++;
    if(registeredRPM > THESHOLD) {
          countSuperior++;
    }
    DEBUG_PORT.printf("############################# Numero di volte in cui eccedo gli rpm: %d #############################", countSuperior);
    DEBUG_PORT.printf("############################# Numero totale di campionamenti: %d #############################", countTOT);
    DEBUG_PORT.print("Score: 0 -> ottimo | 1 -> pessimo: ");
    DEBUG_PORT.println((float)(countSuperior) / (float)(countTOT));
}
