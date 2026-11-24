import QtQuick
import QtQuick.Controls

/**
 * Ventana principal de la aplicación (raíz de la UI).
 *
 * Crea la única instancia de PrinterViewModel y la pasa a las páginas, que se apilan
 * en un StackView (navegación por pila: push para entrar, pop para volver).
 */
ApplicationWindow {
    id: root
    width: 520
    height: 600
    visible: true
    title: qsTr("PrintStation")

    PrinterViewModel { id: vm }

    StackView {
        anchors.fill: parent
        initialItem: HomePage { printer: vm }
    }
}
