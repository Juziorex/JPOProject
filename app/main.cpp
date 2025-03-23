/**
 * @file main.cpp
 * @brief Główny plik aplikacji Qt do wyszukiwania i analizy stacji pomiarowych.
 *
 * Ten plik zawiera funkcję main, która inicjalizuje aplikację QGuiApplication,
 * rejestruje typ MainWindow w QML, konfiguruje silnik QML i ładuje główny plik QML.
 */

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "mainwindow.h"

/**
 * @brief Funkcja główna aplikacji.
 *
 * Inicjalizuje aplikację Qt, rejestruje typ MainWindow jako StationFinder w QML,
 * konfiguruje silnik QML, ładuje główny plik QML (main.qml) i uruchamia pętlę zdarzeń aplikacji.
 *
 * @param argc Liczba argumentów wiersza poleceń.
 * @param argv Tablica argumentów wiersza poleceń.
 * @return int Kod wyjścia aplikacji (0 w przypadku sukcesu, -1 w przypadku błędu).
 */
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Rejestracja typu MainWindow jako StationFinder w QML
    qmlRegisterType<MainWindow>("StationFinder", 1, 0, "StationFinder");

    QQmlApplicationEngine engine;

    // Usuwamy rejestrację jako właściwość kontekstu, ponieważ teraz używamy typu QML
    // MainWindow stationManager;
    // engine.rootContext()->setContextProperty("stationManager", &stationManager);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
