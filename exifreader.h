#pragma once
#include <QObject>
#include <QVariantMap>

class ExifReader : public QObject {
    Q_OBJECT
public:
    explicit ExifReader(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap readExif(const QString &fileUrl);
};
