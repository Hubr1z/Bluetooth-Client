//client
#include <BLEDevice.h>
#include <Arduino.h>

#define SERVICE_UUID_NODE1        "1de92c84-6aad-4260-9be2-126613bdb0c0"
#define CHARACTERISTIC_UUID_NODE1 "1de92c84-6aad-4260-9be2-126613bdb0c1"

#define SERVICE_UUID_NODE2        "4f056d6b-d747-42bf-87d3-275649d82520"
#define CHARACTERISTIC_UUID_NODE2 "4f056d6b-d747-42bf-87d3-275649d82521"


BLEClient*  pClient1;
BLEClient*  pClient2;
bool connected = false;

//BLE Server name (the other ESP32 name running the server sketch)
#define bleServerName1 "XIAOESP32C6_BLE_SENDER1"
#define bleServerName2 "XIAOESP32C6_BLE_SENDER2"

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress1;
static BLEAddress *pServerAddress2;

BLEUUID serviceUUID1(SERVICE_UUID_NODE1);
BLEUUID charUUID1(CHARACTERISTIC_UUID_NODE1);    
BLEUUID serviceUUID2(SERVICE_UUID_NODE2);
BLEUUID charUUID2(CHARACTERISTIC_UUID_NODE2);    

uint8_t receivedData[8];

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks1: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName1) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress1 = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      Serial.println("Device 1 found. Connecting!");
    }
  }
};

class MyAdvertisedDeviceCallbacks2: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName2) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress2 = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      Serial.println("Device 2 found. Connecting!");
    }
  }
};

void setup() {
  memset(receivedData, 0, sizeof(receivedData));

  Serial.begin(115200);
  
  Serial.println("Starting BLE client...");

  BLEDevice::init("XIAOESP32C6_Client");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan1 = BLEDevice::getScan();
  pBLEScan1->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks1());
  pBLEScan1->setActiveScan(true);
  pBLEScan1->start(30);

  pClient1 = BLEDevice::createClient();
  // Connect to the remove BLE Server1.
  pClient1->connect(*pServerAddress1);
  Serial.println(" - Connected to server1");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService1 = pClient1->getService(serviceUUID1);
  if (pRemoteService1 == nullptr) {
    Serial.print("Failed to find our service UUID1: ");
    Serial.println(serviceUUID1.toString().c_str());
    return;
  }

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  BLERemoteCharacteristic* pCharacteristic1 = pRemoteService1->getCharacteristic(charUUID1);
  if (pCharacteristic1 == nullptr) {
    Serial.print("Failed to find our characteristic UUID1");
    return;
  }

  pCharacteristic1->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData1, size_t length, bool isNotify) {
    Serial.println("Notify received");
    Serial.print("Value: ");
    Serial.println(*pData1);
  });

//Same codes but for a second client to connect to second server.
  BLEScan* pBLEScan2 = BLEDevice::getScan();
  pBLEScan2->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks2());
  pBLEScan2->setActiveScan(true);
  pBLEScan2->start(30);

  pClient2 = BLEDevice::createClient();
  // Connect to the remove BLE Server2.
  pClient2->connect(*pServerAddress2);
  Serial.println(" - Connected to server2");

  BLERemoteService* pRemoteService2 = pClient2->getService(serviceUUID2);
  if (pRemoteService2 == nullptr) {
    Serial.print("Failed to find our service UUID2: ");
    Serial.println(serviceUUID2.toString().c_str());
    return;
  }
  
  BLERemoteCharacteristic* pCharacteristic2 = pRemoteService2->getCharacteristic(charUUID2);
  if (pCharacteristic2 == nullptr) {
    Serial.print("Failed to find our characteristic UUID2");
    return;
  }

  pCharacteristic2->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData2, size_t length, bool isNotify) {
    Serial.println("Notify received");
    Serial.print("Value: ");
    Serial.println(*pData2);
  });
  
  connected = true;
}
BLERemoteService* pRemoteService1;
BLERemoteCharacteristic* pCharacteristic;
void loop() {
  if (connected) {
    pRemoteService1 = pClient1->getService(serviceUUID1);
    pCharacteristic = pRemoteService1->getCharacteristic(charUUID1);
    pCharacteristic->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData1, size_t length, bool isNotify) {
      Serial.println("Notify received");
      memcpy(receivedData, pData1, 8);
      Serial.println("Received 8-byte data:");
    for (int i = 0; i < 8; i++) {
      Serial.print("Byte ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(receivedData[i]);  // Print in HEX format
    }
    });
    BLERemoteService* pRemoteService2 = pClient2->getService(serviceUUID2);
    BLERemoteCharacteristic* pCharacteristic2 = pRemoteService2->getCharacteristic(charUUID2);
    pCharacteristic->registerForNotify([](BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData2, size_t length, bool isNotify) {
      Serial.println("Notify received");
      Serial.print("Value: ");
      Serial.println(*pData2);
    });
  } 
  delay(1000);
}