#include "stopwatch.h"

#include <QDebug>

Stopwatch::Stopwatch(QObject *parent, const QString &name)
    : QObject(parent), m_name(name)
{
    m_timer.start();
}

Stopwatch::~Stopwatch()
{
    qint64 ns = m_timer.nsecsElapsed();
    double ms = ns / 1'000'000.0;

    qDebug() << m_name << ":" << ms << "ms";
}

void Stopwatch::start() {
    m_timer.start();
}

qint64 Stopwatch::elapsedNs() const {
    return m_timer.nsecsElapsed();
}
