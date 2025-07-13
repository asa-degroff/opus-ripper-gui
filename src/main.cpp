#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>

using namespace Qt::StringLiterals;

#include "controllers/ConversionController.h"
#include "models/ConversionModel.h"
#include "models/ProgressModel.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    app.setOrganizationName("OpusRipper");
    app.setOrganizationDomain("opusripper.local");
    app.setApplicationName("Opus Ripper GUI");
    
    // Register QML types
    qmlRegisterType<ConversionModel>("OpusRipperGUI", 1, 0, "ConversionModel");
    qmlRegisterType<ProgressModel>("OpusRipperGUI", 1, 0, "ProgressModel");
    qmlRegisterType<ConversionController>("OpusRipperGUI", 1, 0, "ConversionController");
    
    // Register Style singleton
    qmlRegisterSingletonType(QUrl("qrc:/qt/qml/OpusRipperGUI/qml/theme/Style.qml"), "OpusRipperGUI", 1, 0, "Style");
    
    QQmlApplicationEngine engine;
    
    // Set up import paths
    const QUrl url(u"qrc:/qt/qml/OpusRipperGUI/qml/main.qml"_s);
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    return app.exec();
}