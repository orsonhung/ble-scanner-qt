#ifndef DATAUTILITY_H
#define DATAUTILITY_H

#include <QString>

class DataUtility
{
public:
    enum Type{UCHAR=0, UINT32, FLOAT};
    static unsigned char byteStringToUChar(QString byte_str);
    static int byteArraytoInt(QByteArray content);
    static float byteArraytoFloat(QByteArray content);
    static QString byteArraytoDateTime(QByteArray content);
    static QString byteArraytoComma(QByteArray content, const int type=0);
};

#endif // DATAUTILITY_H
