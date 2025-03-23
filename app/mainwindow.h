#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QGeoCoordinate>
#include <QVariantList>
#include <QVariantMap>
#include <QEventLoop>

/**
 * @brief Klasa MainWindow zarządza logiką aplikacji do wyszukiwania i analizy stacji pomiarowych.
 *
 * Klasa ta odpowiada za komunikację z API, pobieranie danych o stacjach, ich czujnikach i pomiarach,
 * a także za zarządzanie historią wyszukiwania i dostarczanie danych do interfejsu QML.
 */
class MainWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString result READ result NOTIFY resultChanged)
    Q_PROPERTY(QVariantList stations READ stations NOTIFY stationsChanged)
    Q_PROPERTY(QVariantMap userLocation READ userLocation NOTIFY userLocationChanged)
    Q_PROPERTY(QString userAddress READ userAddress NOTIFY userAddressChanged)
    Q_PROPERTY(QVariantMap stationDetails READ stationDetails NOTIFY stationDetailsChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(bool isFromHistory READ isFromHistory NOTIFY isFromHistoryChanged)

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
     */
    explicit MainWindow(QObject *parent = nullptr);

    /**
     * @brief Pobiera wynik ostatniej operacji (np. komunikat o błędzie lub sukcesie).
     * @return QString Wynik operacji.
     */
    QString result() const { return m_result; }

    /**
     * @brief Pobiera listę wszystkich stacji z API.
     * @return QVariantList Lista stacji w formacie QVariantList.
     */
    QVariantList stations() const { return m_stations; }

    /**
     * @brief Pobiera lokalizację użytkownika (jeśli ustawiona).
     * @return QVariantMap Mapa z kluczami "lat" i "lon" reprezentującymi współrzędne.
     */
    QVariantMap userLocation() const { return m_userLocation; }

    /**
     * @brief Pobiera adres użytkownika (jeśli ustawiony).
     * @return QString Adres użytkownika.
     */
    QString userAddress() const { return m_userAddress; }

    /**
     * @brief Pobiera szczegóły wybranej stacji (np. dane z czujników).
     * @return QVariantMap Mapa zawierająca szczegóły stacji.
     */
    QVariantMap stationDetails() const { return m_stationDetails; }

    /**
     * @brief Pobiera historię wyszukiwania.
     * @return QVariantList Lista zapisanych stacji w historii.
     */
    QVariantList history() const { return m_history; }

    /**
     * @brief Sprawdza, czy aktualnie wyświetlane dane pochodzą z historii.
     * @return bool True, jeśli dane pochodzą z historii; False w przeciwnym razie.
     */
    bool isFromHistory() const { return m_isFromHistory; }

public slots:
    /**
     * @brief Pobiera wszystkie stacje z API.
     *
     * Wysyła żądanie do API w celu pobrania listy wszystkich stacji pomiarowych.
     * Wyniki są przetwarzane asynchronicznie.
     */
    void fetchAllStations();

    /**
     * @brief Pobiera stacje dla danej miejscowości.
     * @param city Nazwa miejscowości (np. "Warszawa").
     */
    void fetchStationsByCity(const QString &city);

    /**
     * @brief Pobiera najbliższą stację dla podanej lokalizacji.
     * @param location Adres lokalizacji (np. "Wałcz ul. Południowa 10").
     */
    void fetchNearestStation(const QString &location);

    /**
     * @brief Pobiera szczegóły wybranej stacji (czujniki i indeks jakości powietrza).
     * @param stationId Identyfikator stacji.
     */
    void fetchStationDetails(int stationId);

    /**
     * @brief Ładuje historię wyszukiwania z pliku.
     */
    void loadHistory();

    /**
     * @brief Wyświetla stację z historii wyszukiwania.
     * @param index Indeks stacji w historii.
     */
    void displayStationFromHistory(int index);

    /**
     * @brief Usuwa stację z historii wyszukiwania.
     * @param index Indeks stacji do usunięcia.
     */
    void removeStationFromHistory(int index);

    /**
     * @brief Czyści całą historię wyszukiwania.
     */
    void clearHistory();

    /**
     * @brief Zapisuje stację do historii wyszukiwania na podstawie jej identyfikatora.
     * @param stationId Identyfikator stacji do zapisania.
     */
    void saveStationToHistory(int stationId);

    /**
     * @brief Pobiera stacje zapisane w historii dla danej miejscowości.
     *
     * Wyszukuje w historii stacje dla podanej miejscowości, zwracając tylko najnowsze wpisy
     * dla każdej stacji (na podstawie identyfikatora stacji).
     *
     * @param city Nazwa miejscowości.
     * @return QVariantList Lista stacji z historii dla podanej miejscowości.
     */
    QVariantList getStationsForCity(const QString &city);

signals:
    /**
     * @brief Sygnał emitowany, gdy wynik operacji zostanie zaktualizowany.
     */
    void resultChanged();

    /**
     * @brief Sygnał emitowany, gdy lista stacji zostanie zaktualizowana.
     */
    void stationsChanged();

    /**
     * @brief Sygnał emitowany, gdy lokalizacja użytkownika zostanie zaktualizowana.
     */
    void userLocationChanged();

    /**
     * @brief Sygnał emitowany, gdy adres użytkownika zostanie zaktualizowany.
     */
    void userAddressChanged();

    /**
     * @brief Sygnał emitowany, gdy szczegóły stacji zostaną zaktualizowane.
     */
    void stationDetailsChanged();

    /**
     * @brief Sygnał emitowany, gdy historia wyszukiwania zostanie zaktualizowana.
     */
    void historyChanged();

    /**
     * @brief Sygnał emitowany, gdy wystąpi błąd sieciowy.
     */
    void networkError();

    /**
     * @brief Sygnał emitowany, gdy zmieni się źródło danych (historia lub API).
     */
    void isFromHistoryChanged();

private slots:
    /**
     * @brief Przetwarza odpowiedź z API dotyczącą listy stacji.
     * @param reply Odpowiedź z API.
     */
    void processStationsReply(QNetworkReply *reply);

    /**
     * @brief Przetwarza odpowiedź z API dotyczącą czujników stacji.
     * @param reply Odpowiedź z API.
     */
    void processSensorsReply(QNetworkReply *reply);

    /**
     * @brief Przetwarza odpowiedź z API dotyczącą danych z czujników.
     * @param reply Odpowiedź z API.
     */
    void processDataReply(QNetworkReply *reply);

    /**
     * @brief Przetwarza odpowiedź z API dotyczącą indeksu jakości powietrza.
     * @param reply Odpowiedź z API.
     */
    void processIndexReply(QNetworkReply *reply);

private:
    /**
     * @brief Pobiera współrzędne geograficzne dla podanej lokalizacji.
     * @param location Adres lokalizacji (np. "Wałcz ul. Południowa 10").
     * @return QGeoCoordinate Współrzędne geograficzne (lub nieważne, jeśli nie znaleziono).
     */
    QGeoCoordinate getLocationCoords(const QString &location);

    /**
     * @brief Oblicza odległość między dwoma punktami na Ziemi (w kilometrach).
     * @param lat1 Szerokość geograficzna pierwszego punktu.
     * @param lon1 Długość geograficzna pierwszego punktu.
     * @param lat2 Szerokość geograficzna drugiego punktu.
     * @param lon2 Długość geograficzna drugiego punktu.
     * @return double Odległość w kilometrach.
     */
    double calculateDistance(double lat1, double lon1, double lat2, double lon2);

    /**
     * @brief Zapisuje stację do historii wyszukiwania (przeciążona wersja z QVariantMap).
     * @param station Dane stacji w formacie QVariantMap.
     */
    void saveStationToHistory(const QVariantMap &station);

    QNetworkAccessManager *manager; ///< Wskaźnik na menedżer sieciowy.
    QString m_result; ///< Wynik ostatniej operacji.
    QVariantList m_stations; ///< Lista stacji.
    QVariantMap m_userLocation; ///< Lokalizacja użytkownika (lat, lon).
    QString m_userAddress; ///< Adres użytkownika.
    QVariantMap m_stationDetails; ///< Szczegóły wybranej stacji.
    QVariantList m_history; ///< Historia wyszukiwania.
    bool m_isFromHistory = false; ///< Flaga wskazująca, czy dane pochodzą z historii.
    QMap<int, QVariantMap> m_pendingStations; ///< Mapa przechowująca oczekujące stacje (obecnie nieużywana).
    QMap<int, int> m_pendingRequests; ///< Mapa przechowująca liczbę oczekujących żądań dla każdej stacji.
};

#endif // MAINWINDOW_H
