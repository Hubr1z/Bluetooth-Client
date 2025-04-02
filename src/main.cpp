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

#define SENSOR_THRESHOLD 80
// The remote service we wish to connect to.
static BLEUUID serviceUUID1(SERVICE_UUID_NODE1);
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID1(CHARACTERISTIC_UUID_NODE1);

static BLEUUID serviceUUID2(SERVICE_UUID_NODE2);
static BLEUUID charUUID2(CHARACTERISTIC_UUID_NODE2);

static boolean doConnect[2] = {false};
static boolean connected[2] = {false};
static boolean doScan[2] = {false};
static BLERemoteCharacteristic *pRemoteCharacteristic1, *pRemoteCharacteristic2;
static BLEAdvertisedDevice *myDevice1, *myDevice2;

static boolean data1State[8] = {false};
static boolean data2State[8] = {false};
static int data1[8] = {0};
static int data2[8] = {0};

static boolean doOutputFlag1 = false;
static boolean doOutputFlag2 = false;

int thresholdFunction(int *data, boolean *dataState) {
  int total = 0;
  for (int i = 0; i < 8; i++) {
    if (*(data + i) > SENSOR_THRESHOLD) {
      *(dataState + i) = 1;
    } else {
      *(dataState + i) = 0;
    }
  }
  for (int i = 0; i < 8; i++) {
    total += *(dataState + i);
  }
  return total;
}

void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  // // For gates debug
  // Serial.print("Notify callback for characteristic ");
  // Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  // Serial.print(" of data length ");
  // Serial.println(length);
  // Serial.print("data: ");
  //     for (int i = 0; i < 8; i++){
  //     Serial.print(*(pData+i));
  //     Serial.print(",");
  //     }
  // Serial.println();

  // Check the UUID to determine which device (Node 1 or Node 2) sent the notification
  if (pBLERemoteCharacteristic->getUUID().equals(charUUID1)) {
    // Node 1 - copy data to data1
    for (int i = 0; i < 8; i++) {
      data1[i] = pData[i];
      // If you want to track the state based on the threshold, you can do this:
      data1State[i] = (data1[i] > SENSOR_THRESHOLD);
    }
    doOutputFlag1 = true;
  }
  else if (pBLERemoteCharacteristic->getUUID().equals(charUUID2)) {
    // Node 2 - copy data to data2
    for (int i = 0; i < 8; i++) {
      data2[i] = pData[i];
      // If you want to track the state based on the threshold, you can do this:
      data2State[i] = (data2[i] > SENSOR_THRESHOLD);
    }
    doOutputFlag2 = true;
  }
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

bool connectToServer(BLEUUID *serviceUUID, BLEUUID *charUUID, BLERemoteCharacteristic **pRemoteCharacteristic, BLEAdvertisedDevice **myDevice, int i) {
  Serial.print("Forming a connection to ");
  Serial.println((*myDevice)->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback(i));

  // Connect to the remove BLE Server.
  pClient->connect(*myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
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
  (*pRemoteCharacteristic) = pRemoteService->getCharacteristic(*charUUID);
  if ((*pRemoteCharacteristic) == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID -> toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if ((*pRemoteCharacteristic)->canRead()) {
    String value = (*pRemoteCharacteristic)->readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  if ((*pRemoteCharacteristic)->canNotify()) {
    (*pRemoteCharacteristic)->registerForNotify(notifyCallback);
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
  MyAdvertisedDeviceCallbacks(BLEUUID *serviceUUID, BLEAdvertisedDevice **myDevice, int i){
    serviceUUIDObject = *serviceUUID;
    myDeviceObject = myDevice;
    j = i;
  };
  private:
  BLEUUID serviceUUIDObject;
  BLEAdvertisedDevice **myDeviceObject;
  int j;
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUIDObject)) {

      BLEDevice::getScan()->stop();
      *myDeviceObject = new BLEAdvertisedDevice(advertisedDevice);
      doConnect[j] = true;
      doScan[j] = true;

    }  // Found our server
  }  // onResult
};  // MyAdvertisedDeviceCallbacks

void scanSetUp(BLEUUID *serviceUUID, BLEAdvertisedDevice **myDevice, int i) {
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(serviceUUID, myDevice, i));
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
};
// Retrieve a Scanner and set the callback we want to use to be informed when we
// have detected a new device.  Specify that we want active scanning and start the
// scan to run for 5 seconds. But now a function.

void serialReportISR (void *arg) {
if (connected[0] && connected[1]) {
//Human Crowdedness Algorithm  
int total1 = thresholdFunction(data1 , data1State);
int total2 = thresholdFunction(data2 , data2State);
int total = total1 + total2;

//Serial output for debug purposes
  // Serial.print("Notify callback for characteristic ");
  // Serial.print(CHARACTERISTIC_UUID_NODE1);
  // Serial.print(" of data length ");
  // Serial.println(8);
  // Serial.print("data: ");
  //     for (int i = 0; i < 8; i++){
  //     Serial.print(data1[i]);
  //     Serial.print(",");
  //     }
  // Serial.println();

  //   Serial.print("Notify callback for characteristic ");
  // Serial.print(CHARACTERISTIC_UUID_NODE2);
  // Serial.print(" of data length ");
  // Serial.println(8);
  // Serial.print("data: ");
  //     for (int i = 0; i < 8; i++){
  //     Serial.print(data2[i]);
  //     Serial.print(",");
  //     }
  // Serial.println();

if((0 <= total) && (total < 4)) {
  switch(total) {
    case 0: {
      Serial.println("0 people detected");
    }
    //0 ppl
      break;
    case 1: {
      Serial.println("1 people detected");
    }
    //should'nt be possible but 1 if so
      break;
    case 2: {
      Serial.println("1 people detected");
    }
    //1 ppl
      break;
    case 3: {
      Serial.println("2 people detected");
    }
    //2 ppl
      break;
    default: 
    Serial.println("yikes");
    }
  }
  else {
    Serial.println("more than 2 people detected");
  }
  //more than 2 ppl
  }
};

void setup() {
  Serial.begin(115200);

  esp_timer_handle_t serialReport;
  esp_timer_create_args_t serialReportTimer = {
    .callback = serialReportISR,        //!< Function to call when timer expires
    .arg = nullptr,                          //!< Argument to pass to the callback
    .dispatch_method = ESP_TIMER_TASK,   //!< Call the callback from task or from ISR
    .name = "Timer_Serial_Report",               //!< Timer name, used in esp_timer_dump function
    .skip_unhandled_events = true,     //!< Skip unhandled events for periodic timers
  };
  esp_timer_create(&serialReportTimer, &serialReport);
  esp_timer_start_periodic(serialReport, 1000000);


  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  scanSetUp(&serviceUUID1, &myDevice1, 0);
  scanSetUp(&serviceUUID2, &myDevice2, 1);


}  // End of setup.

// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect[0]) {
    if (connectToServer(&serviceUUID1, &charUUID1, &pRemoteCharacteristic1, &myDevice1, 0)) {
      Serial.println("We are now connected to the BLE Node 1.");
    } else {
      Serial.println("We have failed to connect to server 1; there is nothing more we will do.");
    }
    doConnect[0] = false;
  }

  if (doConnect[1]) {
    if (connectToServer(&serviceUUID2, &charUUID2, &pRemoteCharacteristic2, &myDevice2, 1)) {
      Serial.println("We are now connected to the BLE Node 2.");
    } else {
      Serial.println("We have failed to connect to server 2; there is nothing more we will do.");
    }
    doConnect[1] = false;
  }

  

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  // if (connected[0]) {
  //   String newValue = "Time since boot: " + String(millis() / 1000);
  //   Serial.println("Setting new characteristic value to \"" + newValue + "\"");

  //   // Set the characteristic's value to be the array of bytes that is actually a string.
  //   pRemoteCharacteristic1->writeValue(newValue.c_str(), newValue.length());
  // }

  // if (connected[1]) {
  //   String newValue = "Time since boot: " + String(millis() / 1000);
  //   Serial.println("Setting new characteristic value to \"" + newValue + "\"");

  //   // Set the characteristic's value to be the array of bytes that is actually a string.
  //   pRemoteCharacteristic2->writeValue(newValue.c_str(), newValue.length());
  // } 
  

  else if (doScan[0] || doScan[1]) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
    // Delay a second between loops.
}  // End of loop