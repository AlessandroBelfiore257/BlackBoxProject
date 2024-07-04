#include "BluetoothSerial.h"
#include "ELMduino.h"

BluetoothSerial SerialBT;
#define ELM_PORT SerialBT
#define DEBUG_PORT Serial

ELM327 myELM327;

uint8_t remoteAddres[] = { 0x10, 0x21, 0x3E, 0x4C, 0x6F, 0x3A };

typedef enum { ENG_RPM,
               SPEED,
               THROTTLE,
               LOAD,  
               COOLANT_TEMP,    
               TORQUE,         
               BATTERY_VOLTAGE,  
               OIL_TEMP,        
               FUEL_LEVEL } obd_pid_states;
obd_pid_states obd_state = ENG_RPM;

float rpm = 0;
float mph = 0;
float throttle = 0;
float load = 0;
float coolant_temp = 0;
float torque = 0;
float battery_voltage = 0;
float oil_temp = 0;
float fuel_level = 0;

void setup()
{
    DEBUG_PORT.begin(115200);
    ELM_PORT.begin("ArduHUD", true);
    SerialBT.setPin("1234");

    if (!ELM_PORT.connect(remoteAddres))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
        while (1)
            ;
    }

    if (!myELM327.begin(ELM_PORT, true, 2000))
    {
        DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 2");
        while (1)
            ;
    }

    DEBUG_PORT.println("Connected to ELM327");
}

void loop()
{
    switch (obd_state)
    {
        case ENG_RPM:
        {
            rpm = myELM327.rpm();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("RPM: ");
                DEBUG_PORT.println(rpm);
                obd_state = SPEED;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = SPEED;
            }
            break;
        }

        case SPEED:
        {
            mph = myELM327.mph();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Speed (mph): ");
                DEBUG_PORT.println(mph);
                obd_state = THROTTLE;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = THROTTLE;
            }
            break;
        }

        case THROTTLE:
        {
            throttle = myELM327.throttle();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Throttle (%): ");
                DEBUG_PORT.println(throttle);
                obd_state = LOAD;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = LOAD;
            }
            break;
        }

        case LOAD:
        {
            load = myELM327.engineLoad();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Engine Load (%): ");
                DEBUG_PORT.println(load);
                obd_state = COOLANT_TEMP;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = COOLANT_TEMP;
            }
            break;
        }

        case COOLANT_TEMP:
        {
            coolant_temp = myELM327.engineCoolantTemp();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Coolant Temp (C): ");
                DEBUG_PORT.println(coolant_temp);
                obd_state = TORQUE;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = TORQUE;
            }
            break;
        }

        case TORQUE:
        {
            torque = myELM327.torque();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Torque (Nm): ");
                DEBUG_PORT.println(torque);
                obd_state = BATTERY_VOLTAGE;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = BATTERY_VOLTAGE;
            }
            break;
        }

        case BATTERY_VOLTAGE:
        {
            battery_voltage = myELM327.batteryVoltage();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Battery Voltage (V): ");
                DEBUG_PORT.println(battery_voltage);
                obd_state = OIL_TEMP;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = OIL_TEMP;
            }
            break;
        }

        case OIL_TEMP:
        {
            oil_temp = myELM327.oilTemp();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Oil Temp (C): ");
                DEBUG_PORT.println(oil_temp);
                obd_state = FUEL_LEVEL;
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = FUEL_LEVEL;
            }
            break;
        }

        case FUEL_LEVEL:
        {
            fuel_level = myELM327.fuelLevel();
            if (myELM327.nb_rx_state == ELM_SUCCESS)
            {
                DEBUG_PORT.print("Fuel Level (%): ");
                DEBUG_PORT.println(fuel_level);
                obd_state = ENG_RPM;  // Riparti dal primo stato
            }
            else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
            {
                myELM327.printError();
                obd_state = ENG_RPM;  // Riparti dal primo stato
            }
            break;
        }

        default:
            DEBUG_PORT.println("Unknown state.");
            obd_state = ENG_RPM;
            break;
    }
} 
