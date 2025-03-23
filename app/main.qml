/**
 * @file main.qml
 * @brief Główny plik QML aplikacji do wyszukiwania i analizy stacji pomiarowych.
 *
 * Ten plik definiuje interfejs użytkownika aplikacji, w tym mapę z markerami stacji,
 * dialogi do zarządzania historią i analizy danych, oraz wykres do wizualizacji danych historycznych.
 */

import QtQuick
import QtQuick.Controls
import QtPositioning
import QtLocation
import StationFinder 1.0

/**
 * @brief Główne okno aplikacji.
 *
 * Zawiera mapę z markerami stacji pomiarowych, przyciski do wyszukiwania stacji,
 * dialogi do zarządzania historią i analizy danych, oraz wykres do wizualizacji danych historycznych.
 */
ApplicationWindow {
    id: mainWindow
    visible: true
    width: 800
    height: 600
    title: "Stacje pomiarowe"

    /**
     * @brief Instancja StationFinder do komunikacji z backendem C++.
     */
    StationFinder {
        id: finder
    }

    /**
     * @brief Główny układ interfejsu użytkownika.
     *
     * Zawiera wiersz z przyciskami i polami tekstowymi do wyszukiwania stacji,
     * tekst z wynikiem wyszukiwania oraz mapę z markerami.
     */
    Column {
        anchors.fill: parent
        spacing: 10
        padding: 10

        Row {
            spacing: 10
            Button { text: "Wszystkie stacje"; onClicked: finder.fetchAllStations() }
            TextField { id: cityField; placeholderText: "Miejscowość" }
            Button { text: "Stacje w miejscowości"; onClicked: finder.fetchStationsByCity(cityField.text) }
            TextField { id: locField; placeholderText: "Lokalizacja (np. Wałcz ul. Południowa 10)" }
            Button { text: "Najbliższa stacja"; onClicked: finder.fetchNearestStation(locField.text) }
            Button { text: "Historia"; onClicked: historyDialog.open() }
            Button { text: "Analiza danych"; onClicked: analysisDialog.open() }
        }

        Text { text: finder.result }

        /**
         * @brief Mapa wyświetlająca stacje pomiarowe.
         *
         * Używa pluginu OSM do renderowania mapy. Umożliwia przesuwanie i powiększanie mapy
         * za pomocą myszy oraz wyświetlanie markerów dla stacji i lokalizacji użytkownika.
         */
        Map {
            id: map
            width: parent.width
            height: parent.height - 100
            plugin: Plugin { name: "osm" }
            center: QtPositioning.coordinate(52.2297, 21.0122) // Polska
            zoomLevel: 6

            /**
             * @brief Obszar myszy do obsługi przesuwania i powiększania mapy.
             *
             * Umożliwia przesuwanie mapy przez przeciąganie lewym przyciskiem myszy
             * oraz powiększanie/pomniejszanie za pomocą kółka myszy.
             */
            MouseArea {
                id: mapMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                property real lastX: 0
                property real lastY: 0
                property bool isDragging: false

                onPressed: (mouse) => {
                    if (mouse.button === Qt.LeftButton) {
                        lastX = mouse.x
                        lastY = mouse.y
                        isDragging = true
                    }
                }

                onReleased: (mouse) => {
                    if (mouse.button === Qt.LeftButton) {
                        isDragging = false
                    }
                }

                onPositionChanged: (mouse) => {
                    if (isDragging) {
                        var deltaX = mouse.x - lastX
                        var deltaY = mouse.y - lastY

                        var pixelPerDegreeLat = map.height / (map.maximumLatitude - map.minimumLatitude)
                        var pixelPerDegreeLon = map.width / (map.maximumLongitude - map.minimumLongitude)

                        var deltaLat = -deltaY / pixelPerDegreeLat
                        var deltaLon = deltaX / pixelPerDegreeLon

                        var newLat = map.center.latitude + deltaLat
                        var newLon = map.center.longitude + deltaLon

                        newLat = Math.max(map.minimumLatitude, Math.min(map.maximumLatitude, newLat))
                        newLon = Math.max(map.minimumLongitude, Math.min(map.maximumLongitude, newLon))

                        map.center = QtPositioning.coordinate(newLat, newLon)

                        lastX = mouse.x
                        lastY = mouse.y
                    }
                }

                onWheel: (wheel) => {
                    var mouseX = wheel.x
                    var mouseY = wheel.y

                    var coordBefore = map.toCoordinate(Qt.point(mouseX, mouseY))

                    var zoomDelta = wheel.angleDelta.y / 120
                    var newZoomLevel = Math.max(map.minimumZoomLevel, Math.min(map.maximumZoomLevel, map.zoomLevel + zoomDelta))

                    var centerBefore = map.center

                    map.zoomLevel = newZoomLevel

                    var coordAfter = map.toCoordinate(Qt.point(mouseX, mouseY))

                    var deltaLat = coordAfter.latitude - coordBefore.latitude
                    var deltaLon = coordAfter.longitude - coordBefore.longitude

                    var newLat = centerBefore.latitude - deltaLat
                    var newLon = centerBefore.longitude - deltaLon

                    newLat = Math.max(map.minimumLatitude, Math.min(map.maximumLatitude, newLat))
                    newLon = Math.max(map.minimumLongitude, Math.min(map.maximumLongitude, newLon))

                    map.center = QtPositioning.coordinate(newLat, newLon)
                }
            }

            /**
             * @brief Połączenie z sygnałami StationFinder do aktualizacji markerów na mapie.
             *
             * Reaguje na zmiany listy stacji i błędy sieciowe, aktualizując markery na mapie
             * oraz wyświetlając odpowiednie dialogi.
             */
            Connections {
                target: finder
                function onStationsChanged() {
                    map.clearMapItems()
                    var bounds = QtPositioning.coordinate()
                    var first = true

                    if (finder.userLocation.lat !== undefined && finder.userLocation.lon !== undefined) {
                        var userCoord = QtPositioning.coordinate(finder.userLocation.lat, finder.userLocation.lon)
                        var userMarker = Qt.createQmlObject('
                            import QtQuick
                            import QtLocation
                            import QtQuick.Controls
                            MapQuickItem {
                                coordinate: QtPositioning.coordinate()
                                anchorPoint.x: sourceItem.width/2
                                anchorPoint.y: sourceItem.height
                                sourceItem: Rectangle {
                                    width: 10
                                    height: 10
                                    color: "green"
                                    radius: 5
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: userTooltip.visible = true
                                        onExited: userTooltip.visible = false
                                    }
                                    ToolTip {
                                        id: userTooltip
                                        visible: false
                                        timeout: 5000
                                        text: "To Twoja lokalizacja: ' + finder.userAddress + '"
                                    }
                                }
                            }
                        ', map)
                        userMarker.coordinate = userCoord
                        map.addMapItem(userMarker)
                        bounds = userCoord
                        first = false
                    }

                    for (var i in finder.stations) {
                        var station = finder.stations[i]
                        var coord = QtPositioning.coordinate(station.lat, station.lon)
                        var marker = Qt.createQmlObject('
                            import QtQuick
                            import QtLocation
                            import QtQuick.Controls
                            MapQuickItem {
                                property var stationData: null
                                property bool detailsFetched: false
                                coordinate: QtPositioning.coordinate()
                                anchorPoint.x: sourceItem.width/2
                                anchorPoint.y: sourceItem.height
                                sourceItem: Rectangle {
                                    width: 10
                                    height: 10
                                    color: "red"
                                    radius: 5
                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: {
                                            if (!finder.isFromHistory && !detailsFetched) {
                                                finder.fetchStationDetails(stationData.id)
                                                detailsFetched = true
                                            }
                                            tooltip.visible = true
                                        }
                                        onExited: {
                                            tooltip.visible = false
                                        }
                                        onClicked: {
                                            if (!finder.isFromHistory) {
                                                finder.saveStationToHistory(stationData.id)
                                            }
                                        }
                                    }
                                    ToolTip {
                                        id: tooltip
                                        visible: false
                                        timeout: 5000
                                        text: {
                                            var details = finder.stationDetails
                                            if (!finder.isFromHistory && (!details.stationId || details.stationId !== stationData.id)) {
                                                return "Ładowanie danych..."
                                            }
                                            var text = "Stacja: " + stationData.name + "\\n"
                                            text += "Adres: " + (stationData.addressStreet || "Brak danych") + ", " + stationData.cityName + "\\n"
                                            text += "Gmina: " + stationData.communeName + ", Powiat: " + stationData.districtName + ", Województwo: " + stationData.provinceName + "\\n"

                                            if (details.airQualityIndex) {
                                                text += "Indeks jakości powietrza: " + details.airQualityIndex.indexLevelName + " (Obliczony: " + details.airQualityIndex.calcDate + ")\\n"
                                            }

                                            if (details.sensors) {
                                                text += "Czujniki:\\n"
                                                for (var j in details.sensors) {
                                                    var sensor = details.sensors[j]
                                                    text += " - " + sensor.paramName + " (" + sensor.paramFormula + "): "
                                                    if (sensor.measurements && sensor.measurements.length > 0) {
                                                        text += sensor.measurements[0].value + " (Data: " + sensor.measurements[0].date + ")\\n"
                                                    } else {
                                                        text += "Brak danych\\n"
                                                    }
                                                }
                                            }
                                            text += "\\nKliknij marker, aby zapisać do historii"
                                            return text
                                        }
                                    }
                                }
                            }
                        ', map)
                        marker.stationData = station
                        marker.coordinate = coord
                        map.addMapItem(marker)
                    }
                }

                function onNetworkError() {
                    networkErrorDialog.open()
                }
            }

            Component.onCompleted: {
                map.onMapItemsChanged.connect(function() {
                    if (map.mapItems.length > 0) {
                        map.fitViewportToMapItems()
                        if (finder.stations.length === 1 && finder.userLocation.lat !== undefined) {
                            map.zoomLevel = 12
                        }
                    }
                })
            }
        }
    }

    /**
     * @brief Dialog wyświetlający historię wyszukiwania.
     *
     * Umożliwia przeglądanie zapisanych stacji, ich usuwanie oraz czyszczenie całej historii.
     */
    Dialog {
        id: historyDialog
        title: "Historia wyszukiwania"
        width: 400
        height: 300
        anchors.centerIn: parent
        standardButtons: Dialog.Close

        Column {
            anchors.fill: parent
            spacing: 5

            ListView {
                id: historyList
                width: parent.width
                height: parent.height - clearHistoryButton.height - parent.spacing
                model: finder.history
                delegate: Row {
                    width: parent.width
                    spacing: 5

                    ItemDelegate {
                        width: parent.width - deleteButton.width - parent.spacing
                        text: modelData.name + " (" + modelData.cityName + ") - " + modelData.timestamp
                        onClicked: {
                            finder.displayStationFromHistory(index)
                            historyDialog.close()
                        }
                    }

                    Button {
                        id: deleteButton
                        text: "Usuń"
                        width: 60
                        onClicked: {
                            finder.removeStationFromHistory(index)
                        }
                    }
                }
            }

            Button {
                id: clearHistoryButton
                text: "Wyczyść historię"
                width: parent.width
                onClicked: {
                    finder.clearHistory()
                }
            }
        }
    }

    /**
     * @brief Dialog informujący o błędzie sieciowym.
     *
     * Wyświetla komunikat o braku połączenia z internetem i umożliwia przejście do historii.
     */
    Dialog {
        id: networkErrorDialog
        title: "Błąd połączenia"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        Column {
            spacing: 10
            Text {
                text: "Brak połączenia z internetem/bazą danych.\nCzy chcesz wyświetlić dane historyczne?"
            }
        }

        onAccepted: {
            historyDialog.open()
        }
    }

    /**
     * @brief Dialog do analizy danych historycznych.
     *
     * Umożliwia wybór miejscowości i stacji, a następnie wyświetla wykres danych z ostatnich 24 godzin
     * oraz statystyki (najmniejsza/największa wartość, średnia, trend).
     */
    Dialog {
        id: analysisDialog
        title: "Analiza danych historycznych"
        width: 600
        height: 600 // Zwiększamy wysokość, aby zmieścić statystyki
        anchors.centerIn: parent
        standardButtons: Dialog.Close

        property var selectedStation: null
        property var chartData: []
        property string statsText: "" // Przechowujemy tekst statystyk

        Column {
            anchors.fill: parent
            spacing: 10
            padding: 10

            Row {
                spacing: 10
                TextField {
                    id: analysisCityField
                    placeholderText: "Wpisz miejscowość"
                    width: 200
                }
                Button {
                    text: "Pokaż dane"
                    onClicked: {
                        var stations = finder.getStationsForCity(analysisCityField.text)
                        if (stations.length === 0) {
                            analysisErrorText.text = "Nie znaleziono stacji w danej miejscowości."
                            analysisErrorText.visible = true
                            chartCanvas.visible = false
                        } else if (stations.length === 1) {
                            // Jeśli jest tylko jedna stacja, od razu pokazujemy dane
                            analysisDialog.selectedStation = stations[0]
                            updateChartData()
                        } else {
                            // Sprawdzamy, czy stacje są różne na podstawie id
                            var uniqueStationIds = []
                            for (var i = 0; i < stations.length; i++) {
                                var stationId = stations[i].id
                                if (uniqueStationIds.indexOf(stationId) === -1) {
                                    uniqueStationIds.push(stationId)
                                }
                            }

                            if (uniqueStationIds.length === 1) {
                                // Jeśli wszystkie stacje mają to samo id, bierzemy najnowszą
                                analysisDialog.selectedStation = stations[0] // Najnowsza jest pierwsza po sortowaniu w getStationsForCity
                                updateChartData()
                            } else {
                                // Jeśli mamy różne stacje, pokazujemy dialog wyboru
                                stationSelectionDialog.stations = stations
                                stationSelectionDialog.open()
                            }
                        }
                    }
                }
            }

            Text {
                id: analysisErrorText
                visible: false
                color: "red"
                wrapMode: Text.WordWrap
                width: parent.width - 20
            }

            Item {
                id: chartContainer
                width: parent.width - 20
                height: 300 // Zmniejszamy wysokość wykresu, aby zrobić miejsce na statystyki

                /**
                 * @brief Wykres danych historycznych.
                 *
                 * Rysuje wykres liniowy dla danych z ostatnich 24 godzin, z osiami, etykietami
                 * i legendą. Obsługuje interakcję myszą, wyświetlając tooltip z wartościami.
                 */
                Canvas {
                    id: chartCanvas
                    anchors.fill: parent
                    visible: false

                    property var dataSeries: []
                    property real maxValue: 100
                    property real minValue: 0
                    property var dates: [] // Lista godzin co 3 godziny

                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)

                        // Rysujemy osie
                        ctx.strokeStyle = "black"
                        ctx.lineWidth = 1
                        ctx.beginPath()
                        ctx.moveTo(40, 10) // Oś Y (od góry do dołu)
                        ctx.lineTo(40, height - 60) // Zostawiamy miejsce na legendę na dole
                        ctx.moveTo(40, height - 60) // Oś X (od lewej do prawej)
                        ctx.lineTo(width - 10, height - 60)
                        ctx.stroke()

                        // Rysujemy etykiety osi Y
                        var yRange = maxValue - minValue
                        var yStep = (height - 70) / 5
                        for (var i = 0; i <= 5; i++) {
                            var y = height - 60 - i * yStep
                            var value = minValue + (i * yRange / 5)
                            ctx.fillText(value.toFixed(0), 5, y)
                            ctx.beginPath()
                            ctx.moveTo(35, y)
                            ctx.lineTo(40, y)
                            ctx.stroke()
                        }

                        // Rysujemy etykiety osi X (godziny co 3 godziny)
                        var xStep = (width - 50) / (dates.length > 1 ? dates.length - 1 : 1)
                        for (var i = 0; i < dates.length; i++) {
                            var x = 40 + i * xStep
                            var hour = Qt.formatDateTime(dates[i], "hh:mm")
                            ctx.fillText(hour, x - 15, height - 40)
                            ctx.beginPath()
                            ctx.moveTo(x, height - 65)
                            ctx.lineTo(x, height - 60)
                            ctx.stroke()
                        }

                        // Rysujemy serie danych
                        var colorMap = {
                            "PM10": "black",
                            "PM2.5": "gray",
                            "SO2": "yellow",
                            "NO2": "blue",
                            "O3": "green",
                            "CO": "purple",
                            "C6H6": "orange"
                        }

                        var startTime = dates[0].getTime()
                        var endTime = dates[dates.length - 1].getTime()
                        var timeRange = endTime - startTime

                        for (var s = 0; s < dataSeries.length; s++) {
                            var series = dataSeries[s]
                            var param = series.param
                            var values = series.values
                            var timestamps = series.timestamps
                            ctx.strokeStyle = colorMap[param] || "red"
                            ctx.lineWidth = 2
                            ctx.beginPath()

                            for (var i = 0; i < values.length; i++) {
                                var timestamp = timestamps[i].getTime()
                                var xFraction = (timestamp - startTime) / timeRange
                                var x = 40 + xFraction * (width - 50)
                                var y = height - 60 - ((values[i] - minValue) / (maxValue - minValue)) * (height - 70)
                                if (i === 0) {
                                    ctx.moveTo(x, y)
                                } else {
                                    ctx.lineTo(x, y)
                                }
                            }
                            ctx.stroke()
                        }

                        // Rysujemy legendę na dole
                        var legendX = 40
                        var legendY = height - 20
                        for (var s = 0; s < dataSeries.length; s++) {
                            var series = dataSeries[s]
                            var param = series.param
                            ctx.fillStyle = colorMap[param] || "red"
                            ctx.fillRect(legendX, legendY - 10, 20, 10)
                            ctx.fillStyle = "black"
                            ctx.fillText(param, legendX + 25, legendY)
                            legendX += 80
                            if (legendX > width - 100) {
                                legendX = 40
                                legendY += 20
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onPositionChanged: (mouse) => {
                            var startTime = dates[0].getTime()
                            var endTime = dates[dates.length - 1].getTime()
                            var timeRange = endTime - startTime
                            var closestPoint = null
                            var minDistance = Number.MAX_VALUE
                            var closestSeries = null

                            for (var s = 0; s < chartCanvas.dataSeries.length; s++) {
                                var series = chartCanvas.dataSeries[s]
                                var values = series.values
                                var timestamps = series.timestamps
                                for (var i = 0; i < values.length; i++) {
                                    var timestamp = timestamps[i].getTime()
                                    var xFraction = (timestamp - startTime) / timeRange
                                    var x = 40 + xFraction * (chartCanvas.width - 50)
                                    var y = chartCanvas.height - 60 - ((values[i] - chartCanvas.minValue) / (chartCanvas.maxValue - chartCanvas.minValue)) * (chartCanvas.height - 70)
                                    var distance = Math.sqrt(Math.pow(mouse.x - x, 2) + Math.pow(mouse.y - y, 2))
                                    if (distance < minDistance) {
                                        minDistance = distance
                                        closestPoint = { x: x, y: y, value: values[i] }
                                        closestSeries = series
                                    }
                                }
                            }

                            if (closestPoint && minDistance < 20) {
                                chartTooltip.text = closestSeries.param + ": " + closestPoint.value.toFixed(2)
                                chartTooltip.x = mouse.x + 10
                                chartTooltip.y = mouse.y + 10
                                chartTooltip.visible = true
                            } else {
                                chartTooltip.visible = false
                            }
                        }
                        onExited: {
                            chartTooltip.visible = false
                        }
                    }

                    ToolTip {
                        id: chartTooltip
                        visible: false
                        background: Rectangle {
                            color: "white"
                            border.color: "black"
                        }
                    }
                }
            }

            // Dodajemy tekst ze statystykami pod wykresem
            ScrollView {
                width: parent.width - 20
                height: 200 // Wysokość dla statystyk
                clip: true

                Text {
                    id: statsTextDisplay
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: analysisDialog.statsText
                }
            }
        }
    }

    /**
     * @brief Dialog wyboru stacji do analizy danych.
     *
     * Wyświetla listę stacji dla danej miejscowości, umożliwiając wybór jednej z nich
     * do analizy danych historycznych.
     */
    Dialog {
        id: stationSelectionDialog
        title: "Wybierz stację"
        width: 300
        height: 200
        anchors.centerIn: parent
        standardButtons: Dialog.Close

        property var stations: []

        Column {
            anchors.fill: parent
            spacing: 5

            ListView {
                width: parent.width
                height: parent.height - 20
                model: stationSelectionDialog.stations
                delegate: ItemDelegate {
                    width: parent.width
                    text: modelData.name
                    onClicked: {
                        analysisDialog.selectedStation = modelData
                        stationSelectionDialog.close()
                        updateChartData()
                    }
                }
            }
        }
    }

    /**
     * @brief Funkcja aktualizująca dane wykresu w dialogu analizy.
     *
     * Przygotowuje dane z ostatnich 24 godzin dla wybranej stacji, oblicza statystyki
     * (najmniejsza/największa wartość, średnia, trend) i aktualizuje wykres oraz tekst statystyk.
     */
    function updateChartData() {
        chartCanvas.dataSeries = []
        chartCanvas.dates = []
        analysisErrorText.visible = false
        chartCanvas.visible = true
        analysisDialog.statsText = "" // Resetujemy tekst statystyk

        if (!analysisDialog.selectedStation || !analysisDialog.selectedStation.sensors) {
            analysisErrorText.text = "Nie ma zapisanej historii pomiarów dla tej stacji."
            analysisErrorText.visible = true
            chartCanvas.visible = false
            return
        }

        var sensors = analysisDialog.selectedStation.sensors
        var maxValue = 0
        var minValue = Number.MAX_VALUE
        var allDates = []

        // Znajdujemy najnowszy pomiar
        var latestDate = null
        for (var i in sensors) {
            var sensor = sensors[i]
            if (sensor.measurements && sensor.measurements.length > 0) {
                var measurements = sensor.measurements
                for (var j = 0; j < measurements.length; j++) {
                    var date = new Date(measurements[j].date)
                    if (!latestDate || date > latestDate) {
                        latestDate = date
                    }
                }
            }
        }

        if (!latestDate) {
            analysisErrorText.text = "Nie ma zapisanej historii pomiarów dla tej stacji."
            analysisErrorText.visible = true
            chartCanvas.visible = false
            return
        }

        // Obliczamy granicę 24 godzin wstecz od najnowszego pomiaru
        var startDate = new Date(latestDate.getTime() - 24 * 60 * 60 * 1000)

        // Filtrujemy pomiary, zbieramy dane i obliczamy statystyki
        var stats = {} // Obiekt do przechowywania statystyk dla każdego parametru
        for (var i in sensors) {
            var sensor = sensors[i]
            if (sensor.measurements && sensor.measurements.length > 0) {
                var series = {
                    param: sensor.paramFormula,
                    values: [],
                    timestamps: []
                }
                var measurements = sensor.measurements
                var paramStats = {
                    min: Number.MAX_VALUE,
                    minDate: null,
                    max: Number.MIN_VALUE,
                    maxDate: null,
                    sum: 0,
                    count: 0,
                    firstValue: null,
                    lastValue: null
                }

                for (var j = 0; j < measurements.length; j++) {
                    var date = new Date(measurements[j].date)
                    if (date >= startDate && date <= latestDate) {
                        var value = measurements[j].value
                        series.values.push(value)
                        series.timestamps.push(date)

                        // Obliczamy statystyki
                        if (value < paramStats.min) {
                            paramStats.min = value
                            paramStats.minDate = date
                        }
                        if (value > paramStats.max) {
                            paramStats.max = value
                            paramStats.maxDate = date
                        }
                        paramStats.sum += value
                        paramStats.count++

                        // Zapisujemy pierwszą i ostatnią wartość do obliczenia trendu
                        if (paramStats.firstValue === null || date < new Date(series.timestamps[0])) {
                            paramStats.firstValue = value
                        }
                        if (paramStats.lastValue === null || date > new Date(series.timestamps[series.timestamps.length - 1])) {
                            paramStats.lastValue = value
                        }

                        maxValue = Math.max(maxValue, value)
                        minValue = Math.min(minValue, value)
                    }
                }

                if (series.values.length > 0) {
                    chartCanvas.dataSeries.push(series)
                    // Obliczamy średnią i trend
                    paramStats.average = paramStats.sum / paramStats.count
                    var valueDiff = paramStats.lastValue - paramStats.firstValue
                    if (valueDiff > 0.1 * paramStats.average) {
                        paramStats.trend = "rosnący"
                    } else if (valueDiff < -0.1 * paramStats.average) {
                        paramStats.trend = "malejący"
                    } else {
                        paramStats.trend = "stabilny"
                    }
                    stats[sensor.paramFormula] = paramStats
                }
            }
        }

        if (chartCanvas.dataSeries.length === 0) {
            analysisErrorText.text = "Brak danych z ostatnich 24 godzin."
            analysisErrorText.visible = true
            chartCanvas.visible = false
            return
        }

        // Przygotowujemy etykiety godzinowe co 3 godziny
        var hours = []
        for (var h = 0; h <= 24; h += 3) {
            var hourDate = new Date(startDate.getTime() + h * 60 * 60 * 1000)
            hours.push(hourDate)
        }
        chartCanvas.dates = hours

        chartCanvas.maxValue = maxValue + 10
        chartCanvas.minValue = Math.max(0, minValue - 10)
        chartCanvas.requestPaint()

        // Budujemy tekst statystyk
        var statsText = ""
        for (var param in stats) {
            var paramStats = stats[param]
            statsText += param + ":\n"
            statsText += "  Najmniejsza wartość: " + paramStats.min.toFixed(2) + " (data: " + Qt.formatDateTime(paramStats.minDate, "yyyy-MM-dd hh:mm") + ")\n"
            statsText += "  Największa wartość: " + paramStats.max.toFixed(2) + " (data: " + Qt.formatDateTime(paramStats.maxDate, "yyyy-MM-dd hh:mm") + ")\n"
            statsText += "  Średnia: " + paramStats.average.toFixed(2) + "\n"
            statsText += "  Trend: " + paramStats.trend + "\n\n"
        }
        analysisDialog.statsText = statsText
    }
}
