#pragma once
#include <QObject>
#include <QString>
#include <QThread>
#include <QtQml/qqmlregistration.h>
#include "PrinterState.h"
#include "JobQueueModel.h" // completo (no forward-declaration): moc lo necesita para la Q_PROPERTY

class PrintEngine;

/**
 * @brief ViewModel principal (MVVM): lo único que QML ve de la impresora.
 *
 * Vive en el hilo de UI y es dueño de:
 *  - el hilo worker donde corre PrintEngine ("la impresora");
 *  - la cola de trabajos (JobQueueModel), única fuente de verdad de la cola.
 *
 * Hace de despachante: cuando la impresora queda libre, le manda el siguiente trabajo
 * en espera. Nunca toca el estado del engine directamente; le habla con señales
 * (xxxRequested) y reacciona a las que él emite.
 *
 * Desde QML: `PrinterViewModel { id: vm }` (registrado con QML_ELEMENT).
 */
class PrinterViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    /// Nombre del estado de la impresora ("Idle", "Printing", "Paused"...).
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateChanged)
    /// true si la impresora tiene un trabajo (imprimiendo o pausado).
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    /// true si la impresora está en pausa.
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    /// Nombre del trabajo activo; vacío si no hay ninguno.
    Q_PROPERTY(QString jobName READ jobName NOTIFY progressChanged)
    /// Páginas impresas del trabajo activo.
    Q_PROPERTY(int currentPage READ currentPage NOTIFY progressChanged)
    /// Páginas totales del trabajo activo.
    Q_PROPERTY(int totalPages READ totalPages NOTIFY progressChanged)
    /// Progreso del trabajo activo, de 0.0 a 1.0.
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    /// La cola de trabajos. CONSTANT: el puntero no cambia nunca (el contenido sí,
    /// pero eso lo avisa el propio modelo con sus señales).
    Q_PROPERTY(JobQueueModel *jobs READ jobs CONSTANT)

public:
    /**
     * @brief Crea la cola, mueve un PrintEngine nuevo a un hilo worker y lo arranca.
     * @param parent Dueño QObject del ViewModel (en QML, lo maneja el engine de QML).
     */
    explicit PrinterViewModel(QObject *parent = nullptr);

    /**
     * @brief Cierre ordenado: detiene el event loop del worker y espera a que termine.
     *
     * El engine se libera solo, vía deleteLater conectado a QThread::finished.
     */
    ~PrinterViewModel() override;

    /// @return Nombre legible del estado de la impresora (ej. "Printing").
    QString stateText() const;
    /// @return true si la impresora está Printing o Paused.
    bool busy() const;
    /// @return true si la impresora está Paused.
    bool paused() const;
    /// @return Nombre del trabajo activo, o cadena vacía si no hay.
    QString jobName() const { return m_jobName; }
    /// @return Páginas impresas del trabajo activo.
    int currentPage() const { return m_page; }
    /// @return Páginas totales del trabajo activo (0 si no hay).
    int totalPages() const { return m_total; }
    /// @return Progreso del trabajo activo entre 0.0 y 1.0 (0.0 si no hay).
    double progress() const { return m_total > 0 ? double(m_page) / m_total : 0.0; }
    /// @return La cola de trabajos (propiedad del ViewModel; no la libere quien la recibe).
    JobQueueModel *jobs() const { return m_queue; }

    // --- Acciones sobre la cola (invocables desde QML) ---

    /**
     * @brief Encola un trabajo y, si la impresora está libre, lo empieza a imprimir.
     * @param name  Nombre del documento. Si está vacío (o sólo espacios) se genera
     *              uno por defecto: "Document_N.pdf".
     * @param pages Cantidad de páginas; se acota al rango [1, 999].
     */
    Q_INVOKABLE void addJob(const QString &name, int pages);

    /**
     * @brief Cancela un trabajo de la cola.
     *
     * - Si está en espera, pasa a Cancelled inmediatamente.
     * - Si es el activo, se le pide al engine que lo corte y el estado se actualiza
     *   cuando el engine lo confirme (jobFinished). Si justo terminó, queda Completed.
     * - Si ya terminó o no existe, no hace nada.
     *
     * @param jobId Trabajo a cancelar.
     */
    Q_INVOKABLE void cancelJob(int jobId);

    /**
     * @brief Saca de la cola los trabajos terminados (Completed y Cancelled).
     */
    Q_INVOKABLE void clearFinished();

    // --- Acciones sobre el trabajo activo (invocables desde QML) ---

    /// @brief Pausa la impresión del trabajo activo. Sin efecto si no se está imprimiendo.
    Q_INVOKABLE void pause();
    /// @brief Reanuda el trabajo pausado. Sin efecto si no está en pausa.
    Q_INVOKABLE void resume();
    /// @brief Cancela el trabajo activo (equivale a cancelJob() con su id). Sin efecto si no hay.
    Q_INVOKABLE void cancel();

signals:
    /// Cambió el estado de la impresora (afecta stateText, busy y paused).
    void stateChanged();
    /// Cambió el trabajo activo o su avance (afecta jobName, currentPage, totalPages, progress).
    void progressChanged();

    // Señales "hacia" el worker: al estar el engine en otro hilo, Qt las entrega como
    // QueuedConnection (se ejecutan en el hilo del worker). Uso interno: QML no debería usarlas.

    /**
     * @brief Pide al engine que imprima un trabajo.
     * @param jobId Id del trabajo.
     * @param name  Nombre del documento.
     * @param pages Cantidad de páginas.
     */
    void startRequested(int jobId, const QString &name, int pages);
    /// Pide al engine que pause el trabajo activo.
    void pauseRequested();
    /// Pide al engine que reanude el trabajo pausado.
    void resumeRequested();
    /// Pide al engine que cancele el trabajo activo.
    void cancelRequested();

private:
    // Reacciones a lo que reporta el worker (corren en el hilo de UI).

    /**
     * @brief Actualiza el estado local y lo refleja en la fila del trabajo activo
     *        (Printing ⇄ Paused).
     * @param state Estado reportado por el engine.
     */
    void onEngineStateChanged(PrinterState::State state);

    /**
     * @brief Registra el avance del trabajo activo en el ViewModel y en la cola.
     * @param jobId Trabajo reportado; se ignora si no es el activo.
     * @param page  Páginas impresas.
     * @param total Páginas totales.
     */
    void onEngineProgress(int jobId, int page, int total);

    /**
     * @brief Marca el trabajo como Completed/Cancelled, libera la impresora y despacha
     *        el siguiente.
     * @param jobId     Trabajo que terminó.
     * @param completed true si se imprimió entero; false si se canceló.
     */
    void onEngineJobFinished(int jobId, bool completed);

    /**
     * @brief Si la impresora está libre y hay trabajos en espera, manda el primero (FIFO)
     *        al engine. Si no, no hace nada.
     */
    void dispatchNext();

    QThread m_thread;                 ///< Hilo worker donde vive m_engine.
    PrintEngine *m_engine = nullptr;  ///< Sin parent; se libera con deleteLater al terminar el hilo.
    JobQueueModel *m_queue = nullptr; ///< Hijo de this; vive en el hilo de UI.

    PrinterState::State m_state = PrinterState::State::Idle; ///< Último estado reportado por el engine.
    int m_activeJobId = -1; ///< Trabajo asignado a la impresora (-1 = ninguno).
    QString m_jobName;      ///< Nombre del trabajo activo.
    int m_page = 0;         ///< Páginas impresas del trabajo activo.
    int m_total = 0;        ///< Páginas totales del trabajo activo.
    int m_docCounter = 0;   ///< Contador para nombres por defecto ("Document_N.pdf").
};
