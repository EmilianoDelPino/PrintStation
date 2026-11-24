#pragma once
#include <QAbstractListModel>
#include <QList>
#include <optional>
#include <QtQml/qqmlregistration.h>
#include "PrintJob.h"

/**
 * @brief Cola de impresión expuesta como modelo de lista, lista para un ListView de QML.
 *
 * - Vive en el hilo de UI (igual que las vistas que lo leen). El worker NUNCA lo toca:
 *   los cambios llegan como señales queued al PrinterViewModel, que es quien lo actualiza.
 * - Es "tonto": guarda datos y avisa cambios con precisión (begin/end*Rows, dataChanged).
 *   La lógica de qué imprimir y cuándo está en PrinterViewModel.
 * - QML sólo lo lee. Las acciones (agregar/cancelar) pasan por el ViewModel.
 *
 * QML_UNCREATABLE: QML conoce el tipo (chequeo de tipos, autocompletado) pero no puede
 * instanciarlo; la única instancia la crea PrinterViewModel.
 */
class JobQueueModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("JobQueueModel is owned by PrinterViewModel")

    /// Cantidad de trabajos en la cola. Bindeable desde QML (rowCount() no lo es).
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    /**
     * @brief Roles del modelo: cada uno es una "columna con nombre" que el delegate lee.
     *
     * Arrancan en Qt::UserRole para no pisar los roles estándar (DisplayRole, etc.).
     * El nombre que ve QML está entre paréntesis (ver roleNames()).
     */
    enum Role {
        JobIdRole = Qt::UserRole + 1, ///< int (jobId): identificador del trabajo.
        JobNameRole,                  ///< QString (jobName): nombre del documento.
        PagesRole,                    ///< int (pages): páginas totales.
        PrintedPagesRole,             ///< int (printedPages): páginas impresas.
        ProgressRole,                 ///< double (progress): 0.0..1.0, derivado.
        StatusTextRole,               ///< QString (statusText): nombre del JobStatus::Status.
        IsActiveRole,                 ///< bool (isActive): imprimiéndose o pausado.
        CanCancelRole,                ///< bool (canCancel): no está en un estado final.
    };
    Q_ENUM(Role)

    /**
     * @brief Crea una cola vacía.
     * @param parent Dueño del modelo (normalmente el PrinterViewModel).
     */
    explicit JobQueueModel(QObject *parent = nullptr);

    // --- Interfaz de QAbstractListModel (lo que consume la vista) ---

    /**
     * @brief Cantidad de filas bajo @p parent.
     * @param parent Índice padre. En una lista sólo la raíz (índice inválido) tiene hijos.
     * @return Cantidad de trabajos si @p parent es la raíz; 0 en otro caso.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /**
     * @brief Valor de un rol para una fila.
     * @param index Fila a consultar. Debe ser un índice válido de este modelo.
     * @param role  Uno de los valores de Role (o Qt::DisplayRole, que devuelve el nombre).
     * @return El valor pedido, o un QVariant inválido si el índice o el rol no existen.
     */
    QVariant data(const QModelIndex &index, int role) const override;

    /**
     * @brief Nombres con los que QML accede a cada rol.
     * @return Mapa rol → nombre. Ej.: JobIdRole → "jobId", que el delegate recibe
     *         declarando `required property int jobId`.
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief Getter de la propiedad count.
     * @return Cantidad de trabajos en la cola (incluye los terminados).
     */
    int count() const { return int(m_jobs.size()); }

    // --- API de C++ para el ViewModel ---

    /**
     * @brief Agrega un trabajo al final de la cola, en estado Queued.
     * @param name  Nombre del documento.
     * @param pages Cantidad de páginas.
     * @return El id único asignado al trabajo.
     */
    int addJob(const QString &name, int pages);

    /**
     * @brief Cambia el estado de un trabajo y notifica a la vista.
     * @param jobId  Trabajo a modificar. Si no existe, no hace nada.
     * @param status Estado nuevo. Si es igual al actual, no hace nada.
     */
    void setStatus(int jobId, JobStatus::Status status);

    /**
     * @brief Actualiza las páginas impresas de un trabajo y notifica a la vista.
     * @param jobId        Trabajo a modificar. Si no existe, no hace nada.
     * @param printedPages Páginas impresas hasta ahora.
     */
    void setPrintedPages(int jobId, int printedPages);

    /**
     * @brief Saca de la cola todos los trabajos en estado final (Completed o Cancelled).
     * @return Cantidad de trabajos eliminados.
     */
    int removeFinished();

    /**
     * @brief Busca un trabajo por id.
     *
     * Devuelve una copia y no un puntero: un puntero a un elemento de la lista quedaría
     * colgando apenas la lista se reacomode al insertar o borrar.
     *
     * @param jobId Trabajo buscado.
     * @return Copia del trabajo, o std::nullopt si no existe.
     */
    std::optional<PrintJob> job(int jobId) const;

    /**
     * @brief Próximo trabajo a imprimir según orden FIFO.
     * @return Copia del primer trabajo en estado Queued, o std::nullopt si no hay ninguno.
     */
    std::optional<PrintJob> nextQueued() const;

signals:
    /// Se emite cuando cambia la cantidad de filas (al agregar o eliminar trabajos).
    void countChanged();

private:
    /**
     * @brief Fila en la que está un trabajo (búsqueda lineal).
     * @param jobId Trabajo buscado.
     * @return Índice de fila, o -1 si no existe.
     */
    int rowOf(int jobId) const;

    QList<PrintJob> m_jobs; ///< Los trabajos, en orden de llegada (= orden de las filas).
    int m_nextId = 1;       ///< Próximo id a asignar; crece siempre, nunca se reutiliza.
};
