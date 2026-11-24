// Bound: los componentes anidados (Component, delegates) pueden usar ids de este
// archivo (ej. "page") de forma explícita y verificable por el compilador de QML.
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Pantalla de la cola: formulario para encolar + lista de trabajos.
 *
 * Sólo bindings y llamadas al ViewModel; ninguna regla de negocio vive acá.
 * Se apila sobre HomePage; el botón "Back" vuelve con StackView.pop().
 */
Page {
    id: page

    /// ViewModel dueño de la cola (printer.jobs) y de las acciones addJob/cancelJob/clearFinished. Obligatorio.
    required property PrinterViewModel printer

    header: ToolBar {
        RowLayout {
            anchors.fill: parent

            ToolButton {
                text: qsTr("‹ Back")
                onClicked: page.StackView.view.pop()
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Print queue")
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
            }
            ToolButton {
                text: qsTr("Clear finished")
                onClicked: page.printer.clearFinished()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Encolar un trabajo nuevo
        RowLayout {
            Layout.fillWidth: true

            TextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("Document name (optional)")
                onAccepted: addButton.clicked()
            }
            SpinBox {
                id: pagesBox
                from: 1
                to: 99
                value: 5
                editable: true
            }
            Button {
                id: addButton
                text: qsTr("Add")
                onClicked: {
                    page.printer.addJob(nameField.text, pagesBox.value)
                    nameField.clear()
                }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: page.printer.jobs

            // Por cada fila del modelo, QML crea (o recicla) una instancia de esto.
            delegate: JobDelegate {
                width: ListView.view.width
                onCancelClicked: page.printer.cancelJob(jobId)
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("No jobs in queue")
                opacity: 0.6
            }
        }
    }
}
