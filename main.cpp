#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QtGlobal>
#include <QtVersion>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("DiaBox");
    app.setOrganizationName("mneuroth");

    // Use the Universal style – supports built-in dark/light theming
    QQuickStyle::setStyle("Universal");

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("qtVersion", QString(qVersion()));

    // Load the root QML component from the registered QML module
    engine.loadFromModule("DiaBox", "Main");

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    return app.exec();
}
