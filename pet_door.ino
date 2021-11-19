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
State currentState;

byte readCard[4];
String MasterTag;
String tagID;
int tagFound;
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
    MasterTag = "E781DAC6"; // REPLACE this Tag ID with your Tag ID!!!
    tagID = "";
    tagFound = false;
    limitUpClosed = false;
    limitDownClosed = false;
    dogPresent = false;
    timeForWait = 10; // 10 seconds
    timeForWait = timeForWait * 1000;
    startTimer = true;
    currentState = HOME;
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

    switch (currentState)
    {
    case HOME:
        homeState();
        break;
    case READ_TAG:
        readTagState();
        break;
    case UP:
        upState();
        break;
    case WAIT:
        waitState();
        break;

    default:
        break;
    }
    delay(2);
}

void updateInputs()
{
    limitDownClosed = digitalRead(DOWN_SENSOR);
    limitUpClosed = digitalRead(UP_SENSOR);
    dogPresent = !digitalRead(IR_SENSOR);
}

void homeState()
{
    Serial.println("Home state");
    if (dogPresent)
    {
        currentState = UP;
        return;
    }

    if (limitDownClosed)
    {
        currentState = READ_TAG;
        return;
    }
    myStepper.step(step);
    step++;
}

void readTagState()
{
    Serial.println("Read Tag state");
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }
    mfrc522.PICC_HaltA(); // Stop reading
    currentState = UP;
}

void upState()
{
    Serial.println("Up state");
    if (limitUpClosed)
    {
        currentState = WAIT;
        return;
    }
    myStepper.step(-step);
    step++;
}

void waitState()
{
    Serial.println("Wait state");
    if (startTimer)
    {
        timeNow = millis();
        startTimer = false;
    }
    if (millis() >= timeNow + timeForWait)
    {
        currentState = HOME;
        startTimer = true;
    }
    else
    {
        Serial.println("Waitting");
    }
}
