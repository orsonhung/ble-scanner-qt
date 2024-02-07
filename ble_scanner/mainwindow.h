#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QStringListModel>
#include <QStandardItemModel>
#include <QLowEnergyCharacteristic>
#include "bluetoothdevice.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void lvdeviceUpdate(QBluetoothDeviceInfo device);
    void lvserviceUpdate(QList<QBluetoothUuid> &services);
    void lvcharacUpdate(QList<QLowEnergyCharacteristic> &characteristics);
    void receive(const QByteArray &value);
    void accumReceiValue(const QByteArray &value);
    void read(QByteArray &request);
    void receiveDisplay();

private slots:

    void on_pb_start_clicked();

    void on_lv_device_clicked(const QModelIndex &index);

    void on_pb_connect_clicked();

    void on_lv_service_clicked(const QModelIndex &index);

    void on_pb_service_clicked();

    void on_lv_character_clicked(const QModelIndex &index);

    void on_pb_character_clicked();

    void on_pb_request_clicked();

    void on_le_write_little_textChanged(const QString &arg1);

    void on_le_write_big_textChanged(const QString &arg1);

    void on_pb_decode_clicked();

    void on_pb_output_clicked();

    void on_pb_output_txt_clicked();

signals:
    void sendRequest();
    void receiveValue(const QByteArray &value);
    void recursiveRead(QByteArray &request);

private:
    Ui::MainWindow *m_ui;
    BluetoothDevice *m_bt;
    //QBluetoothDeviceDiscoveryAgent *m_discoveryAgent;
    QList<QBluetoothDeviceInfo> m_devices;
    QStringListModel *m_device_model, *m_service_model, *m_characteristic_model;
    QStandardItemModel *m_summary_model;
    QModelIndex *m_index;
    QBluetoothDeviceInfo m_target_device;
    QLowEnergyService *m_target_service;
    QLowEnergyCharacteristic m_target_characteristic;
    QLowEnergyDescriptor m_target_descriptor;
    QLowEnergyController *m_controller;
    QLowEnergyService *m_services;
    QList<QBluetoothUuid> m_services_uuids;
    QList<QLowEnergyCharacteristic> m_characteristics;
    int m_device_idx, m_service_idx, m_character_idx;
    QString m_req_text_little, m_req_text_big;

    QByteArray m_request;
    int m_request_idx;
    int m_need_read_byte;
    int m_already_read;
    void Request(QByteArray request);
    void GetNeedReadType();
    void WriteDataRecord(QByteArray content, QTextStream &txt);
    QByteArray m_decode_content, m_receive_content, m_all_receive_content, m_crc32;

    void set_ee_summary_tv();
    void service_ee_summary(QByteArray content);
    void service_ee_single_history(QByteArray content);
    void set_ff_summary_tv();
    void service_ff_summary(QByteArray content);

    QList<QStandardItem*> m_summary_items;
};
#endif // MAINWINDOW_H
