import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

/**
 * Una fila de la cola: nombre, estado, progreso y botón de cancelar.
 *
 * Cada "required property" se llena automáticamente con el rol del modelo que tiene
 * el MISMO nombre (ver JobQueueModel::roleNames()).
 * El delegate no conoce al ViewModel: sólo muestra datos y emite cancelClicked().
 * Quien lo usa decide qué hacer con eso ("data down, events up").
 */
Frame {
    id: root

    /// Id del trabajo (rol "jobId"). Lo usa quien recibe cancelClicked() para saber cuál cancelar.
    required property int jobId
    /// Nombre del documento (rol "jobName").
    required property string jobName
    /// Páginas totales (rol "pages").
    required property int pages
    /// Páginas ya impresas (rol "printedPages").
    required property int printedPages
    /// Progreso 0.0..1.0 (rol "progress").
    required property double progress
    /// Estado legible: "Queued", "Printing", "Paused", "Completed", "Cancelled" (rol "statusText").
    required property string statusText
    /// true si es el trabajo que ocupa la impresora; se resalta en negrita (rol "isActive").
    required property bool isActive
    /// true si todavía se puede cancelar; habilita el botón (rol "canCancel").
    required property bool canCancel

    /// El usuario tocó "Cancel" en esta fila. No lleva parámetros: el receptor usa jobId.
    signal cancelClicked()

    ColumnLayout {
        anchors.fill: parent
        spacing: 4

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: root.jobName
                font.bold: root.isActive
                elide: Text.ElideMiddle
            }
            Label {
                text: root.statusText
                opacity: 0.7
            }
            Button {
                text: qsTr("Cancel")
                flat: true
                enabled: root.canCancel
                onClicked: root.cancelClicked()
            }
        }

        ProgressBar {
            Layout.fillWidth: true
            value: root.progress
        }

        Label {
            text: qsTr("%1 / %2 pages").arg(root.printedPages).arg(root.pages)
            font.pixelSize: 12
            opacity: 0.7
        }
    }
}
