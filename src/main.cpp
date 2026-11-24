#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQuickControls2/QQuickStyle>

/**
 * @brief Punto de entrada: crea la aplicación, carga la UI QML y corre el event loop.
 *
 * Los ViewModels no se crean acá: QML los instancia (ver Main.qml), gracias a que
 * están registrados con QML_ELEMENT en el módulo "PrintStation".
 *
 * @param argc Cantidad de argumentos de línea de comandos.
 * @param argv Argumentos de línea de comandos (Qt consume los suyos, ej. -platform).
 * @return Código de salida: el de QGuiApplication::exec(), o -1 si falló la carga del QML.
 */
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // "Basic" permite personalizar los controles (el estilo nativo de macOS no).
    QQuickStyle::setStyle("Basic");

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);

    engine.loadFromModule("PrintStation", "Main");
    return app.exec();
}
