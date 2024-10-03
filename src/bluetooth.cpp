#include "config.h"

#if BUILD_ENV_NAME == lilygo_t_display_s3

#include "bluetooth.h"

SerialStatusCallback::~SerialStatusCallback() {};
void SerialStatusCallback::onConnected() { Serial.println(F("Callback::onConnected")); };
void SerialStatusCallback::onDisconnected() { Serial.println(F("Callback::onDisconnected")); };
void SerialStatusCallback::onTransmit() { Serial.println(F("Callback::onTransmit")); };
void SerialStatusCallback::onReceive() { Serial.println(F("Callback::onReceive")); };

const BLEUUID BTAdapter::SERVICE_UUID("0000fff0-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in.
const BLEUUID BTAdapter::READ_UUID("0000fff1-0000-1000-8000-00805f9b34fb");
const BLEUUID BTAdapter::WRITE_UUID("0000fff2-0000-1000-8000-00805f9b34fb");

BTAdapter::BTAdapter() : m_buffer(LoopbackStream(256)) {
    m_doConnect = false;
    m_connected = false;
    m_doScan = true;
    m_pRemoteRead = NULL;
    m_pRemoteWrite = NULL;
    m_myDevice = NULL;
    m_mutex = portMUX_INITIALIZER_UNLOCKED;
    m_callback = NULL;
}

BTAdapter& BTAdapter::GetInstance() {
    static BTAdapter instance;
    return instance;
}

bool BTAdapter::IsConnected() const {
    return m_connected;
}

bool BTAdapter::HasDevice() const {
    return m_myDevice != NULL;
}

void BTAdapter::SetCommunicationCallback(BLEStatusCallback* callback) {
    m_callback = callback;
}

bool BTAdapter::Init() {
    BLEDevice::init("DPF-indicator");

    m_doScan = true;

    return true;
}

bool BTAdapter::Scan(const uint32_t duration) {
    if (m_doScan) {
        // Retrieve a Scanner and set the callback we want to use to be informed when we
        // have detected a new device.  Specify that we want active scanning and start the
        // scan to run for 5 seconds.
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(this);
        pBLEScan->setInterval(230);
        pBLEScan->setWindow(230);
        pBLEScan->setActiveScan(true);
        pBLEScan->start(duration, BTAdapter::onScanComplete);

        m_doScan = false;
        return true;
    }
    return false;
}

bool BTAdapter::ConnectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(m_myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(this);

    // Connect to the remove BLE Server.
    pClient->connect(m_myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
    if (pRemoteService == NULL) {
        Serial.printf("Failed to find our service UUID: %s\n", const_cast<BLEUUID*>(&SERVICE_UUID)->toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the write characteristic in the service of the remote BLE server
    m_pRemoteWrite = pRemoteService->getCharacteristic(WRITE_UUID);
    if (m_pRemoteWrite == NULL) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", const_cast<BLEUUID*>(&WRITE_UUID)->toString().c_str());
        pClient->disconnect();
        return false;
    }

    if (m_pRemoteWrite->canWriteNoResponse()) {
        Serial.println(" - Write characteristic found");
        m_pRemoteWrite->registerForNotify(BTAdapter::notifyCallback);
    } else {
        // No write found
        Serial.println(" - No write characteristic found!");
        pClient->disconnect();
        return false;
    }

    // Obtain a reference to the notify characteristic in the service of the remote BLE server
    m_pRemoteRead = pRemoteService->getCharacteristic(READ_UUID);
    if (m_pRemoteRead == NULL) {
        Serial.printf("Failed to find our characteristic UUID: %s\n", const_cast<BLEUUID*>(&READ_UUID)->toString().c_str());
        pClient->disconnect();
        return false;
    }
    // Serial.printf(" - Found our characteristic: %s\n", m_pRemoteRead->toString().c_str());

    if (m_pRemoteRead->canNotify()) {
        Serial.println(" - Notify characteristic found");
        m_pRemoteRead->registerForNotify(BTAdapter::notifyCallback);
    } else {
        // No read found
        Serial.println(" - No notify characteristic found!");
        pClient->disconnect();
        return false;
    }

    m_connected = true;
    if (m_callback != NULL) m_callback->onConnected();
    return true;
}

bool BTAdapter::Loop() {
    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect. Now we connect to it.  Once we are
    // connected we set the connected flag to be true.
    if (m_doConnect) {
        const bool status = ConnectToServer();
        if (status) {
            Serial.println("BLE server connected");
        } else {
            Serial.println("BLE server not connected");
        }
        m_doConnect = false;
        return status;
    }
    return true;
}

void BTAdapter::onConnect(BLEClient* pclient) {
    Serial.println("Device connected");
}

void BTAdapter::onDisconnect(BLEClient* pclient) {
    Serial.println("Device disconnected");
    m_connected = false;
    m_doScan = true;
    if (m_callback != NULL) m_callback->onDisconnected();
}

void BTAdapter::onResult(BLEAdvertisedDevice device) {
    // Called for each advertising BLE server
    Serial.print("BLE advertised device found: ");
    Serial.println(device.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (device.haveServiceUUID() && device.isAdvertisingService(SERVICE_UUID)) {
        // Found our server device
        BLEDevice::getScan()->stop();
        m_myDevice = new BLEAdvertisedDevice(device);
        m_doConnect = true;
        m_doScan = false;
    }
}

void BTAdapter::onScanComplete(BLEScanResults scanResults) {
    Serial.println("Scan completed...");
    BTAdapter::GetInstance().m_doScan = true;
}

void BTAdapter::notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    // #define READ_DEBUG 1

#ifdef READ_DEBUG
    Serial.print("[B] Received: ");
#endif

    taskENTER_CRITICAL(&BTAdapter::GetInstance().m_mutex);

    for (size_t i = 0; i < length; i++) {
        BTAdapter::GetInstance().m_buffer.write(pData[i]);
#ifdef READ_DEBUG
        if (pData[i] != '\r') {
            Serial.print((char)pData[i]);
        } else {
            Serial.print("\\r");
        }
#endif
    }

    taskEXIT_CRITICAL(&BTAdapter::GetInstance().m_mutex);

    if (BTAdapter::GetInstance().m_callback != NULL) BTAdapter::GetInstance().m_callback->onReceive();
#ifdef READ_DEBUG
    Serial.println();
#endif
}

int BTAdapter::available() {
    return m_buffer.available();
}

int BTAdapter::read() {
    taskENTER_CRITICAL(&m_mutex);
    const int data = m_buffer.read();
    taskEXIT_CRITICAL(&m_mutex);
    return data;
}

int BTAdapter::peek() {
    return m_buffer.peek();
}

size_t BTAdapter::write(uint8_t data) {
    // #define WRITE_DEBUG 1
    if (m_connected && m_pRemoteWrite != NULL) {
#ifdef WRITE_DEBUG
        Serial.print("Send: ");
        Serial.print((char)data);

        if (data == '\r') {
            Serial.println();
        }
#endif
        m_pRemoteWrite->writeValue(data, false);
        if (m_callback != NULL) m_callback->onTransmit();
        return 1;
    }
    return 0;
}
#endif
