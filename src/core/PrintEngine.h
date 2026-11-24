#pragma once
#include <QObject>
#include <QString>
#include <QTimer>
#include "PrinterState.h"

/**
 * @brief Motor de impresión simulado: "la impresora física".
 *
 * Vive en un hilo propio (worker) y NO conoce QML ni la cola de trabajos: imprime un
 * trabajo por vez y reporta lo que pasa. Se comunica con el resto exclusivamente por
 * señales/slots (queued, porque emisor y receptor están en hilos distintos).
 *
 * Diseño: en vez de bloquear el hilo con sleep(), usa un QTimer dentro del worker
 * (cada tick = una página). Así el event loop del worker sigue atendiendo pause() y
 * cancel() mientras "imprime", sin necesidad de mutex ni flags atómicos.
 *
 * Uso típico (ver PrinterViewModel):
 * @code
 *   auto *engine = new PrintEngine;      // sin parent
 *   engine->moveToThread(&thread);
 *   connect(&thread, &QThread::finished, engine, &QObject::deleteLater);
 *   thread.start();
 * @endcode
 */
class PrintEngine : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Crea el engine en estado Idle, con su timer de páginas configurado.
     * @param parent Debe quedar en nullptr si el engine se va a mover a otro hilo:
     *               un objeto con parent no puede moverse con moveToThread().
     */
    explicit PrintEngine(QObject *parent = nullptr);

public slots:
    /**
     * @brief Empieza a imprimir un trabajo.
     *
     * Si ya hay un trabajo en curso (Printing o Paused), el pedido se ignora y se
     * loguea un warning: la cola y la decisión de qué imprimir después no viven acá.
     *
     * Efectos: emite progressChanged(jobId, 0, pages) y stateChanged(Printing); luego
     * un progressChanged por página y, al final, jobFinished(jobId, true).
     *
     * @param jobId Identificador del trabajo; se repite en todas las señales que lo
     *              involucran para que el receptor sepa a qué trabajo corresponden.
     * @param name  Nombre del documento (sólo para logs).
     * @param pages Cantidad de páginas. Valores < 1 se tratan como 1.
     */
    void startJob(int jobId, const QString &name, int pages);

    /**
     * @brief Detiene la impresión del trabajo activo sin perder el progreso.
     *
     * Sólo tiene efecto en estado Printing; emite stateChanged(Paused).
     */
    void pause();

    /**
     * @brief Continúa un trabajo pausado desde la página en la que quedó.
     *
     * Sólo tiene efecto en estado Paused; emite stateChanged(Printing).
     */
    void resume();

    /**
     * @brief Aborta el trabajo activo.
     *
     * Sólo tiene efecto en Printing o Paused; emite stateChanged(Idle) y
     * jobFinished(jobId, false). Si el trabajo ya había terminado, no hace nada.
     */
    void cancel();

signals:
    /**
     * @brief El estado de la impresora cambió.
     * @param state Nuevo estado.
     */
    void stateChanged(PrinterState::State state);

    /**
     * @brief Se imprimió una página más (o empezó un trabajo, con page = 0).
     * @param jobId Trabajo al que corresponde el avance.
     * @param page  Páginas impresas hasta ahora.
     * @param total Páginas totales del trabajo.
     */
    void progressChanged(int jobId, int page, int total);

    /**
     * @brief El trabajo activo terminó y la impresora quedó libre (Idle).
     * @param jobId     Trabajo que terminó.
     * @param completed true si se imprimieron todas las páginas; false si se canceló.
     */
    void jobFinished(int jobId, bool completed);

private:
    /**
     * @brief Callback del timer: imprime una página y, si era la última, cierra el trabajo.
     */
    void onTick();

    /**
     * @brief Cambia el estado y emite stateChanged() sólo si realmente cambió.
     * @param s Estado nuevo.
     */
    void setState(PrinterState::State s);

    QTimer *m_timer = nullptr;                          ///< Un tick = una página. Hijo del engine => vive en el worker.
    PrinterState::State m_state = PrinterState::State::Idle; ///< Estado actual.
    int m_jobId = -1;                                   ///< Trabajo activo (-1 = ninguno).
    QString m_jobName;                                  ///< Nombre del trabajo activo (logs).
    int m_page = 0;                                     ///< Páginas impresas del trabajo activo.
    int m_total = 0;                                    ///< Páginas totales del trabajo activo.
};
