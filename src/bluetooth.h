#include <Arduino.h>
#include <LoopbackStream.h>

#include "BLEDevice.h"

class BLEStatusCallback {
   public:
    virtual ~BLEStatusCallback() {};
    virtual void onConnected() = 0;
    virtual void onDisconnected() = 0;
    virtual void onTransmit() = 0;
    virtual void onReceive() = 0;
};

// Just a simple callback implementation using serial trace
class SerialStatusCallback : public BLEStatusCallback {
   public:
    virtual ~SerialStatusCallback() override;
    virtual void onConnected() override;
    virtual void onDisconnected() override;
    virtual void onTransmit() override;
    virtual void onReceive() override;
};

class BTAdapter : public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks, public Stream {
   public:
    static BTAdapter& GetInstance();

    bool IsConnected() const;
    bool HasDevice() const;

    void SetCommunicationCallback(BLEStatusCallback* callback);

    bool Init();
    bool Scan(const uint32_t duration = 5);
    bool ConnectToServer();

    bool Loop();

   private:
    BTAdapter();
    BTAdapter(BTAdapter& other) = delete;
    void operator=(const BTAdapter&) = delete;

   protected:
    // The remote service we wish to connect to.
    static const BLEUUID SERVICE_UUID;
    // The characteristic of the remote service we are interested in.
    static const BLEUUID READ_UUID;
    static const BLEUUID WRITE_UUID;

    static BTAdapter* m_singleton;

    bool m_doConnect;
    bool m_connected;
    bool m_doScan;
    BLERemoteCharacteristic* m_pRemoteRead;
    BLERemoteCharacteristic* m_pRemoteWrite;
    BLEAdvertisedDevice* m_myDevice;
    LoopbackStream m_buffer;
    portMUX_TYPE m_mutex;
    BLEStatusCallback* m_callback;

    virtual void onConnect(BLEClient* pclient) override;
    virtual void onDisconnect(BLEClient* pclient) override;
    virtual void onResult(BLEAdvertisedDevice advertisedDevice) override;
    static void onScanComplete(BLEScanResults scanResults);

    static void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

    // Stream interface implementation
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual size_t write(uint8_t data) override;
};