#include <Arduino.h>

#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>

int inPin = 2;         // the number of the input pin
int outPin = 3;       // the number of the output pin

int state = HIGH;      // the current state of the output pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin

// the follow variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastToggletime = 0;         // the last time the output pin was toggled
long debounce = 200;   // the debounce time, increase if the output flickers

/***** Configure the chosen CE,CS pins *****/
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

struct payload_t {
  unsigned long ms;
  unsigned long counter;
};

uint32_t ctr = 0;
uint32_t displayTimer = 0;

void setup() {
  Serial.begin(9600);

  pinMode(inPin, INPUT);
  pinMode(outPin, OUTPUT);

  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  Serial.println(mesh.getNodeID());
  // Connect to the mesh
  mesh.begin();
}

void toggleLED() {
  if (state == HIGH)
      state = LOW;
    else
      state = HIGH;
}

void loop() {

  // Call mesh.update to keep the network updated
  mesh.update();

  // In addition, keep the 'DHCP service' running on the master node so addresses will
  // be assigned to the sensor nodes
  mesh.DHCP();

  // Check for incoming data from the sensors
  if (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    Serial.print("Got ");
    uint32_t dat = 0;
    switch (header.type) {
      // Display the incoming millis() values from the sensor nodes
      case 'M':
        network.read(header, &dat, sizeof(dat));
        Serial.print(dat);
        Serial.print(" from RF24Network address 0");
        Serial.println(header.from_node, OCT);
        break;
      default:
        network.read(header, 0, 0);
        Serial.println(header.type);
        break;
    }
  }


  reading = digitalRead(inPin);

  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW && millis() - lastToggletime > debounce) {

    Serial.println("State change");
    
    toggleLED();

    Serial.print("New state: ");
    Serial.println(state);
    lastToggletime = millis(); 

    // Send state change to all nodes
    for (int i = 0; i < mesh.addrListTop; i++) {
      payload_t payload = {millis(), state};   
      RF24NetworkHeader header(mesh.addrList[i].address, OCT); //Constructing a header
      Serial.println( network.write(header, &payload, sizeof(payload)) == 1 ? F("Send OK") : F("Send Fail")); //Sending an message
    }

  }

  digitalWrite(outPin, state);

  previous = reading;

}