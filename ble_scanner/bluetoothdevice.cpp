#include "bluetoothdevice.h"

BluetoothDevice::BluetoothDevice()
    : QBluetoothDeviceDiscoveryAgent(nullptr)
{
    //this->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BluetoothDevice::StartScanning()
{
    qDebug() << "BT Start!...";

    m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    connect(m_discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(m_discoveryAgent, SIGNAL(finished()), this, SLOT(deviceDiscoverFinished()));

    //connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &Device::deviceScanFinished);

    m_discoveryAgent->setLowEnergyDiscoveryTimeout(1000);
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);

}

void BluetoothDevice::ConnectToDevice(const QBluetoothDeviceInfo &device)
{
    m_controller = QLowEnergyController::createCentral(device, nullptr);
    qDebug() << "BT controller state"<< m_controller->state();
    connect(m_controller, SIGNAL(connected()), this, SLOT(discoverService()));
    connect(m_controller, SIGNAL(discoveryFinished()), this, SLOT(serviceDiscoverFinished()));
    //connect(this, SIGNAL(serviceConnected()), this, SLOT(discoverCharacteristic()));
    m_controller->connectToDevice();
    qDebug() << "BT connecting to device: "<< device.deviceUuid().toString() << "...";

}

void BluetoothDevice::SelectService(const QBluetoothUuid &uuid)
{
    if(m_controller->state() == QLowEnergyController::DiscoveredState){
        qDebug() << "BT create service...";
        m_service = m_controller->createServiceObject(uuid);
    }

    connect(m_service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(serviceStateHandler(QLowEnergyService::ServiceState)));
    connect(m_service, SIGNAL(characteristicWritten(const QLowEnergyCharacteristic&, const QByteArray&)), this, SLOT(read(const QLowEnergyCharacteristic&)));
    connect(m_service, SIGNAL(characteristicRead(const QLowEnergyCharacteristic&, const QByteArray&)), this, SLOT(receive(const QLowEnergyCharacteristic&, const QByteArray&)));

    discoverCharacteristic();
}

void BluetoothDevice::SelectCharacteristic(const QBluetoothUuid &uuid)
{
    qDebug()<<"BT create characteristic: "<< uuid.toString();
    m_characteristic = m_service->characteristic(uuid);
    QLowEnergyDescriptor cccd = m_characteristic.clientCharacteristicConfiguration();
    m_service->writeDescriptor(cccd, QLowEnergyCharacteristic::CCCDEnableIndication);
    //m_service->writeDescriptor(cccd, QByteArray::fromHex("0002"));
    //QList<QLowEnergyDescriptor> descripts = m_characteristic.descriptors();
    //for(QList<QLowEnergyDescriptor>::const_iterator it = descripts.begin(); it!=descripts.end(); ++it){
    //    qDebug()<<"BT descripts name & uuid & value: "<< it->name() << it->uuid().toString() << it->value();
    //}

    //m_descriptor = m_characteristic.descriptor(descripts[0].uuid());
    //m_target_service->writeDescriptor(m_target_descriptor, QLowEnergyCharacteristic::CCCDEnableIndication);
    //qDebug()<<"service error?: "<< m_target_service->error();
}

void BluetoothDevice::Disconnect()
{
    qDebug()<<"BT Disconnect...";
    m_controller->disconnectFromDevice();
}

// slots
void BluetoothDevice::write(QByteArray &request)
{
    qDebug()<<"BT in " << m_characteristic.name() << m_characteristic.uuid().toString() << " write request: "<< request;
    m_service->writeCharacteristic(m_characteristic, request, QLowEnergyService::WriteWithResponse); //with write confirmation
    //m_service->writeDescriptor(m_descriptor, request);
    //qDebug()<<"descripts name & uuid & value: "<< m_target_descriptor.name() << m_target_descriptor.uuid().toString() << m_target_descriptor.value();

}

void BluetoothDevice::read(const QLowEnergyCharacteristic &charac)
{
    qDebug()<<"BT read";
    m_service->readCharacteristic(charac);
    //qDebug()<<"BT receive(): char name & uuid & value: "<< charac.name() << charac.uuid().toString() << charac.value().size() << charac.value();

    //m_service->readDescriptor(m_descriptor);
    //qDebug()<<"service error?: "<< m_service->error();
    //qDebug()<<"descripts name & uuid & value: "<< m_target_descriptor.name() << m_target_descriptor.uuid().toString() << m_target_descriptor.value();
}

void BluetoothDevice::receive(const QLowEnergyCharacteristic& charac, const QByteArray& value)
{
    qDebug()<<"BT receive(): char name & uuid & value: "<< charac.name() << charac.uuid().toString() << charac.value().size() << charac.value();
    qDebug()<<"BT receive(): descript name & uuid & value: "<< m_descriptor.name() << m_descriptor.uuid().toString() << m_descriptor.value();
    //m_value = m_characteristic.value();
    emit characteristicValueUpdated(value);
}

void BluetoothDevice::discoverService()
{
    //when device connected...
    qDebug() << "BT device is connected. Start discovering services...";
    if(m_controller->state() == QLowEnergyController::ConnectedState){
        m_controller->discoverServices();
    }
}

void BluetoothDevice::discoverCharacteristic()
{
    //when service selected...
    qDebug() << "BT service is created. Start discovering characteristic...";
    m_service->discoverDetails();

    //emit BluetoothDevice::characteristicUpdated(m_characteristics);
}

void BluetoothDevice::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    qDebug() << "BT Found new device:" << device.name() << "(" << device.deviceUuid().toString() << ") " << device.address().toString();
    emit BluetoothDevice::deviceUpdated(device);
}

void BluetoothDevice::deviceDiscoverFinished()
{
    m_discoveryAgent->discoveredDevices();
}

void BluetoothDevice::serviceDiscoverFinished()
{
    qDebug() << "BT services are discovered...";
    if(m_controller->state() == QLowEnergyController::DiscoveredState){
        //m_ui->tb_state->setText("Discovered!\nSelect a service on the service list...");
        m_services_uuids = m_controller->services();

        QStringList service_slist;
        for(QList<QBluetoothUuid>::const_iterator it = m_services_uuids.begin(); it!=m_services_uuids.end(); ++it){
            qDebug() << "service: " << it->toString();
            service_slist << it->toString();
            //m_service_model->setStringList(service_slist);
        }
    }
    emit BluetoothDevice::serviceUpdated(m_services_uuids);
}

void BluetoothDevice::serviceStateHandler(QLowEnergyService::ServiceState state)
{
    qDebug() << "stateChanged!! m_service state"<< m_service->state();
    if(state == QLowEnergyService::RemoteServiceDiscovered)
    {
        m_characteristics = m_service->characteristics();
        emit BluetoothDevice::characteristicUpdated(m_characteristics);
    }
}
