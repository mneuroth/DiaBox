#include <QFile>
#include <QDebug>

#include "exifreader.h"
#include "TinyEXIF-1.0.4/TinyEXIF.h"

ExifReader::ExifReader(QObject *parent)
    : QObject(parent)
{}

QVariantMap ExifReader::readExif(const QString &fileUrl)
{
    QVariantMap map;

    QString path = fileUrl;
    if (path.startsWith("file://"))
        path = path.mid(7);

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file" << path;
        return map;
    }

    QByteArray data = file.readAll();
    TinyEXIF::EXIFInfo exif((const uint8_t *)data.constData(), (unsigned)data.size());

    if (!exif.Fields) {
        qWarning() << "No EXIF data found";
        return map;
    }

    map["CameraMake"] = QString::fromStdString(exif.Make);
    map["CameraModel"] = QString::fromStdString(exif.Model);
    map["DateTime"] = QString::fromStdString(exif.DateTime);
    map["Orientation"] = QString::number(exif.Orientation);
    map["FocalLength"] = QString::number(exif.FocalLength);
    map["FNumber"] = QString::number(exif.FNumber);
    map["ExposureTime"] = QString::number(exif.ExposureTime);
    map["ShutterSpeedValue"] = QString::number(exif.ShutterSpeedValue);
    map["ISOSpeedRatings"] = QString::number(exif.ISOSpeedRatings);
    map["ExposureProgram"] = QString::number(exif.ExposureProgram);
    map["FocalLengthMin"] = QString::number(exif.LensInfo.FocalLengthMin);
    map["FocalLengthMax"] = QString::number(exif.LensInfo.FocalLengthMax);

    return map;
}
