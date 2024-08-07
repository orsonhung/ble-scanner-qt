#ifndef BLUETOOTHDEVICE_H
#define BLUETOOTHDEVICE_H

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QObject>

class BluetoothDevice : public QBluetoothDeviceDiscoveryAgent
{
    Q_OBJECT //what? why? this can make signals built, without this, "undefine reference"
public:
    BluetoothDevice();
    void StartScanning();
    void ConnectToDevice(const QBluetoothDeviceInfo &device);
    void SelectService(const QBluetoothUuid &uuid);
    void SelectCharacteristic(const QBluetoothUuid &uuid);
    void Disconnect();

public slots:
    void write(QByteArray &request);
    void read(const QLowEnergyCharacteristic& charac);
    void receive(const QLowEnergyCharacteristic& charac, const QByteArray& value);
    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void deviceDiscoverFinished();
    void serviceDiscoverFinished();
    void discoverService();
    void discoverCharacteristic();
    void serviceStateHandler(QLowEnergyService::ServiceState state);

signals:
    void deviceUpdated(const QBluetoothDeviceInfo &device);
    void serviceUpdated(QList<QBluetoothUuid> &services);
    //void serviceConnected();
    //void characteristicDiscovered();
    void characteristicUpdated(QList<QLowEnergyCharacteristic> &characteristics);
    void characteristicValueUpdated(const QByteArray &value);

private:
    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QList<QBluetoothDeviceInfo> m_devices;
    QLowEnergyController *m_controller;

    QList<QBluetoothUuid> m_services_uuids;
    QLowEnergyService *m_service;

    QList<QLowEnergyCharacteristic> m_characteristics;
    QLowEnergyCharacteristic m_characteristic;

    //QList<QLowEnergyDescriptor> m_descriptors;
    QLowEnergyDescriptor m_descriptor;

    QByteArray m_value;
};

#endif // BLUETOOTHDEVICE_H
