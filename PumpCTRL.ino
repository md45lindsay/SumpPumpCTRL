/*
   PURPOSE: Read distance, temperature and humidity (optional) and
            calculate the height of water in a sump bassin and
            control a relay to activate a pump.

            Step 1, distance, temp/hum and relay working
            Step 2, I2C comms working with UNO
            Step 3, Add LCD
            Step 4, Add BLE

   BOARD:   Pro Trinket 5V USB
*/

#include <RunningAverage.h>
#include <DHT.h>

//Variables
const int pinDistTrigger = 4;
const int pinDistEcho = 5;
const int pinTempHumi = 6;
const int pinRelay = 13; //Will be '8' but 13 for now
float temperature = 0;
float humidity = 0;
int timeToWait = 10;  // In seconds
String pumpStatus = "OFF";
int pumpHighcm = 55;            //Sensor reading when High level water
int pumpLowcm = 65;             //Sensor reading when Low level water
int tankHeightcm = 58;          //Tank height
int dischargeHeightcm = 292;    //Pump discharge height
unsigned long tempLastRun, tempDelay = 600001;
float distanceAdj;

RunningAverage distances(5);
DHT TempHumi(pinTempHumi, DHT11);


/*
   DHT Sensor reading
*/

void getEnvironment() {
  if (tempDelay > 600000) {
    humidity = TempHumi.readHumidity();
    temperature = TempHumi.computeHeatIndex(TempHumi.readTemperature(), humidity, false);
    Serial.println(String(temperature) + "C @ " + String(humidity) + "%");
    tempDelay = 0;
    tempLastRun = millis();
  }
  else {
    tempDelay = millis() - tempLastRun;
  }
//  scheduleTimer1Task(&getEnvironment, NULL, 60000);
}

/*
   Distance reading adjusted by environment variables
*/

float getDistance() {
  // This function takes 5 measurements from the sensor
  // and average the distance

  int loopCount = 0;      // Reset loop counter
  float echoDuration = 0;   // Initiate the echo duration variable

  distances.clear();    // Clear distance array

  // Get average echo time from distance sensor
  while (loopCount < 5) {
    // Clears the relayPin, trigPin and echoPin
    digitalWrite(pinDistTrigger, LOW);
    digitalWrite(pinDistEcho, LOW);
    delayMicroseconds(2);

    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(pinDistTrigger, HIGH);
    delayMicroseconds(10);
    digitalWrite(pinDistTrigger, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    echoDuration = pulseIn(pinDistEcho, HIGH);
    distances.addValue(echoDuration);   // Add the value in the distance array
    loopCount ++;   // Increment the loop counter
    delay (60);     // Delay 60ms per reading as per sensor vendor specs
  }
  //Adjust distance with environment data
  float velocity = 331.4 + 0.6 * temperature + 0.0124 * humidity;   //Standardized velocity formula including temperature (C) and humidity (%)
  return velocity * long(distances.getAverage()) / 1000 / 10 / 2;    // Return distance (cm) = Velocity * Duration (us) / 1000 (for ms) / 10 (for cm) / 2 (for 1 way trip)
}

/*
   Tank Status
*/
void tankStatus() {
  //Tank status
  //Tank is full
  if ( distanceAdj <= pumpHighcm ) {
    pumpStatus = "ON";
  }
  //Tank is empty
  if ( distanceAdj >= pumpLowcm ) {
    pumpStatus = "OFF";
  }
}

/*
   Pump Status
*/
void pumpRelay() {
  //Pump Relay
  //Start pump
  if ( pumpStatus == "ON") {
    digitalWrite(pinRelay, HIGH);
  }

  //Stop pump
  if (pumpStatus == "OFF") {
    digitalWrite(pinRelay, LOW);
  }
}

/*
 * Prepare the data and send
 */
void prepDataSend() {
  
}

/*
   Setup & loop
*/
void setup() {
  //Setup the HC-SR04
  pinMode(pinDistTrigger, OUTPUT);   //Set trigger pin
  pinMode(pinDistEcho, INPUT);    //Set echo pin

  // Setup Relay board
  pinMode(pinRelay, OUTPUT);  //Set relay pin
  digitalWrite(pinRelay, LOW);

  //Setup DHT
  TempHumi.begin();
  getEnvironment();   // First time data gathering needed for 1st distance reading adjustment
}

void loop() {
  getEnvironment();  // Get environment variables (if internal function counter lasted more than 10 min)
  distanceAdj = getDistance();  // Get echo duration & Calculating the distance adjusted with environment variables
  tankStatus();     // Verify water height vs tank limits
  pumpRelay();      // Toggle relay depending of water status vs tank
  prepDataSend();       // Format data and send/display
//Sending data through I2C
  // Want to develop variable delay. Slower when tank level low, higher when tank level high and during discharge
  delay(timeToWait * 1000);
}



