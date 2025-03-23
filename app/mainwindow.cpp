#include "mainwindow.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDateTime>
#include <cmath>

/** @brief Stała reprezentująca wartość liczby Pi. */
const double PI = 3.14159265358979323846;

/**
 * @brief Konstruktor klasy MainWindow.
 *
 * Inicjalizuje menedżer sieciowy, ustawia połączenia sygnałów i slotów dla odpowiedzi z API,
 * oraz ładuje historię wyszukiwania z pliku.
 *
 * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
 */
MainWindow::MainWindow(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        if (reply->url().toString().contains("station/findAll")) {
            processStationsReply(reply);
        } else if (reply->url().toString().contains("station/sensors")) {
            processSensorsReply(reply);
        } else if (reply->url().toString().contains("data/getData")) {
            processDataReply(reply);
        } else if (reply->url().toString().contains("aqindex/getIndex")) {
            processIndexReply(reply);
        }
    });
    m_userLocation = QVariantMap();
    m_userAddress = QString();
    loadHistory();
}

/**
 * @brief Pobiera wszystkie stacje z API.
 *
 * Czyści bieżące dane (lokalizację użytkownika, adres, wynik), ustawia flagę isFromHistory na false,
 * a następnie wysyła żądanie do API w celu pobrania listy wszystkich stacji pomiarowych.
 * W przypadku błędu sieciowego emituje sygnał networkError.
 */
void MainWindow::fetchAllStations() {
    m_userLocation.clear();
    m_userAddress.clear();
    m_result.clear();
    emit userLocationChanged();
    emit userAddressChanged();
    emit resultChanged();
    m_isFromHistory = false;
    emit isFromHistoryChanged();
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("https://api.gios.gov.pl/pjp-api/rest/station/findAll")));
    connect(reply, &QNetworkReply::errorOccurred, this, [this]() {
        emit networkError();
    });
}

/**
 * @brief Pobiera stacje dla danej miejscowości z API.
 *
 * Czyści bieżące dane (lokalizację użytkownika, adres), ustawia wynik na nazwę miejscowości,
 * ustawia flagę isFromHistory na false, a następnie wysyła żądanie do API w celu pobrania listy
 * wszystkich stacji. Filtruje stacje po stronie klienta na podstawie nazwy miejscowości.
 *
 * @param city Nazwa miejscowości (np. "Warszawa").
 */
void MainWindow::fetchStationsByCity(const QString &city) {
    m_userLocation.clear();
    m_userAddress.clear();
    m_result = city;
    emit userLocationChanged();
    emit userAddressChanged();
    emit resultChanged();
    m_isFromHistory = false;
    emit isFromHistoryChanged();
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("https://api.gios.gov.pl/pjp-api/rest/station/findAll")));
    connect(reply, &QNetworkReply::errorOccurred, this, [this]() {
        emit networkError();
    });
}

/**
 * @brief Pobiera najbliższą stację dla podanej lokalizacji.
 *
 * Przekształca podaną lokalizację na współrzędne geograficzne, ustawia je jako lokalizację użytkownika,
 * a następnie wysyła żądanie do API w celu pobrania listy wszystkich stacji. Po otrzymaniu odpowiedzi
 * wybiera stację najbliższą podanym współrzędnym.
 *
 * @param location Adres lokalizacji (np. "Wałcz ul. Południowa 10").
 */
void MainWindow::fetchNearestStation(const QString &location) {
    QGeoCoordinate coords = getLocationCoords(location);
    if (coords.isValid()) {
        m_userLocation["lat"] = coords.latitude();
        m_userLocation["lon"] = coords.longitude();
        emit userLocationChanged();
        m_userAddress = location;
        emit userAddressChanged();
        m_result = QString("%1 %2").arg(coords.latitude()).arg(coords.longitude());
        emit resultChanged();
        m_isFromHistory = false;
        emit isFromHistoryChanged();
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("https://api.gios.gov.pl/pjp-api/rest/station/findAll")));
        connect(reply, &QNetworkReply::errorOccurred, this, [this]() {
            emit networkError();
        });
    } else {
        m_result = "Nie znaleziono lokalizacji: " + location;
        emit resultChanged();
        emit networkError();
    }
}

/**
 * @brief Pobiera szczegóły wybranej stacji (czujniki i indeks jakości powietrza).
 *
 * Czyści bieżące szczegóły stacji, ustawia identyfikator stacji, a następnie wysyła dwa żądania do API:
 * jedno dla czujników stacji, a drugie dla indeksu jakości powietrza. Wyniki są przetwarzane asynchronicznie.
 *
 * @param stationId Identyfikator stacji.
 */
void MainWindow::fetchStationDetails(int stationId) {
    m_stationDetails.clear();
    m_stationDetails["stationId"] = stationId;

    m_pendingRequests[stationId] = 2;

    QNetworkReply *sensorsReply = manager->get(QNetworkRequest(QUrl(QString("https://api.gios.gov.pl/pjp-api/rest/station/sensors/%1").arg(stationId))));
    QNetworkReply *indexReply = manager->get(QNetworkRequest(QUrl(QString("https://api.gios.gov.pl/pjp-api/rest/aqindex/getIndex/%1").arg(stationId))));
    connect(sensorsReply, &QNetworkReply::errorOccurred, this, [this]() {
        emit networkError();
    });
    connect(indexReply, &QNetworkReply::errorOccurred, this, [this]() {
        emit networkError();
    });
}

/**
 * @brief Przetwarza odpowiedź z API dotyczącą listy stacji.
 *
 * Parsuje odpowiedź JSON z API, tworząc listę stacji z ich danymi (id, nazwa, współrzędne, miasto, itd.).
 * Następnie filtruje stacje w zależności od trybu (wszystkie stacje, stacje w miejscowości, najbliższa stacja).
 * W przypadku błędu sieciowego ustawia odpowiedni komunikat i emituje sygnał networkError.
 *
 * @param reply Odpowiedź z API.
 */
void MainWindow::processStationsReply(QNetworkReply *reply) {
    m_stations.clear();
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray stations = doc.array();

        for (const QJsonValue &value : stations) {
            QJsonObject obj = value.toObject();
            if (obj["id"].isNull() || obj["gegrLat"].isNull() || obj["gegrLon"].isNull()) {
                continue;
            }

            QVariantMap map;
            map["id"] = obj["id"].toInt();
            map["name"] = obj["stationName"].toString();
            bool latOk, lonOk;
            double lat = obj["gegrLat"].toString().toDouble(&latOk);
            double lon = obj["gegrLon"].toString().toDouble(&lonOk);
            if (!latOk || !lonOk) {
                continue;
            }
            map["lat"] = lat;
            map["lon"] = lon;
            QJsonObject city = obj["city"].toObject();
            map["cityName"] = city["name"].toString();
            QJsonObject commune = city["commune"].toObject();
            map["communeName"] = commune["communeName"].toString();
            map["districtName"] = commune["districtName"].toString();
            map["provinceName"] = commune["provinceName"].toString();
            map["addressStreet"] = obj["addressStreet"].toString();
            map["savedToHistory"] = false;
            m_stations.append(map);
        }

        if (m_result.isEmpty()) {
            // Tryb "Wszystkie stacje"
        } else if (!m_result.contains(" ")) {
            QString city = m_result;
            QVariantList filteredStations;
            for (const QVariant &station : m_stations) {
                QVariantMap map = station.toMap();
                if (map["cityName"].toString() == city) {
                    filteredStations.append(map);
                }
            }
            m_stations = filteredStations;
            if (m_stations.isEmpty()) {
                m_result = "Nie znaleziono stacji w miejscowości " + city;
            } else {
                m_result = "";
            }
        } else {
            QStringList coords = m_result.split(" ");
            double lat = coords[0].toDouble();
            double lon = coords[1].toDouble();
            double minDist = std::numeric_limits<double>::max();
            QVariantMap closest;

            for (const QVariant &station : m_stations) {
                QVariantMap map = station.toMap();
                double sLat = map["lat"].toDouble();
                double sLon = map["lon"].toDouble();
                double dist = calculateDistance(lat, lon, sLat, sLon);
                if (dist < minDist) {
                    minDist = dist;
                    closest = map;
                    closest["dist"] = minDist;
                }
            }
            m_stations.clear();
            m_stations.append(closest);
            m_result = QString("Najbliższa stacja: %1, Odległość: %2 km").arg(closest["name"].toString()).arg(minDist, 0, 'f', 2);
        }
    } else {
        m_result = "Brak połączenia z internetem/bazą danych";
        emit resultChanged();
        emit networkError();
        reply->deleteLater();
        return;
    }

    emit stationsChanged();
    emit resultChanged();
    reply->deleteLater();
}

/**
 * @brief Przetwarza odpowiedź z API dotyczącą czujników stacji.
 *
 * Parsuje odpowiedź JSON z API, tworząc listę czujników dla danej stacji. Dla każdego czujnika
 * wysyła dodatkowe żądanie w celu pobrania danych pomiarowych. Aktualizuje szczegóły stacji
 * i emituje sygnał stationDetailsChanged, gdy wszystkie żądania zostaną zakończone.
 *
 * @param reply Odpowiedź z API.
 */
void MainWindow::processSensorsReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray sensors = doc.array();
        QVariantList sensorsList;

        int stationId = m_stationDetails["stationId"].toInt();

        for (const QJsonValue &value : sensors) {
            QJsonObject obj = value.toObject();
            QVariantMap sensor;
            sensor["id"] = obj["id"].toInt();
            sensor["paramName"] = obj["param"].toObject()["paramName"].toString();
            sensor["paramFormula"] = obj["param"].toObject()["paramFormula"].toString();
            sensorsList.append(sensor);
            m_pendingRequests[stationId]++;
            manager->get(QNetworkRequest(QUrl(QString("https://api.gios.gov.pl/pjp-api/rest/data/getData/%1").arg(sensor["id"].toInt()))));
        }
        m_stationDetails["sensors"] = sensorsList;

        m_pendingRequests[stationId]--;
    } else {
        emit networkError();
    }
    reply->deleteLater();
}

/**
 * @brief Przetwarza odpowiedź z API dotyczącą danych z czujników.
 *
 * Parsuje odpowiedź JSON z API, zapisując wszystkie pomiary dla danego czujnika (nie tylko najnowszy).
 * Aktualizuje szczegóły stacji i emituje sygnał stationDetailsChanged, gdy wszystkie żądania
 * dla danej stacji zostaną zakończone.
 *
 * @param reply Odpowiedź z API.
 */
void MainWindow::processDataReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QString key = obj["key"].toString();
        QJsonArray values = obj["values"].toArray();
        QVariantList measurements;

        int stationId = m_stationDetails["stationId"].toInt();

        // Zapisujemy wszystkie pomiary, a nie tylko pierwszy
        for (const QJsonValue &value : values) {
            QJsonObject measurement = value.toObject();
            if (!measurement["value"].isNull()) {
                QVariantMap data;
                data["date"] = measurement["date"].toString();
                data["value"] = measurement["value"].toDouble();
                measurements.append(data);
            }
        }

        QVariantList sensors = m_stationDetails["sensors"].toList();
        for (int i = 0; i < sensors.size(); ++i) {
            QVariantMap sensor = sensors[i].toMap();
            if (sensor["paramFormula"].toString() == key) {
                sensor["measurements"] = measurements;
                sensors[i] = sensor;
                break;
            }
        }
        m_stationDetails["sensors"] = sensors;
        emit stationDetailsChanged();

        m_pendingRequests[stationId]--;
    } else {
        emit networkError();
    }
    reply->deleteLater();
}

/**
 * @brief Przetwarza odpowiedź z API dotyczącą indeksu jakości powietrza.
 *
 * Parsuje odpowiedź JSON z API, zapisując dane o indeksie jakości powietrza (data obliczenia,
 * poziom indeksu, nazwa poziomu). Aktualizuje szczegóły stacji i emituje sygnał stationDetailsChanged,
 * gdy wszystkie żądania dla danej stacji zostaną zakończone.
 *
 * @param reply Odpowiedź z API.
 */
void MainWindow::processIndexReply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        QVariantMap index;
        index["calcDate"] = obj["stCalcDate"].toString();
        QJsonObject stIndex = obj["stIndexLevel"].toObject();
        index["indexLevel"] = stIndex["id"].toInt();
        index["indexLevelName"] = stIndex["indexLevelName"].toString();
        m_stationDetails["airQualityIndex"] = index;
        emit stationDetailsChanged();

        int stationId = m_stationDetails["stationId"].toInt();

        m_pendingRequests[stationId]--;
    } else {
        emit networkError();
    }
    reply->deleteLater();
}

/**
 * @brief Pobiera współrzędne geograficzne dla podanej lokalizacji za pomocą API Nominatim.
 *
 * Wysyła synchroniczne żądanie do API Nominatim w celu uzyskania współrzędnych geograficznych
 * dla podanej lokalizacji. Zwraca współrzędne, jeśli lokalizacja zostanie znaleziona, lub
 * nieważne współrzędne w przypadku błędu.
 *
 * @param location Adres lokalizacji (np. "Wałcz ul. Południowa 10").
 * @return QGeoCoordinate Współrzędne geograficzne (lub nieważne, jeśli nie znaleziono).
 */
QGeoCoordinate MainWindow::getLocationCoords(const QString &location) {
    QNetworkAccessManager tempManager;
    QNetworkRequest request(QUrl("https://nominatim.openstreetmap.org/search?q=" + location + ",+Polska&format=json&limit=1"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "QtApp/1.0");
    QNetworkReply *reply = tempManager.get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray data = doc.array();
        if (!data.isEmpty()) {
            QJsonObject obj = data[0].toObject();
            return QGeoCoordinate(obj["lat"].toString().toDouble(), obj["lon"].toString().toDouble());
        }
    }
    reply->deleteLater();
    return QGeoCoordinate();
}

/**
 * @brief Oblicza odległość między dwoma punktami na Ziemi (w kilometrach).
 *
 * Używa wzoru haversine do obliczenia odległości między dwoma punktami na powierzchni Ziemi
 * na podstawie ich współrzędnych geograficznych.
 *
 * @param lat1 Szerokość geograficzna pierwszego punktu.
 * @param lon1 Długość geograficzna pierwszego punktu.
 * @param lat2 Szerokość geograficzna drugiego punktu.
 * @param lon2 Długość geograficzna drugiego punktu.
 * @return double Odległość w kilometrach.
 */
double MainWindow::calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371;
    double dLat = (lat2 - lat1) * PI / 180;
    double dLon = (lon2 - lon1) * PI / 180;
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * PI / 180) * std::cos(lat2 * PI / 180) *
                   std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

/**
 * @brief Zapisuje stację do historii wyszukiwania (przeciążona wersja z QVariantMap).
 *
 * Dodaje stację do historii wyszukiwania, zapisując jej dane (w tym indeks jakości powietrza
 * i pomiary z czujników) wraz z aktualnym znacznikiem czasu. Historia jest zapisywana do pliku JSON.
 *
 * @param station Dane stacji w formacie QVariantMap.
 */
void MainWindow::saveStationToHistory(const QVariantMap &station) {
    QVariantMap historyEntry = station;
    historyEntry["airQualityIndex"] = m_stationDetails["airQualityIndex"];
    historyEntry["sensors"] = m_stationDetails["sensors"];
    historyEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file("history.json");
    QVariantList history;
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        history = doc.toVariant().toList();
        file.close();
    }

    history.prepend(historyEntry);

    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(QJsonArray::fromVariantList(history));
        file.write(doc.toJson());
        file.close();
    }

    m_history = history;
    emit historyChanged();
}

/**
 * @brief Zapisuje stację do historii wyszukiwania na podstawie jej identyfikatora.
 *
 * Wyszukuje stację o podanym identyfikatorze w liście stacji, a następnie zapisuje jej dane
 * (w tym indeks jakości powietrza i pomiary z czujników) do historii wyszukiwania.
 * Sprawdza, czy stacja nie została już zapisana, aby uniknąć duplikatów.
 *
 * @param stationId Identyfikator stacji do zapisania.
 */
void MainWindow::saveStationToHistory(int stationId) {
    for (int i = 0; i < m_stations.size(); ++i) {
        QVariantMap station = m_stations[i].toMap();
        if (station["id"].toInt() == stationId) {
            if (station.contains("savedToHistory") && station["savedToHistory"].toBool()) {
                return;
            }

            QVariantMap historyEntry = station;
            historyEntry["airQualityIndex"] = m_stationDetails["airQualityIndex"];
            historyEntry["sensors"] = m_stationDetails["sensors"];
            historyEntry["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

            QFile file("history.json");
            QVariantList history;
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                history = doc.toVariant().toList();
                file.close();
            }

            history.prepend(historyEntry);

            if (file.open(QIODevice::WriteOnly)) {
                QJsonDocument doc(QJsonArray::fromVariantList(history));
                file.write(doc.toJson());
                file.close();
            }

            m_history = history;
            emit historyChanged();

            station["savedToHistory"] = true;
            m_stations[i] = station;
            emit stationsChanged();
            break;
        }
    }
}

/**
 * @brief Ładuje historię wyszukiwania z pliku.
 *
 * Odczytuje dane historii z pliku "history.json" i zapisuje je do zmiennej m_history.
 * Emituje sygnał historyChanged po zakończeniu operacji.
 */
void MainWindow::loadHistory() {
    QFile file("history.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        m_history = doc.toVariant().toList();
        file.close();
    }
    emit historyChanged();
}

/**
 * @brief Wyświetla stację z historii wyszukiwania.
 *
 * Wczytuje stację z historii na podstawie podanego indeksu, ustawia ją jako bieżącą stację,
 * czyści dane użytkownika (lokalizację i adres), ustawia flagę isFromHistory na true,
 * a następnie aktualizuje interfejs użytkownika.
 *
 * @param index Indeks stacji w historii.
 */
void MainWindow::displayStationFromHistory(int index) {
    if (index < 0 || index >= m_history.size()) return;

    QVariantMap station = m_history[index].toMap();
    station["savedToHistory"] = true;
    m_stations.clear();
    m_stations.append(station);
    m_userLocation.clear();
    m_userAddress.clear();
    emit userLocationChanged();
    emit userAddressChanged();

    m_stationDetails = station;
    m_stationDetails["stationId"] = station["id"];
    emit stationDetailsChanged();

    m_isFromHistory = true;
    emit isFromHistoryChanged();
    emit stationsChanged();
}

/**
 * @brief Usuwa stację z historii wyszukiwania.
 *
 * Usuwa stację z historii na podstawie podanego indeksu i zapisuje zaktualizowaną historię
 * do pliku "history.json". Emituje sygnał historyChanged po zakończeniu operacji.
 *
 * @param index Indeks stacji do usunięcia.
 */
void MainWindow::removeStationFromHistory(int index) {
    if (index < 0 || index >= m_history.size()) return;

    m_history.removeAt(index);

    QFile file("history.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(QJsonArray::fromVariantList(m_history));
        file.write(doc.toJson());
        file.close();
    }

    emit historyChanged();
}

/**
 * @brief Czyści całą historię wyszukiwania.
 *
 * Usuwa wszystkie wpisy z historii wyszukiwania i zapisuje pustą historię do pliku "history.json".
 * Emituje sygnał historyChanged po zakończeniu operacji.
 */
void MainWindow::clearHistory() {
    m_history.clear();

    QFile file("history.json");
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(QJsonArray::fromVariantList(m_history));
        file.write(doc.toJson());
        file.close();
    }

    emit historyChanged();
}

/**
 * @brief Pobiera stacje zapisane w historii dla danej miejscowości.
 *
 * Wyszukuje w historii stacje dla podanej miejscowości, zwracając tylko najnowsze wpisy
 * dla każdej stacji (na podstawie identyfikatora stacji). Porównanie jest niewrażliwe na wielkość liter.
 *
 * @param city Nazwa miejscowości.
 * @return QVariantList Lista stacji z historii dla podanej miejscowości.
 */
QVariantList MainWindow::getStationsForCity(const QString &city) {
    QMap<int, QVariantMap> latestStations; // Mapa przechowująca najnowsze zapisy dla każdej stacji (klucz: id stacji)

    for (const QVariant &entry : m_history) {
        QVariantMap station = entry.toMap();
        if (station["cityName"].toString().toLower() == city.toLower()) {
            int stationId = station["id"].toInt();
            if (!latestStations.contains(stationId)) {
                // Jeśli nie mamy jeszcze tej stacji, dodajemy ją
                latestStations[stationId] = station;
            } else {
                // Jeśli mamy już tę stację, porównujemy daty i zapisujemy nowszą
                QDateTime currentTimestamp = QDateTime::fromString(station["timestamp"].toString(), Qt::ISODate);
                QDateTime existingTimestamp = QDateTime::fromString(latestStations[stationId]["timestamp"].toString(), Qt::ISODate);
                if (currentTimestamp > existingTimestamp) {
                    latestStations[stationId] = station;
                }
            }
        }
    }

    // Konwertujemy QList<QMap<QString, QVariant>> na QVariantList
    QVariantList result;
    for (const QVariantMap &station : latestStations.values()) {
        result.append(station);
    }

    return result; // Zwracamy QVariantList
}
