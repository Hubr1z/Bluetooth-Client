//client
#include <BLEDevice.h>
#include <Arduino.h>

#define SERVICE_UUID_NODE1        "1de92c84-6aad-4260-9be2-126613bdb0c0"
#define CHARACTERISTIC_UUID_NODE1 "1de92c84-6aad-4260-9be2-126613bdb0c1"

#define SERVICE_UUID_NODE2        "4f056d6b-d747-42bf-87d3-275649d82520"
#define CHARACTERISTIC_UUID_NODE2 "4f056d6b-d747-42bf-87d3-275649d82521"


//BLE Server name (the other ESP32 name running the server sketch)
#define bleServerName1 "XIAOESP32C6_BLE_SENDER1"
#define bleServerName2 "XIAOESP32C6_BLE_SENDER2"

// The remote service we wish to connect to.
static BLEUUID serviceUUID1(SERVICE_UUID_NODE1);
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID1(CHARACTERISTIC_UUID_NODE1);

static BLEUUID serviceUUID2(SERVICE_UUID_NODE2);
static BLEUUID charUUID2(CHARACTERISTIC_UUID_NODE2);

static boolean doConnect[2] = {false};
static boolean connected[2] = {false};
static boolean doScan[2] = {false};
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
      for (int i = 0; i < 8; i++){
      Serial.print(*(pData+i));
      Serial.print(",");
      }
  Serial.println();
}

class MyClientCallback : public BLEClientCallbacks {
  public:
  MyClientCallback(int i){
    j = i;
  };
  private:
  int j;
  void onConnect(BLEClient *pclient) {}

  void onDisconnect(BLEClient *pclient) {
    connected[j] = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer(BLEUUID *serviceUUID, BLEUUID *charUUID, int i) {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback(i));

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  Serial.println(" - Connected to server");
  pClient->setMTU(517);  //set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(*serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID -> toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(*charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID -> toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    String value = pRemoteCharacteristic->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P18);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P18);
  connected[i] = true;
  return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  public:
  MyAdvertisedDeviceCallbacks(BLEUUID *serviceUUID, int i){
    serviceUUID_Object = *serviceUUID;
    j = i;
  };
  private:
  BLEUUID serviceUUID_Object;
  int j;
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID_Object)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect[j] = true;
      doScan[j] = true;

    }  // Found our server

  }  // onResult
};  // MyAdvertisedDeviceCallbacks

void scanSetUp(BLEUUID *serviceUUID, int i) {
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(&(*serviceUUID), i));
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
};
// Retrieve a Scanner and set the callback we want to use to be informed when we
// have detected a new device.  Specify that we want active scanning and start the
// scan to run for 5 seconds. But now a function.

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  scanSetUp(&serviceUUID1, 0);


}  // End of setup.

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect[0] == true) {
    if (connectToServer(&serviceUUID1, &charUUID1, 0)) {
      Serial.println("We are now connected to the BLE Node 1.");
    } else {
      Serial.println("We have failed to connect to server 1; there is nothing more we will do.");
    }
    doConnect[0] = false;
  }


  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected[0]) {
    String newValue = "Time since boot: " + String(millis() / 1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");

    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } 
  else if (doScan[0]) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino

  }

  delay(1000);  // Delay a second between loops.
}  // End of loop