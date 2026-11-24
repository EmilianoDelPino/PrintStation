// Bound: los componentes anidados (Component, delegates) pueden usar ids de este
// archivo (ej. "page") de forma explícita y verificable por el compilador de QML.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Pantalla de inicio: estado de la impresora y controles del trabajo activo.
 *
 * Muestra estado, trabajo y progreso; permite encolar un trabajo rápido, pausar,
 * reanudar y cancelar, y navegar a la cola (QueuePage).
 */
Page {
    id: page

    /// ViewModel del que se leen los datos y al que se delegan las acciones. Obligatorio.
    required property PrinterViewModel printer

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: 12
                text: qsTr("PrintStation")
                font.bold: true
            }
            ToolButton {
                // count es una Q_PROPERTY del modelo con NOTIFY: se actualiza sola.
                text: qsTr("Queue (%1)").arg(page.printer.jobs.count)
                onClicked: page.StackView.view.push(queuePageComponent)
            }
        }
    }

    // La pantalla de cola se crea recién cuando se navega a ella (y se destruye al volver).
    Component {
        id: queuePageComponent
        QueuePage { printer: page.printer }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("Status: ") + page.printer.stateText
            font.pixelSize: 22
        }

        Label {
            visible: page.printer.busy
            text: page.printer.jobName + "  —  "
                  + qsTr("page %1 of %2").arg(page.printer.currentPage).arg(page.printer.totalPages)
        }

        ProgressBar {
            Layout.fillWidth: true
            value: page.printer.progress
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                // Encola un documento de prueba; si la impresora está libre arranca ya.
                text: qsTr("Quick print")
                onClicked: page.printer.addJob("", 12)
            }
            Button {
                text: page.printer.paused ? qsTr("Resume") : qsTr("Pause")
                enabled: page.printer.busy
                onClicked: page.printer.paused ? page.printer.resume() : page.printer.pause()
            }
            Button {
                text: qsTr("Cancel")
                enabled: page.printer.busy
                onClicked: page.printer.cancel()
            }
        }
    }
}
