#include <WiFi.h>
#include <WiFiClient.h>
#include <Temboo.h>
#include "Home.h"
#include <SPI.h>
#include <EEPROM.h>
#include <boards.h>
#include <RBL_nRF8001.h>
#include <Servo.h> 

Servo myservo;
WiFiClient client;
int maxCalls = 10;
int calls = 0;

void setup() {
  Serial.begin(9600);
//  int wifiStatus = WL_IDLE_STATUS;
//  
//  while(wifiStatus != WL_CONNECTED) {
//    Serial.print("WiFi:");
//    wifiStatus = WiFi.begin(WIFI_SSID, WPA_PASSWORD);
//
//    if (wifiStatus == WL_CONNECTED)
//      Serial.println("OK");
//    else
//      Serial.println("FAIL");
//    delay(1000);
//  }
  
  ble_set_name("Noah Home");
  ble_begin();
  pinMode(DIGITAL_OUT_PIN, OUTPUT);
  pinMode(DIGITAL_IN_PIN, INPUT);
  
  // Default to internally pull high, change it if you need
  digitalWrite(DIGITAL_IN_PIN, HIGH);
  //digitalWrite(DIGITAL_IN_PIN, LOW);
  
//  pinMode(DESELECT_SD, OUTPUT);
//  digitalWrite(DESELECT_SD, HIGH);
  myservo.attach(SERVO_PIN);
  myservo.write(0);
  Serial.println("Setup complete.\n");
}

void loop()
{
  static boolean analog_enabled = false;
  static byte old_state = LOW;
    
  // If data is ready
  while(ble_available()) {
    // read out command and data
    byte data0 = ble_read();
    byte data1 = ble_read();
    byte data2 = ble_read();
    
    if (data0 == 0x01) {  // control digital out pin
      if (data1 == 0x01)
        digitalWrite(DIGITAL_OUT_PIN, HIGH);
      else
        digitalWrite(DIGITAL_OUT_PIN, LOW);
    }
    else if (data0 == 0xA0) { // enable analog in reading
      if (data1 == 0x01)
        analog_enabled = true;
      else
        analog_enabled = false;
    }
    else if (data0 == 0x02) { // PWM pin
      analogWrite(PWM_PIN, data1);
    }
    else if (data0 == 0x03) { // Servo pin
      myservo.write(data1);
    }
    else if (data0 == 0x04) {
      analog_enabled = false;
      myservo.write(0);
      analogWrite(PWM_PIN, 0);
      digitalWrite(DIGITAL_OUT_PIN, LOW);
    }
  }
  
  if (analog_enabled) {
    int value = analogRead(ANALOG_IN_PIN); 
    float temperatureF = (((((analogRead(ANALOG_IN_PIN) * 5.0) / 1024.0) - 0.5) * 100) * 9.0 / 5.0) + 32.0;
    
    ble_write(0x0B);
    ble_write(value >> 8);
    ble_write(temperatureF);   
  }
  
  // If digital in changes, report the state
  if (digitalRead(DIGITAL_IN_PIN) != old_state) {
    old_state = digitalRead(DIGITAL_IN_PIN);
    
    if (digitalRead(DIGITAL_IN_PIN) == HIGH) {
      ble_write(0x0A);
      ble_write(0x01);
      ble_write(0x00);
    }
    else {
      ble_write(0x0A);
      ble_write(0x00);
      ble_write(0x00);
    }
  }
  
  if (!ble_connected()) {
    analog_enabled = false;
    digitalWrite(DIGITAL_OUT_PIN, LOW);
    
    if (digitalRead(DIGITAL_IN_PIN) != old_state) {
      old_state = digitalRead(DIGITAL_IN_PIN);
    
      if (digitalRead(DIGITAL_IN_PIN) == HIGH) {
        if (!ble_connected()) {
          Serial.println("Send Email Test.\n");
          delay(100);
//          runSendEmail(old_state);
          calls++;
        }
      }
    }
  }
  
  ble_do_events();  
}

void runSendEmail(int sensorValue) {
  TembooChoreo SendEmailChoreo(client);

  SendEmailChoreo.setAccountName(TEMBOO_ACCOUNT);
  SendEmailChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  SendEmailChoreo.setAppKey(TEMBOO_APP_KEY);

  String MessageBodyValue = "Arduino";
  SendEmailChoreo.addInput("MessageBody", MessageBodyValue);
  SendEmailChoreo.addInput("Subject", "Noah's Arduino");
  SendEmailChoreo.addInput("Username", "hob594spam@gmail.com");
  SendEmailChoreo.addInput("Password", "amdnydcceynupzks");
  SendEmailChoreo.addInput("FromAddress", "hob594spam@gmail.com");
  SendEmailChoreo.addInput("ToAddress", "hob594@gmail.com");

  SendEmailChoreo.setChoreo("/Library/Google/Gmail/SendEmail");
  unsigned int returnCode = SendEmailChoreo.run();
  while (SendEmailChoreo.available()) {
    char c = SendEmailChoreo.read();
    Serial.print(c);
  }
  Serial.println();
  SendEmailChoreo.close();
}

