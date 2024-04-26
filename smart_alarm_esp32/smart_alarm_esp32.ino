#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID     "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define LDR_CHAR_UUID    "a6cf17cc-bc80-4d73-b5d6-f0f58935551d"
#define THRES_CHAR_UUID  "b9be30bc-de11-47e7-bdf7-5318c22a2ffc"
#define ACTIVE_CHAR_UUID "17729b1d-bfe4-4c9e-8585-030ef373d043"

#define DEVICE_NAME      "ESP32"

BLECharacteristic* ldrChar;
BLECharacteristic* thresChar;
BLECharacteristic* activeChar;

typedef const uint8_t pin; 

pin redLedPin = 33;
pin greenLedPin = 14;
pin trigPin = 13; 
pin echoPin = 12; 
pin buzzerPin = 26;
pin ldrPin = 25; 

TaskHandle_t Task1; 

const uint8_t toneLength = 4; 
uint toneArr[toneLength] = {100, 100, 100, 100};

bool isActive = false; 
uint16_t threshold = 0;
uint distance; 
unsigned long endTime = 0; 

void blink(pin ledPin, uint8_t count) {
    digitalWrite(ledPin, HIGH); 
    delay(250);
    digitalWrite(ledPin, LOW);
}

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    blink(greenLedPin, 3);
  }

};

class ThresholdCharactericticCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    String value = pCharacteristic->getValue().c_str(); 
    Serial.println(value);
    threshold = value.toInt();
  }
  void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    // uint16_t ldrValue = analogRead(ldrPin);
    // String ldrValue2 = String(ldrValue);
  }
};


class LdrCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    uint16_t ldrValue = analogRead(ldrPin);


    pCharacteristic->setValue(String(ldrValue).c_str());
  }
};

class ActiveCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    Serial.println("adasda"); 
    Serial.println(String(pCharacteristic->getValue().c_str()));
    isActive = String(pCharacteristic->getValue().c_str()) == "1";
  }
  void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param) {
    Serial.println("read");
  }
};

void beepTask(void *pvParameters) {
  while (true) {
    for (uint8_t i = 0; i < toneLength; i++) {
      for (uint8_t j = 0; j < toneArr[i]; j++) {
        digitalWrite(buzzerPin, HIGH); 
        delayMicroseconds(30);
        digitalWrite(buzzerPin, LOW); 
        delayMicroseconds(30); 
      }
      delay(200); 
    }
    delay(500);
  }
}


template<class Callbacks>
BLECharacteristic* createCharacteristic(char* CHAR_UUID, BLEService* pService, char* initialValue) {
  BLECharacteristic *characteristic = pService->createCharacteristic(CHAR_UUID, 
                                                                     BLECharacteristic::PROPERTY_READ | 
                                                                     BLECharacteristic::PROPERTY_WRITE);
  characteristic->setCallbacks(new Callbacks());
  characteristic->setValue(initialValue);
  return characteristic; 
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT); 
  pinMode(ldrPin, INPUT);

  BLEDevice::init(DEVICE_NAME);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  ldrChar = createCharacteristic<LdrCharacteristicCallbacks>(LDR_CHAR_UUID, pService, "0");
  thresChar = createCharacteristic<ThresholdCharactericticCallbacks>(THRES_CHAR_UUID, pService, "0");
  activeChar = createCharacteristic<ActiveCharacteristicCallbacks>(ACTIVE_CHAR_UUID, pService, "0");

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

}

void loop() {
  if (isActive && analogRead(ldrPin) >= threshold) {
    digitalWrite(greenLedPin, HIGH); 

    xTaskCreatePinnedToCore(beepTask, "Beep", 1000, NULL, 0, &Task1, 1);
    uint8_t counter = 0; 
    while (isActive && counter < 5) {
      digitalWrite(trigPin, HIGH);  
      delayMicroseconds(10); 
      digitalWrite(trigPin, LOW); 

      distance = pulseIn(echoPin, HIGH) / 170;
    
      if (distance < 5) {
        counter++; 
        delay(200);
      } else {
        counter = 0; 
        delay(1000); 
      }
    }

    if (!isActive || counter == 5) {
      vTaskDelete(Task1); 
      activeChar->setValue("0"); 
      isActive = false;
      digitalWrite(greenLedPin, LOW);
    }
  }

  delay(2000);
}