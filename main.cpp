#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>
#include <QtGlobal>
#include <QtVersion>
#include <QFileSystemModel>

#include "CacheImageProvider.h"
#include "DirectoryImageCache.h"
#include "stopwatch.h"
#include "exifreader.h"

#include <QDebug>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("DiaBox");
    app.setOrganizationName("mneuroth");

    // Use the Universal style – supports built-in dark/light theming
    QQuickStyle::setStyle("Universal");

    QQmlApplicationEngine engine;

    qmlRegisterType<Stopwatch>("Perf", 1, 0, "Stopwatch");
    qmlRegisterType<ExifReader>("Meta", 1, 0, "ExifReader");

    // Register the cache image provider
    engine.addImageProvider("cache", new CacheImageProvider(DirectoryImageCache::instance()));

    engine.rootContext()->setContextProperty("qtVersion", QString(qVersion()));

    QFileSystemModel model;
    model.setRootPath("C:/users/micha/Pictures");

    // Zeige Dateien und Verzeichnisse, aber nicht . und ..
    model.setFilter(QDir::AllDirs /*| QDir::Files*/ | QDir::NoDotAndDotDot);

    engine.rootContext()->setContextProperty("fileSystemModel", &model);
    engine.rootContext()->setContextProperty("rootModelIndex", model.index("C:/users/micha/Pictures"));

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
