/**
 * @file tst_mainwindow.cpp
 * @brief Implementacja testów jednostkowych dla klasy MainWindow.
 *
 * Zawiera implementacje testów dla metod calculateDistance i getStationsForCity.
 */

 #include "tst_mainwindow.h"
 #include <QDateTime>
 
 void TestMainWindow::testCalculateDistance() {
     MainWindow mainWindow;
 
     // Test 1: Odległość między dwoma punktami w Polsce
     // Warszawa (52.2297 N, 21.0122 E) i Kraków (50.0647 N, 19.9450 E)
     // Oczekiwana odległość: około 252 km (sprawdzone w narzędziach online, np. Google Maps)
     double lat1 = 52.2297;
     double lon1 = 21.0122;
     double lat2 = 50.0647;
     double lon2 = 19.9450;
     double distance = mainWindow.calculateDistance(lat1, lon1, lat2, lon2);
 
     // Sprawdzamy, czy odległość jest w rozsądnym zakresie (252 km ± 5 km, uwzględniając zaokrąglenia)
     QVERIFY2(distance >= 247 && distance <= 257, "Odległość między Warszawą a Krakowem powinna wynosić około 252 km.");
 
     // Test 2: Odległość między tym samym punktem (powinna być 0)
     distance = mainWindow.calculateDistance(lat1, lon1, lat1, lon1);
     QVERIFY2(qAbs(distance) < 0.001, "Odległość między tym samym punktem powinna wynosić 0.");
 }
 
 void TestMainWindow::testGetStationsForCity() {
     MainWindow mainWindow;
 
     // Przygotowujemy dane testowe: symulujemy historię z kilkoma stacjami
     QVariantList history;
 
     // Stacja 1: Warszawa, ID 1, starszy wpis
     QVariantMap station1;
     station1["id"] = 1;
     station1["cityName"] = "Warszawa";
     station1["name"] = "Stacja Warszawa 1";
     station1["timestamp"] = QDateTime(QDate(2023, 10, 1), QTime(12, 0)).toString(Qt::ISODate);
     history.append(station1);
 
     // Stacja 2: Warszawa, ID 1, nowszy wpis (powinien zastąpić starszy)
     QVariantMap station2;
     station2["id"] = 1;
     station2["cityName"] = "Warszawa";
     station2["name"] = "Stacja Warszawa 1 (nowsza)";
     station2["timestamp"] = QDateTime(QDate(2023, 10, 2), QTime(12, 0)).toString(Qt::ISODate);
     history.append(station2);
 
     // Stacja 3: Kraków, ID 2
     QVariantMap station3;
     station3["id"] = 2;
     station3["cityName"] = "Kraków";
     station3["name"] = "Stacja Kraków 1";
     station3["timestamp"] = QDateTime(QDate(2023, 10, 1), QTime(12, 0)).toString(Qt::ISODate);
     history.append(station3);
 
     // Ustawiamy historię w obiekcie MainWindow
     // Aby to zrobić, musimy uzyskać dostęp do prywatnego pola m_history.
     // W tym celu możemy tymczasowo zmodyfikować MainWindow, aby dodać metodę ustawiającą historię
     // lub użyć refleksji Qt, ale dla prostoty zakładamy, że możemy ustawić pole bezpośrednio
     // w teście (w praktyce należałoby dodać metodę publiczną do ustawiania historii w MainWindow).
     // Aby uniknąć modyfikacji MainWindow, zapiszemy historię do pliku i załadujemy ją.
     QFile file("history.json");
     if (file.open(QIODevice::WriteOnly)) {
         QJsonDocument doc(QJsonArray::fromVariantList(history));
         file.write(doc.toJson());
         file.close();
     }
     mainWindow.loadHistory();
 
     // Test 1: Pobieramy stacje dla Warszawy
     QVariantList warsawStations = mainWindow.getStationsForCity("Warszawa");
     QCOMPARE(warsawStations.size(), 1); // Powinna być tylko jedna stacja (najnowsza)
     QVariantMap warsawStation = warsawStations[0].toMap();
     QCOMPARE(warsawStation["id"].toInt(), 1);
     QCOMPARE(warsawStation["name"].toString(), "Stacja Warszawa 1 (nowsza)"); // Nowszy wpis
 
     // Test 2: Pobieramy stacje dla Krakowa
     QVariantList krakowStations = mainWindow.getStationsForCity("Kraków");
     QCOMPARE(krakowStations.size(), 1); // Powinna być tylko jedna stacja
     QVariantMap krakowStation = krakowStations[0].toMap();
     QCOMPARE(krakowStation["id"].toInt(), 2);
     QCOMPARE(krakowStation["name"].toString(), "Stacja Kraków 1");
 
     // Test 3: Pobieramy stacje dla nieistniejącej miejscowości
     QVariantList emptyStations = mainWindow.getStationsForCity("Poznań");
     QCOMPARE(emptyStations.size(), 0); // Powinna być pusta lista
 
     // Czyszczenie po teście
     QFile::remove("history.json");
 }
 
 QTEST_APPLESS_MAIN(TestMainWindow)