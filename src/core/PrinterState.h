#pragma once
#include <QObject>

/**
 * @brief Estados posibles de la impresora (el PrintEngine).
 *
 * Q_NAMESPACE + Q_ENUM_NS registran el enum en el sistema de metatipos de Qt, lo que
 * permite:
 *  - pasarlo como argumento de señales entre hilos (las conexiones queued copian los
 *    argumentos y necesitan conocer el tipo);
 *  - obtener el nombre legible de cada valor con QMetaEnum (ej. "Printing").
 */
namespace PrinterState {
Q_NAMESPACE

/**
 * @brief Estado de la impresora.
 */
enum class State {
    Idle,     ///< Sin trabajo asignado; lista para recibir uno.
    Printing, ///< Imprimiendo un trabajo (el timer de páginas corre).
    Paused,   ///< Hay un trabajo a mitad de camino, detenido por el usuario.
    Error,    ///< Reservado para fallas (papel, tinta). Aún no se usa.
};
Q_ENUM_NS(State)
}
