#include "datautility.h"

unsigned char DataUtility::byteStringToUChar(QString byte_str)
{
    if((byte_str.end()-byte_str.begin()) != 2){
        return 0x00;
    }

    bool ok;
    uint hex = byte_str.toUInt(&ok, 16);
    if (!ok) {
        return 0x00;
    }
    return static_cast<unsigned char>(hex);
}

int DataUtility::byteArraytoInt(QByteArray content)
{
    int size = content.size();
    int sum = 0;
    int shift = 0;
    for(int i = 0; i < size; i++)
    {
        sum += ((static_cast<uint8_t>(content.at(i))) << shift);
        //qDebug() << "static_cast<uint8_t>(content.at("<<i<<"))) "<< static_cast<uint8_t>(content.at(i));
        shift += 8;
    }
    return sum;
}

float DataUtility::byteArraytoFloat(QByteArray content)
{
    float f;
    memcpy(&f, content, sizeof(f));
    //qDebug() << "f:"<< f;
    return f;
}

QString DataUtility::byteArraytoDateTime(QByteArray content)
{
    QString datetime = QString::number(byteArraytoInt(content.first(2))); // year
    for(size_t i = 2; i < content.size(); i++){
        if(i<=3){
            datetime.append("/");
            datetime.append(QString::number(byteArraytoInt(content.sliced(i,1))));
        }   else if(i==4)    {
            datetime.append(" ");
            datetime.append(QString::number(byteArraytoInt(content.sliced(i,1))));
        }   else    {
            datetime.append(":");
            datetime.append(QString::number(byteArraytoInt(content.sliced(i,1))));
        }

    }
    return datetime;
}

QString DataUtility::byteArraytoComma(QByteArray content, const int type)
{
    if(type==UCHAR){
        QString output = QString::number(byteArraytoInt(content.first(1)));
        for(size_t i = 1; i < content.size(); i++){
            output.append(", ");
            output.append(QString::number(byteArraytoInt(content.sliced(i,1))));
        }
        return output;
    }   else if(type==FLOAT)   {
        QString output = QString::number(byteArraytoFloat(content.first(4)));
        for(size_t i = 1; i < content.size()/4; i++){
            output.append(", ");
            output.append(QString::number(byteArraytoFloat(content.sliced(i*4,4))));
        }
        return output;
    }   else if(type==UINT32)    {
        QString output = QString::number(byteArraytoInt(content.first(4)));
        for(size_t i = 1; i < content.size()/4; i++){
            output.append(", ");
            output.append(QString::number(byteArraytoInt(content.sliced(i*4,4))));
        }
        return output;
    }   else    {
        QString output = QString::number(byteArraytoFloat(content.first(4)));
        for(size_t i = 1; i < content.size()/4; i++){
            output.append(", ");
            output.append(QString::number(byteArraytoFloat(content.sliced(i*4,4))));
        }
        return output;
    }
}
