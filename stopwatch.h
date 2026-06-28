#pragma once

#include <QObject>
#include <QElapsedTimer>

class Stopwatch : public QObject {
    Q_OBJECT

public:
    explicit Stopwatch(QObject *parent = nullptr, const QString &name="unknown");
    ~Stopwatch();

    Q_INVOKABLE void start();
    Q_INVOKABLE qint64 elapsedNs() const;

private:
    QString m_name;
    QElapsedTimer m_timer;
};
