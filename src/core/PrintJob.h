#pragma once
#include <QObject>
#include <QString>

/**
 * @brief Estado de un trabajo dentro de la cola de impresión.
 *
 * Igual que PrinterState: Q_NAMESPACE + Q_ENUM_NS permiten obtener el nombre legible
 * del valor via QMetaEnum (para mostrarlo en la UI).
 */
namespace JobStatus {
Q_NAMESPACE

/**
 * @brief Ciclo de vida de un trabajo: Queued → Printing ⇄ Paused → Completed | Cancelled.
 */
enum class Status {
    Queued,    ///< Esperando su turno en la cola.
    Printing,  ///< Es el trabajo activo y se está imprimiendo.
    Paused,    ///< Es el trabajo activo, pero la impresora está en pausa.
    Completed, ///< Se imprimieron todas sus páginas. Estado final.
    Cancelled, ///< Se canceló (en espera o a mitad de impresión). Estado final.
};
Q_ENUM_NS(Status)
}

/**
 * @brief Un trabajo de impresión: dato puro (value type).
 *
 * No es un QObject: se copia libremente, no tiene identidad ni parent y no emite
 * señales. El JobQueueModel guarda una lista de estos; nadie fuera del modelo
 * mantiene punteros a ellos.
 */
struct PrintJob
{
    int id = 0;            ///< Identificador único, asignado por JobQueueModel::addJob().
    QString name;          ///< Nombre del documento (ej. "Document_3.pdf").
    int pages = 0;         ///< Cantidad total de páginas a imprimir.
    int printedPages = 0;  ///< Páginas ya impresas (0..pages).
    JobStatus::Status status = JobStatus::Status::Queued; ///< Estado actual en la cola.

    /**
     * @brief Indica si el trabajo está ocupando la impresora.
     * @return true si está imprimiéndose o pausado a mitad de camino.
     */
    bool isActive() const
    {
        return status == JobStatus::Status::Printing || status == JobStatus::Status::Paused;
    }

    /**
     * @brief Indica si el trabajo llegó a un estado final y ya no va a cambiar.
     * @return true si está Completed o Cancelled.
     */
    bool isFinished() const
    {
        return status == JobStatus::Status::Completed || status == JobStatus::Status::Cancelled;
    }
};
