#include <Arduino.h>
#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

int ledPin = 5;       // the number of the output pin

int state = HIGH;      // the current state of the led pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin

/**** Configure the nrf24l01 CE and CS pins ****/
RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

/**
   User Configuration: nodeID - A unique identifier for each radio. Allows addressing
   to change dynamically with physical changes to the mesh.
   In this example, configuration takes place below, prior to uploading the sketch to the device
   A unique value from 1-255 must be configured for each node.
   This will be stored in EEPROM on AVR devices, so remains persistent between further uploads, loss of power, etc.
 **/
#define nodeID 3


uint32_t displayTimer = 0;

struct payload_t {
  unsigned long ms;
  unsigned long counter;
};


void setup() {
  
  pinMode(ledPin, OUTPUT);

  Serial.begin(9600);
  //printf_begin();
  // Set the nodeID manually
  mesh.setNodeID(nodeID);
  // Connect to the mesh
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();
  Serial.println("Connected?");
}

void toggleLED() {
  if (state == HIGH)
      state = LOW;
    else
      state = HIGH;
}

void loop() {

  mesh.update();

  // Send to the master node every second
  if (millis() - displayTimer >= 1000) {
    displayTimer = millis();

    // Send an 'M' type message containing the current millis()
    if (!mesh.write(&displayTimer, 'M', sizeof(displayTimer))) {

      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println("Renewing Address");
        if(!mesh.renewAddress()){
          //If address renewal fails, reconfigure the radio and restart the mesh
          //This allows recovery from most if not all radio errors
          mesh.begin();
        }
      } else {
        Serial.println("Send fail, Test OK");
      }
    } else {
      Serial.print("Send OK: "); Serial.println(displayTimer);
    }
  }

  while (network.available()) {
    RF24NetworkHeader header;
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    Serial.print("Received packet #");
    Serial.print(payload.counter);
    Serial.print(" at ");
    Serial.println(payload.ms);

    if( payload.counter > 0) {
      Serial.println("Light on");
      digitalWrite(ledPin, HIGH);
    } else {
      Serial.println("Light off");
      digitalWrite(ledPin, LOW);
    }
    
  }
  //reading = digitalRead(inPin);



  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW) {
    
    toggleLED();
    
  }

  //digitalWrite(ledPin, state);

  previous = reading;

}