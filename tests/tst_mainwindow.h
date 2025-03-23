/**
 * @file tst_mainwindow.h
 * @brief Plik nagłówkowy dla testów jednostkowych klasy MainWindow.
 *
 * Zawiera testy jednostkowe dla metod klasy MainWindow, w tym calculateDistance
 * i getStationsForCity.
 */

 #ifndef TST_MAINWINDOW_H
 #define TST_MAINWINDOW_H
 
 #include <QtTest/QtTest>
 #include "../mainwindow.h"
 
 /**
  * @brief Klasa testowa dla MainWindow.
  *
  * Zawiera testy jednostkowe dla metod calculateDistance i getStationsForCity.
  */
 class TestMainWindow : public QObject {
     Q_OBJECT
 
 private slots:
     /**
      * @brief Test metody calculateDistance.
      *
      * Sprawdza, czy metoda poprawnie oblicza odległość między dwoma punktami
      * na Ziemi, używając wzoru haversine.
      */
     void testCalculateDistance();
 
     /**
      * @brief Test metody getStationsForCity.
      *
      * Sprawdza, czy metoda poprawnie filtruje stacje z historii dla podanej miejscowości
      * i zwraca tylko najnowsze wpisy dla każdej stacji.
      */
     void testGetStationsForCity();
 };
 
 #endif // TST_MAINWINDOW_H