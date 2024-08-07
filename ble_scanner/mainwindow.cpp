#include "mainwindow.h"
#include "bluetoothdevice.h"
#include "datautility.h"
#include "./ui_mainwindow.h"
#include <QString>
#include <QFile>
#include <QDir>
#include <sstream>

#define BT_HEADER_LEN 20

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_bt(new BluetoothDevice)
    , m_device_model(new QStringListModel())
    , m_service_model(new QStringListModel())
    , m_characteristic_model(new QStringListModel())
    , m_device_idx(0)
    , m_service_idx(0)
    , m_character_idx(0)
    , m_request_idx(0)
    , m_need_read_byte(0)
    , m_already_read(0)
    , m_ee_record_num(0)
    , m_ff_record_num(0)
    , m_auto_save(false)
{
    qDebug() << "Current path: " << QDir::currentPath();
    m_ui->setupUi(this);
    m_ui->tb_state->setText("Welcome! \nPress \"Start scanning\" to search.");
    m_ui->le_write_little->setText("00");
    m_ui->le_write_big->setText("00");
    m_ui->pb_output->setDisabled(true);
    m_ui->pb_output_txt->setDisabled(true);
    m_ui->pb_output_all->setDisabled(true);
    m_devices.clear();
    m_device_model->setStringList(QStringList{});
    connect(m_bt, SIGNAL(deviceUpdated(QBluetoothDeviceInfo)), this, SLOT(lvdeviceUpdate(QBluetoothDeviceInfo)));//SIGNAL(QBluetoothDeviceInfo) -> SLOT(QBluetoothDeviceInfo) not &
    connect(m_bt, SIGNAL(serviceUpdated(QList<QBluetoothUuid>&)), this, SLOT(lvserviceUpdate(QList<QBluetoothUuid>&)));//SIGNAL(QBluetoothDeviceInfo) -> SLOT(QBluetoothDeviceInfo) not &
    connect(m_bt, SIGNAL(characteristicUpdated(QList<QLowEnergyCharacteristic>&)), this, SLOT(lvcharacUpdate(QList<QLowEnergyCharacteristic>&)));//SIGNAL(QBluetoothDeviceInfo) -> SLOT(QBluetoothDeviceInfo) not &
    connect(m_bt, SIGNAL(characteristicValueUpdated(const QByteArray&)), this, SLOT(receive(const QByteArray&)));
    connect(this, SIGNAL(receiveValue(const QByteArray&)), this, SLOT(accumReceiValue(const QByteArray&)));
    connect(this, SIGNAL(receiveValue(const QByteArray&)), this, SLOT(receiveDisplay()));
    connect(this, SIGNAL(recursiveRead(QByteArray&)), this, SLOT(read(QByteArray&)));
    connect(this, SIGNAL(recursiveOutput(int)), this, SLOT(loopRead(int)));
}

MainWindow::~MainWindow()
{
    delete m_ui;
}

//slots
void MainWindow::lvdeviceUpdate(QBluetoothDeviceInfo device)
{
    qDebug() << "UI lvdeviceUpdate..."<< device.name() << ", " << device.deviceUuid();
    m_devices.push_back(device);
    if(m_devices.size() != 0){
        m_device_model->setStringList(QStringList{});
        QStringList device_slist;
        for(QList<QBluetoothDeviceInfo>::const_iterator it = m_devices.begin(); it!=m_devices.end(); ++it){
            //qDebug() << it->name() << ", " << it->deviceUuid();
            //QBluetoothUuid bt_uuid = it->deviceUuid();
            device_slist << it->name() + ' ' + it->deviceUuid().toString();
            m_device_model->setStringList(device_slist);
        }
        m_ui->lv_device->setModel(m_device_model);
        m_ui->tb_state->setText("Select a device on device list...");
    }
}

void MainWindow::lvserviceUpdate(QList<QBluetoothUuid> &services)
{
    m_services_uuids = services;
    if(services.size()!=0)
    {
        QStringList service_slist;
        for(QList<QBluetoothUuid>::const_iterator it = m_services_uuids.begin(); it!=m_services_uuids.end(); ++it){
            qDebug() << "UI service: " << it->toString();
            service_slist << it->toString();
            m_service_model->setStringList(service_slist);
        }
    }
    m_ui->lv_service->setModel(m_service_model);
}

void MainWindow::lvcharacUpdate(QList<QLowEnergyCharacteristic> &characteristics)
{
    m_characteristics = characteristics;
    if(m_characteristics.size() != 0){
        m_ui->tb_state->setText("Discovered!\nSelect a characteristic on the characteristic list...");
        QStringList characteristic_slist;
        for(QList<QLowEnergyCharacteristic>::const_iterator it = m_characteristics.begin(); it!=m_characteristics.end(); ++it){
            qDebug()<<"UI characteristic name & uuid & value: "<< it->properties() << it->name() << it->uuid().toString() << it->value();
            characteristic_slist << it->name() + ' ' + it->uuid().toString();
            m_characteristic_model->setStringList(characteristic_slist);
        }
    }
    m_ui->lv_character->setModel(m_characteristic_model);
}

void MainWindow::receive(const QByteArray &value)
{
    qDebug()<<"UI receive!! value: " << value;
    m_receive_content = value;
    emit receiveValue(value);
}

void MainWindow::accumReceiValue(const QByteArray &value)
{
    qDebug()<<"UI accumReceiValue!!";
    if(m_need_read_byte == 0)
        GetNeedReadType();

    const int len = value.size();
    if(m_already_read == 0)
        m_all_receive_content = (value); // first time include heeader
    else
        m_all_receive_content.push_back(value.last(len - BT_HEADER_LEN));

    //read iter is based on the len without header, so first time already read is size() - header
    // HEADER(20B) | [DATA(LEN-20B) -> for iter]
    m_already_read += m_receive_content.size() - BT_HEADER_LEN;
    qDebug()<<"already_read: "<< m_already_read;

    if(m_need_read_byte > m_already_read){
        m_request.removeLast();
        m_request.removeLast();
        unsigned char iter_lit = static_cast<unsigned char>(m_already_read % 256);
        unsigned char iter_big = static_cast<unsigned char>(m_already_read >> 8);
        m_request.push_back(iter_lit);
        m_request.push_back(iter_big);
        emit recursiveRead(m_request);
    }   else    {
        if(m_service_idx==2)
            m_ui->pb_output_txt->setEnabled(true);
        if(m_auto_save){
            m_ui->pb_output_txt->click();
            if(m_request_idx != (m_ff_record_num-1))
                emit recursiveOutput(m_request_idx+1);
        }
    }
}

void MainWindow::read(QByteArray &request)
{
    qDebug()<<"UI read!! already_read: " << request;
    m_bt->write(request);
}

void MainWindow::receiveDisplay()
{
    m_ui->tb_recie_content->setText(m_all_receive_content.toHex());
    if(m_service_idx==0){
        QByteArray bt_uuid_barr = m_all_receive_content.sliced(16, 6);
        QByteArray nums = m_all_receive_content.sliced(22, 4);
        QByteArray ee_nums = nums.last(2);
        QByteArray ff_nums = nums.first(2);
        m_ee_record_num = DataUtility::byteArraytoInt(ee_nums);
        m_ff_record_num = DataUtility::byteArraytoInt(ff_nums);
        m_bt_uuid = QString(bt_uuid_barr.toHex()); //opposite: QByteArray a = QByteArray::fromHex("String")
        for(int i = 2; i < m_bt_uuid.size(); i += 3)
            m_bt_uuid.insert(i, '_');

        qDebug()<<"ee record numbers: "<<m_ee_record_num;
        qDebug()<<"ff record numbers: "<<m_ff_record_num;
        qDebug()<<"bt uuid: "<<m_bt_uuid;
    }
}

void MainWindow::loopRead(int idx)
{
    if(idx<m_ff_record_num){
        if(idx<10){
            m_req_text_little = QString::number(0) + QString::number(idx);
        }   else if(idx==10)    {
            m_req_text_little = QString("0a");
        }   else if(idx==11)    {
            m_req_text_little = QString("0b");
        }   else if(idx==12)    {
            m_req_text_little = QString("0c");
        }   else if(idx==13)    {
            m_req_text_little = QString("0d");
        }
        m_req_text_big = QString("00");
        m_ui->le_write_little->setText(m_req_text_little);
        m_ui->le_write_big->setText(m_req_text_big);

        m_ui->pb_request->click();
    }
}

//ui slots
void MainWindow::on_pb_start_clicked()
{
    qDebug()<<"UI start scanning clicked!!";
    m_devices.clear();
    m_bt->StartScanning();
    qDebug()<<"UI m_bt started!!";
}

void MainWindow::on_lv_device_clicked(const QModelIndex &index)
{
    qDebug()<<"UI device list view clicked!! index: "<< index.row();
    m_bt->stop();
    m_ui->tb_state->setText("A device is selected! \nPress \"Connect\" to connect.");
    m_device_idx = index.row();
}

void MainWindow::on_pb_connect_clicked()
{
    const QBluetoothDeviceInfo device = m_devices.at(m_device_idx);
    qDebug() << "UI target device name: "<< device.name() << ", uuid: " << device.deviceUuid().toString();
    m_bt->ConnectToDevice(device);
    m_device_name = device.name()+device.deviceUuid().toString();

    m_ui->tb_state->setText("Connecting to device...");
}

void MainWindow::on_pb_disconnect_clicked()
{
    m_bt->Disconnect();
    ClearAll();
}

void MainWindow::on_lv_service_clicked(const QModelIndex &index)
{
    qDebug() << "UI service list view clicked!! index: " << index.row();
    m_ui->tb_state->setText("A service is selected! \nPress \"Confirm\" to the next step.");
    m_service_idx = index.row();
}

void MainWindow::on_pb_service_clicked()
{
    qDebug() << "UI service confirm button clicked!!";
    m_bt->SelectService(m_services_uuids[m_service_idx]);
    m_auto_save = false;
}

void MainWindow::on_lv_character_clicked(const QModelIndex &index)
{
    qDebug() << "UI character list view clicked!! index: " << index.row();
    m_ui->tb_state->setText("A character is selected! \nPress \"Confirm\" to the next step.");
    m_character_idx = index.row();
}

void MainWindow::on_pb_character_clicked()
{
    m_ui->tb_state->setText("UI Setting done!");
    m_bt->SelectCharacteristic(m_characteristics[m_character_idx].uuid());

    //m_service_idx=2;
    if(m_service_idx==1)
        set_ee_summary_tv();
    else if(m_service_idx==2){
        set_ff_summary_tv();
        if(m_ff_record_num!=0)
            m_ui->pb_output_all->setEnabled(true);
    }
    //m_target_service->writeDescriptor(m_target_descriptor, QLowEnergyCharacteristic::CCCDEnableIndication);
}

void MainWindow::on_pb_request_clicked()
{
    //QByteArray request; // "0x01"
    //QByteArray::fromHex("0200")
    m_request.clear();
    m_request_idx = 0;
    m_need_read_byte = 0;
    m_already_read = 0;
    m_receive_content.clear();
    m_all_receive_content.clear();

    if(m_service_idx == 0){
        m_request.append(0x11);
        m_request.append(0x22);
        m_request.append(0x33);
        m_request.append(0x44);
        m_request.append(0xaa);
        m_request.append(0xbb);
        m_request.append(0xcc);
        m_request.append(0xdd);
    } else  {
        unsigned char lit = 0x00;
        unsigned char big = 0x00;
        if( (m_req_text_big.end()-m_req_text_big.begin() == 2) && (m_req_text_little.end()-m_req_text_little.begin() == 2) )
        {
            lit = DataUtility::byteStringToUChar(m_req_text_little);
            big = DataUtility::byteStringToUChar(m_req_text_big);
            bool ok;
            m_request_idx += m_req_text_little.toInt(&ok, 16);
            m_request_idx += m_req_text_big.toInt(&ok, 16)*256;
            qDebug()<<"Request text(lit) encode! content:" << lit;
            qDebug()<<"Request text(big) encode! content:" << big;
            qDebug()<<"m_request_idx:" << m_request_idx;
        } else  {

        }
        unsigned char iter_lit = 0x00;
        unsigned char iter_big = 0x00;
        m_request.append(lit);
        m_request.append(big);
        m_request.append(iter_lit);
        m_request.append(iter_big);
    }
    qDebug()<<"UI Request: "<< m_request;
    Request(m_request);
    //Receive();

}

void MainWindow::on_le_write_little_textChanged(const QString &arg1)
{
    m_req_text_little = m_ui->le_write_little->text();
    qDebug()<<"Request text(little) changed! content:" << m_req_text_little;
}

void MainWindow::on_le_write_big_textChanged(const QString &arg1)
{
    m_req_text_big = m_ui->le_write_big->text();
    qDebug()<<"Request text(big) changed! content:" << m_req_text_big;
}

void MainWindow::on_pb_decode_clicked()
{
    if(m_service_idx==1){ //ee
        m_crc32 = m_all_receive_content.last(4);
        m_decode_content = QByteArray::fromBase64(m_all_receive_content.first( m_all_receive_content.size()-4 ));
        QString content = QString::fromStdString(m_decode_content.toHex().toStdString() + m_crc32.toHex().toStdString());
        qDebug()<<"decode char name & uuid & value(HEX): "<< m_target_characteristic.name() << m_target_characteristic.uuid().toString() << m_decode_content << m_crc32;
        m_ui->tb_decode_content->setText(content);
        service_ee_summary(m_decode_content);
    }else{ //ff
        m_decode_content = m_all_receive_content;
        QString content = QString::fromStdString(m_decode_content.toHex().toStdString());
        qDebug()<<"decode char name & uuid & value(HEX): "<< m_target_characteristic.name() << m_target_characteristic.uuid().toString() << m_decode_content;
        m_ui->tb_decode_content->setText(content);
        service_ff_summary(m_decode_content);
    }
}

void MainWindow::Request(QByteArray request)
{
    m_bt->write(request);
    m_ui->pb_output->setDisabled(true);
    m_ui->pb_output_txt->setDisabled(true);
}

void MainWindow::GetNeedReadType()
{
    qDebug()<<"service_ff_read...";
    if(m_request.at(2)==0x00 && m_request.at(3)==0x00){
        if(m_receive_content.size() > 4){

            if(m_service_idx == 1){
                QByteArray decode_content = QByteArray::fromBase64(m_receive_content);
                int hex_uint_big = static_cast<int>(static_cast<unsigned char>(decode_content.at(3)));
                int hex_uint_lit = static_cast<int>(static_cast<unsigned char>(decode_content.at(2)));
                const int this_page_record_num = (hex_uint_big<<8) + hex_uint_lit;
                qDebug()<<"this_page_record_num: "<< this_page_record_num << hex_uint_big << hex_uint_lit;
                m_need_read_byte = this_page_record_num * 48 + 8 + 4;
                qDebug()<<"need_read_byte: "<< m_need_read_byte;
            }   else    {
                int hex_uint_big = static_cast<int>(static_cast<unsigned char>(m_receive_content.at(5)));
                int hex_uint_lit = static_cast<int>(static_cast<unsigned char>(m_receive_content.at(4)));
                m_need_read_byte = (hex_uint_big<<8) + hex_uint_lit;
                m_need_read_byte -= BT_HEADER_LEN; // exclude header len
                qDebug()<<"need_read_byte: "<< m_need_read_byte << hex_uint_big << hex_uint_lit;
            }
        }
    }
}

void MainWindow::ClearAll()
{
    m_auto_save = false;
    m_ee_record_num = 0;
    m_ff_record_num = 0;
    m_devices.clear();
    m_device_model->setStringList(QStringList{});
    m_service_model->setStringList(QStringList{});
    m_characteristic_model->setStringList(QStringList{});
    m_summary_model->clear();
    m_services_uuids.clear();
    m_characteristics.clear();
    m_decode_content.clear();
    m_receive_content.clear();
    m_all_receive_content.clear();
    m_crc32.clear();
    m_summary_items.clear();

    m_ui->pb_output->setDisabled(true);
    m_ui->pb_output_txt->setDisabled(true);
    m_ui->pb_output_all->setDisabled(true);
    m_ui->le_write_little->setText("00");
    m_ui->le_write_big->setText("00");
    m_ui->lv_device->setModel(m_device_model);
    m_ui->lv_service->setModel(m_service_model);
    m_ui->lv_character->setModel(m_characteristic_model);
    m_ui->tb_recie_content->setText("");
    m_ui->tb_decode_content->setText("");
    m_ui->tv_summary->setModel(m_summary_model);
}

void MainWindow::set_ee_summary_tv()
{
    m_summary_model = new QStandardItemModel(0,18);
    QList<int> h_width = {40,40,45,30,30,30,30,30,30,45,45,45,45,30,30,30,30,40};
    QList<QString> h_header;
    h_header.append(QString("Magic"));
    h_header.append(QString("id"));
    h_header.append(QString("Y"));
    h_header.append(QString("M"));
    h_header.append(QString("D"));
    h_header.append(QString("hr"));
    h_header.append(QString("min"));
    h_header.append(QString("sec"));
    h_header.append(QString("re1"));
    h_header.append(QString("TC"));
    h_header.append(QString("TG"));
    h_header.append(QString("HDL"));
    h_header.append(QString("LDL"));
    h_header.append(QString("num"));
    h_header.append(QString("re2"));
    h_header.append(QString("re3"));
    h_header.append(QString("re4"));
    h_header.append(QString("re32"));
    m_ui->tv_summary->setModel(m_summary_model);
    for(int c = 0; c < 17; c++){
        QStandardItem* item = new QStandardItem(QString(h_header[c]));
        m_summary_model->setHorizontalHeaderItem(c, item);
        m_ui->tv_summary->setColumnWidth(c, h_width[c]);
    }
}

void MainWindow::service_ee_summary(QByteArray content)
{
    /*for(QList<QStandardItem*>::iterator it = m_summary_tv.begin(); it != m_summary_tv.end(); ++it)
        free(it);
    m_summary_tv.clear();*/

    QByteArray head = content.first(2);
    qDebug()<<"head: "<< head;
    QByteArray his_num = content.sliced(2,2);
    qDebug()<<"his_num: "<< his_num;
    int num = static_cast<int>(his_num.at(0));
    int iter_slice = 4;
    for(int i = 0; i<num; i++){
        QByteArray his = content.sliced(iter_slice,36);
        service_ee_single_history(his);
        qDebug()<<"["<< iter_slice <<"]his: "<< his.toHex();
        iter_slice+=36;
    }
}

void MainWindow::service_ee_single_history(QByteArray content)
{
    int row_count = m_summary_model->rowCount();
    qDebug()<<"row_count: "<< row_count;
    //cc aa 59 00 e7 07 0a 19 13 09 0e 00 00 00 2c 43 00 00 da 42 00 00 30 42 66 66 d4 42 04 00 00 00 00 00 00 00
    m_summary_items.clear();
    m_summary_items.append(new QStandardItem(QString(content.first(2).toHex())));
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(2,2))))); //id num
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(4,2))))); // year
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(6,1))))); // month
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(7,1))))); // day
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(8,1))))); // hr
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(9,1))))); // min
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(10,1))))); // sec
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(11,1))))); // reserve1
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoFloat(content.sliced(12,4))))); // TC
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoFloat(content.sliced(16,4))))); // TG
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoFloat(content.sliced(20,4))))); // HDL
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoFloat(content.sliced(24,4))))); // HDL
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(28,1))))); // result num
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(29,1))))); // reserve2
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(30,1))))); // reserve3
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(31,1))))); // reserve4
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(32,4))))); // reserve32

    m_summary_model->appendRow(m_summary_items);
}

void MainWindow::on_pb_output_clicked()
{
    QString path = QCoreApplication::applicationDirPath() + "/../../../qt_output/";
    QString meter = m_device_name.split('{')[0] + "{" + m_bt_uuid + "}/";

    QString target_dir = path + meter;
    QDir dir(target_dir);
    if (!dir.exists()){
        dir.mkpath(".");
        qDebug() << "create dir!!: "<< dir.absolutePath();
    }

    QString filename = "/output.csv";
    const QString fp = target_dir + filename;
    QFile file(fp);

    if ( !file.open(QFile::WriteOnly |QFile::Truncate)) {
        qDebug() << "File not exists";
    }   else    {
        if(m_service_idx==1){ //ee
            QTextStream output(&file);
            for (auto row = 0; row < m_summary_model->rowCount(); row++) {
                for (auto col = 0; col < 18; col++) {
                    output << m_summary_model->data(m_summary_model->index(row,col)).toString()<<", ";
                }
                output<<"\n";
            }
            qDebug() << "CSV file is written!: "<< file.fileName();
        }else{ //ff
            QTextStream output(&file);
            for (auto col = 0; col < m_summary_model->columnCount(); col++) {
                for (auto row = 0; row < 28; row++) {
                    output << m_summary_model->data(m_summary_model->index(row,col)).toString()<<", ";
                }
                output<<"\n";
            }
            qDebug() << "CSV file is written!: "<< file.fileName();
        }

    }

    file.close();
}

void MainWindow::on_pb_output_txt_clicked()
{
    QString path = QCoreApplication::applicationDirPath() + "/../../../qt_output/";
    QString meter = m_device_name.split('{')[0] + "{" + m_bt_uuid + "}/";
    QString folder = QString::number(m_request_idx);

    QString target_dir = path + meter + folder;
    QDir dir(target_dir);
    if (!dir.exists()){
        dir.mkpath(".");
        qDebug() << "create dir!!: "<< dir.absolutePath();
    }


    QString filename = "/DataRecord.txt";
    const QString fp = target_dir + filename;
    QFile file(fp);

    if ( !file.open(QFile::WriteOnly |QFile::Truncate)) {
        qDebug() << "fp: "<< fp << " File not exists";
    }   else    {
        QTextStream out(&file);
        qDebug() << "m_all_receive_content size: "<< m_all_receive_content.size();
        WriteDataRecord(m_all_receive_content, out);
        qDebug() << "TXT file is written!: "<< file.fileName();
    }

    file.close();
}

void MainWindow::on_pb_output_all_clicked()
{
    qDebug() << "m_ff_record_num: "<<m_ff_record_num<<", m_service_idx: "<<m_service_idx;
    if(m_ff_record_num!=0 && m_service_idx==2){
        m_auto_save = true;
        emit recursiveOutput(0);
        qDebug() << "auto output run!! ";
    }
}

void MainWindow::set_ff_summary_tv()
{
    m_summary_model = new QStandardItemModel(28,0);
    //QList<int> h_width = {40,40,45,30,30,30,30,30,30,45,45,45,45,30,30,30,30,40};
    QList<QString> v_header = {"Header Magic", "Data Length", "CRC32", "Reserve",
                                "Magic", "DataRecord Ver.", "Unit Mode", "Color Params. Size", "Kernel Ver.", "Date&Time", "Status, State, Sample, Test", "Result, ROI, Element Number",
                                "Led Level", "Camera Gain", "Temperature", "Humidity", "Sampling index", "Code Number", "Note", "Reserve",
                                "Whiteboard Calib. BGR", "Color Params.", "Final Signal", "Final Result Value", "Whiteboard BGR", "Strip Area", "Initial ROIs BGR", "ROIs Initial/Final Center"};
    m_ui->tv_summary->setModel(m_summary_model);
    for(int c = 0; c < v_header.size(); c++){
        QStandardItem* item = new QStandardItem(QString(v_header[c]));
        m_summary_model->setVerticalHeaderItem(c, item);
    }
    m_ui->tv_summary->setColumnWidth(0, 200);
    m_ui->tv_summary->setColumnWidth(1, 200);
    m_ui->tv_summary->setColumnWidth(2, 200);
}

void MainWindow::service_ff_summary(QByteArray content)
{
    //63 76 68 00 74 08 00 00 73 06 ab 01 00 00 00 00 00 00 00 00 63 76 69 00
    m_summary_items.clear();
    m_summary_items.append(new QStandardItem(QString(content.first(4).toHex()))); //header
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(4,4))))); //result len
    m_summary_items.append(new QStandardItem(QString(content.sliced(8,4).toHex()))); //crc32
    m_summary_items.append(new QStandardItem(QString(content.sliced(12,8).toHex()))); //reserve

    m_summary_items.append(new QStandardItem(QString(content.sliced(20,4).toHex()))); //header
    m_summary_items.append(new QStandardItem(QString(content.sliced(26,2).toHex()))); //datarecord version
    m_summary_items.append(new QStandardItem(QString(content.sliced(25,1).toHex()))); //unit mode
    m_summary_items.append(new QStandardItem(QString(content.sliced(24,1).toHex()))); //meter size
    int meter_param_size = DataUtility::byteArraytoInt(content.sliced(24,1));
    m_summary_items.append(new QStandardItem(QString(content.sliced(28,4).toHex()))); //version
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoDateTime(content.sliced(32,8)))); //datetime
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(40,4), DataUtility::UCHAR))); //testtype, sampletype, state, status

    int element_num = DataUtility::byteArraytoInt(content.sliced(44,2));
    int roi_num = DataUtility::byteArraytoInt(content.sliced(46,1));
    int result_num = DataUtility::byteArraytoInt(content.sliced(47,1));
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoDateTime(content.sliced(44,4)))); //element, roi, result

    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(50,2))))); // Led level
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(48,2))))); // Camera Gain

    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoFloat(content.sliced(52,4))))); // Temperature
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoFloat(content.sliced(56,4))))); // Humidity

    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(62,2))))); // sampling index
    m_summary_items.append(new QStandardItem(QString::number(DataUtility::byteArraytoInt(content.sliced(60,2))))); // code number

    m_summary_items.append(new QStandardItem(QString(content.sliced(64,12).toHex()))); // note
    m_summary_items.append(new QStandardItem(QString(content.sliced(76,4).toHex()))); // reserve

    int offset = 80;
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,12), DataUtility::FLOAT))); // Whiteboard calibrated bgr
    offset+=12;
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,meter_param_size*4), DataUtility::FLOAT))); // meter params
    offset+=(meter_param_size*4);
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,roi_num*3*4), DataUtility::FLOAT))); // Final signal
    offset+=(roi_num*3*4);
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,result_num*4), DataUtility::FLOAT))); // Final Result
    offset+=((result_num*4));
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,3*4), DataUtility::FLOAT))); // Whiteboard BGR
    offset+=(3*4);
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,16), DataUtility::UINT32))); // Strip Area
    offset+=16;
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,roi_num*3*4), DataUtility::FLOAT))); // Initial ROIs BGR
    offset+=(roi_num*3);
    m_summary_items.append(new QStandardItem(DataUtility::byteArraytoComma(content.sliced(offset,roi_num*4*4), DataUtility::UINT32))); // ROIs initial/final center
    offset+=(roi_num*4*4);

    m_summary_model->appendColumn(m_summary_items);
}

void MainWindow::WriteDataRecord(QByteArray content, QTextStream &txt)
{
    //Header
    txt << "$Header Magic\n" << QString(content.first(4).toHex()) << "\n";
    txt << "$Header Result Length\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(4,4))) << "\n";
    txt << "$Header CRC32\n" << QString(content.sliced(8,4).toHex()) << "\n";
    txt << "$Header Reserve\n" << QString(content.sliced(12,8).toHex()) << "\n";
    txt << "\n";

    //Data static
    txt << "$Magic\n" << QString(content.sliced(20,4).toHex()) << "\n";
    txt << "$DataRecord Version\nv" << QString::number(DataUtility::byteArraytoInt(content.sliced(26,2))) << "\n";
    txt << "$Unit Mode\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(25,1))) << "\n";

    int meter_param_size = DataUtility::byteArraytoInt(content.sliced(24,1));
    txt << "$Color Parameter Size\n" << QString::number(meter_param_size) << "\n";

    txt << "$Kernel Version\nv" << QString::number(DataUtility::byteArraytoInt(content.sliced(30,1))) << "." << QString::number(DataUtility::byteArraytoInt(content.sliced(29,1))) << "." << QString::number(DataUtility::byteArraytoInt(content.sliced(28,1))) << "\n";
    txt << "$Date&Time\n" << DataUtility::byteArraytoDateTime(content.sliced(32,8)) << "\n";
    txt << "$Test Type\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(43,1))) << "\n";
    txt << "$Sample Type\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(42,1))) << "\n";
    txt << "$State\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(41,1))) << "\n";
    txt << "$Status\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(40,1))) << "\n";

    int element_num = DataUtility::byteArraytoInt(content.sliced(44,2));
    int roi_num = DataUtility::byteArraytoInt(content.sliced(46,1));
    int result_num = DataUtility::byteArraytoInt(content.sliced(47,1));
    txt << "$Result, ROI, Element Number\n" << QString::number(result_num) << ", " << QString::number(roi_num) << ", " << QString::number(element_num) << "\n";

    txt << "$Led Level\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(50,2))) << "\n";
    txt << "$Camera Gain\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(48,2))) << "\n";

    txt << "$Temperature\n" << QString::number(DataUtility::byteArraytoFloat(content.sliced(52,4))) << "\n";
    txt << "$Humidity\n" << QString::number(DataUtility::byteArraytoFloat(content.sliced(56,4))) << "\n";

    txt << "$Sampling Index\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(62,2))) << "\n";
    txt << "$Code Number\n" << QString::number(DataUtility::byteArraytoInt(content.sliced(60,2))) << "\n";

    txt << "$Note\n" << QString(content.sliced(64,12).toHex()) << "\n";
    txt << "$Reserve\n" << QString(content.sliced(76,4).toHex()) << "\n";
    txt << "\n";

    //Data dynamic
    int offset = 80;
    txt << "$Whiteboard Calibrated BGR\n" << DataUtility::byteArraytoComma(content.sliced(offset,12), DataUtility::FLOAT) << "\n";
    offset += 12;

    txt << "$Color Parameter\n" << DataUtility::byteArraytoComma(content.sliced(offset,meter_param_size*4), DataUtility::FLOAT) << "\n";
    offset += (meter_param_size*4);

    txt << "$Final Signal\n" << DataUtility::byteArraytoComma(content.sliced(offset,roi_num*3*4), DataUtility::FLOAT) << "\n";
    offset += (roi_num*3*4);

    txt << "$Final Result Value\n" << DataUtility::byteArraytoComma(content.sliced(offset,result_num*4), DataUtility::FLOAT) << "\n";
    offset += (result_num*4);

    txt << "$Whiteboard BGR\n" << DataUtility::byteArraytoComma(content.sliced(offset,3*4), DataUtility::FLOAT) << "\n";
    offset += (3*4);

    txt << "$Strip Area\n" << DataUtility::byteArraytoComma(content.sliced(offset,16), DataUtility::UINT32) << "\n";
    offset += 16;

    for(size_t roii = 0; roii < roi_num; roii++){
        txt << "$Initial ROI" << (roii+1) << " BGR\n" << DataUtility::byteArraytoComma(content.sliced(offset,3*4), DataUtility::FLOAT) << "\n";
        offset += (3*4);
    }

    for(size_t roii = 0; roii < roi_num; roii++){
        txt << "$ROI" << (roii+1) << " initial center\n" << DataUtility::byteArraytoComma(content.sliced(offset,8), DataUtility::UINT32) << "\n";
        offset += 8;
        txt << "$ROI" << (roii+1) << " final center\n" << DataUtility::byteArraytoComma(content.sliced(offset,8), DataUtility::UINT32) << "\n";
        offset += 8;
    }

    txt << "\n";

    for(size_t roii = 0; roii < roi_num; roii++){
        txt << "$ROI" << (roii+1) << " B\n" << DataUtility::byteArraytoComma(content.sliced(offset,element_num), DataUtility::UCHAR) << "\n";
        offset += element_num;

        txt << "\n";
        txt << "$ROI" << (roii+1) << " G\n" << DataUtility::byteArraytoComma(content.sliced(offset,element_num), DataUtility::UCHAR) << "\n";
        offset += element_num;

        txt << "\n";
        txt << "$ROI" << (roii+1) << " R\n" << DataUtility::byteArraytoComma(content.sliced(offset,element_num), DataUtility::UCHAR) << "\n";
        offset += element_num;
        txt << "\n";

    }

    for(size_t roii = 0; roii < roi_num; roii++){
        txt << "$ROI" << (roii+1) << " First Chunk B\n" << DataUtility::byteArraytoComma(content.sliced(offset,5*4), DataUtility::FLOAT) << "\n";
        offset += 5*4;

        txt << "\n";
        txt << "$ROI" << (roii+1) << " First Chunk G\n" << DataUtility::byteArraytoComma(content.sliced(offset,5*4), DataUtility::FLOAT) << "\n";
        offset += 5*4;

        txt << "\n";
        txt << "$ROI" << (roii+1) << " First Chunk R\n" << DataUtility::byteArraytoComma(content.sliced(offset,5*4), DataUtility::FLOAT) << "\n";
        offset += 5*4;
        txt << "\n";

        txt << "$ROI" << (roii+1) << " Last Chunk B\n" << DataUtility::byteArraytoComma(content.sliced(offset,5*4), DataUtility::FLOAT) << "\n";
        offset += 5*4;

        txt << "\n";
        txt << "$ROI" << (roii+1) << " Last Chunk G\n" << DataUtility::byteArraytoComma(content.sliced(offset,5*4), DataUtility::FLOAT) << "\n";
        offset += 5*4;

        txt << "\n";
        txt << "$ROI" << (roii+1) << " Last Chunk R\n" << DataUtility::byteArraytoComma(content.sliced(offset,5*4), DataUtility::FLOAT) << "\n";
        offset += 5*4;
        txt << "\n";

    }

    txt << "$Estimate HCT\n" << QString::number(DataUtility::byteArraytoFloat(content.sliced(offset,4))) << "\n";
    offset += 4;
    txt << "\n"; // end
}


