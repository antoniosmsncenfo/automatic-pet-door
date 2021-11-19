/*
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             49         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * More pin layouts for other boards can be found here: https://github.com/miguelbalboa/rfid#pin-layout
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Stepper.h>

#define SDA_DIO 53
#define RESET_DIO 49
const int DOWN_SENSOR = 3;
const int UP_SENSOR = 4;
const int IR_SENSOR = 5;
const int stepsPerRevolution = 2048; //this has been changed to 2048 for the 28BYJ-48
int step;
// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11); //note the modified sequence
/* Create an instance of the RFID library */
MFRC522 mfrc522(SDA_DIO, RESET_DIO);

enum State
{
    HOME,
    READ_TAG,
    UP,
    WAIT,
    NA
};
State nextState;
State currentState;

int limitUpClosed;
int limitDownClosed;
int dogPresent;
bool startTimer;
unsigned long timeForWait;
unsigned long timeNow;

void homeState();
void readTagState();
void upState();
void waitState();
void updateInputs();

void setup()
{
    limitUpClosed = false;
    limitDownClosed = false;
    dogPresent = false;
    timeForWait = 10; // 10 seconds
    timeForWait = timeForWait * 1000;
    startTimer = true;
    nextState = HOME;
    currentState = NA;
    step = 0;
    pinMode(DOWN_SENSOR, INPUT);
    pinMode(UP_SENSOR, INPUT);
    pinMode(IR_SENSOR, INPUT);

    // set the speed (needed to be reduced for the 28BYJ-48):
    myStepper.setSpeed(15);

    Serial.begin(115200);
    SPI.begin(); // Init SPI bus

    mfrc522.PCD_Init();                // Init MFRC522
    delay(4);                          // Optional delay. Some board do need more time after init to be ready, see Readme
    mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader details
}

void loop()
{
    updateInputs();

    switch (nextState)
    {
    case HOME:
        homeState();
        currentState = HOME;
        break;
    case READ_TAG:
        readTagState();
        currentState = READ_TAG;
        break;
    case UP:
        upState();
        currentState = UP;
        break;
    case WAIT:
        waitState();
        currentState = WAIT;
        break;

    default:
        break;
    }
    //delay(2);
}

void updateInputs()
{
    limitDownClosed = digitalRead(DOWN_SENSOR);
    limitUpClosed = digitalRead(UP_SENSOR);
    dogPresent = !digitalRead(IR_SENSOR);
}

void homeState()
{
    if (currentState != nextState)
        Serial.println("Home state");

    if (dogPresent)
    {
        nextState = UP;
        return;
    }

    if (limitDownClosed)
    {
        nextState = READ_TAG;
        return;
    }
    myStepper.step(step);
    step++;
}

void readTagState()
{
    if (currentState != nextState)
        Serial.println("Read Tag state");

    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }
    mfrc522.PICC_HaltA(); // Stop reading
    nextState = UP;
}

void upState()
{
    if (currentState != nextState)
        Serial.println("Up state");

    if (limitUpClosed)
    {
        nextState = WAIT;
        return;
    }
    myStepper.step(-step);
    step++;
}

void waitState()
{
    if (currentState != nextState)
        Serial.println("Wait state");

    if (startTimer)
    {
        timeNow = millis();
        startTimer = false;
    }
    if (millis() >= timeNow + timeForWait)
    {
        nextState = HOME;
        startTimer = true;
    }
    else
    {
        Serial.print("Waitting: ");
        Serial.println((timeNow + timeForWait - millis()) / 1000);
    }
}
