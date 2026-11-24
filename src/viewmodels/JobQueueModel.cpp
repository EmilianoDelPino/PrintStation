#include "JobQueueModel.h"
#include <QMetaEnum>

using JobStatus::Status;

JobQueueModel::JobQueueModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int JobQueueModel::rowCount(const QModelIndex &parent) const
{
    // En una lista los items no tienen hijos: si nos preguntan por los hijos de un
    // item válido, la respuesta es 0. Sólo la raíz (índice inválido) tiene filas.
    if (parent.isValid())
        return 0;
    return int(m_jobs.size());
}

QVariant JobQueueModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid))
        return {};

    const PrintJob &job = m_jobs.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case JobNameRole:
        return job.name;
    case JobIdRole:
        return job.id;
    case PagesRole:
        return job.pages;
    case PrintedPagesRole:
        return job.printedPages;
    case ProgressRole:
        return job.pages > 0 ? double(job.printedPages) / job.pages : 0.0;
    case StatusTextRole:
        return QString::fromLatin1(QMetaEnum::fromType<Status>().valueToKey(int(job.status)));
    case IsActiveRole:
        return job.isActive();
    case CanCancelRole:
        return !job.isFinished();
    default:
        return {};
    }
}

QHash<int, QByteArray> JobQueueModel::roleNames() const
{
    // Estos nombres son los que usa QML: en el delegate, "required property int jobId"
    // se llena con el valor del rol JobIdRole de esa fila.
    return {
        { JobIdRole, "jobId" },
        { JobNameRole, "jobName" },
        { PagesRole, "pages" },
        { PrintedPagesRole, "printedPages" },
        { ProgressRole, "progress" },
        { StatusTextRole, "statusText" },
        { IsActiveRole, "isActive" },
        { CanCancelRole, "canCancel" },
    };
}

int JobQueueModel::addJob(const QString &name, int pages)
{
    const int row = int(m_jobs.size());

    // Contrato de los modelos de Qt: begin* ANTES de tocar los datos, end* DESPUÉS.
    // Así la vista sabe exactamente qué filas cambiaron y no se redibuja entera.
    beginInsertRows(QModelIndex(), row, row);
    PrintJob job;
    job.id = m_nextId++;
    job.name = name;
    job.pages = pages;
    m_jobs.append(job);
    endInsertRows();

    emit countChanged();
    return job.id;
}

void JobQueueModel::setStatus(int jobId, Status status)
{
    const int row = rowOf(jobId);
    if (row < 0 || m_jobs[row].status == status)
        return;

    m_jobs[row].status = status;

    // Para cambios en una fila existente no hay begin/end: se avisa con dataChanged,
    // indicando qué roles cambiaron (sólo esos bindings se reevalúan en QML).
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, { StatusTextRole, IsActiveRole, CanCancelRole });
}

void JobQueueModel::setPrintedPages(int jobId, int printedPages)
{
    const int row = rowOf(jobId);
    if (row < 0 || m_jobs[row].printedPages == printedPages)
        return;

    m_jobs[row].printedPages = printedPages;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, { PrintedPagesRole, ProgressRole });
}

int JobQueueModel::removeFinished()
{
    int removed = 0;
    // De atrás para adelante: borrar una fila no corre los índices de las que faltan revisar.
    for (int row = int(m_jobs.size()) - 1; row >= 0; --row) {
        if (!m_jobs.at(row).isFinished())
            continue;
        beginRemoveRows(QModelIndex(), row, row);
        m_jobs.removeAt(row);
        endRemoveRows();
        ++removed;
    }
    if (removed > 0)
        emit countChanged();
    return removed;
}

std::optional<PrintJob> JobQueueModel::job(int jobId) const
{
    const int row = rowOf(jobId);
    if (row < 0)
        return std::nullopt;
    return m_jobs.at(row);
}

std::optional<PrintJob> JobQueueModel::nextQueued() const
{
    // FIFO: el primero en la lista que siga esperando.
    for (const PrintJob &job : m_jobs) {
        if (job.status == Status::Queued)
            return job;
    }
    return std::nullopt;
}

int JobQueueModel::rowOf(int jobId) const
{
    // Búsqueda lineal: una cola de impresión tiene decenas de trabajos, no millones.
    // Si creciera, se agregaría un QHash<id, row> (y habría que mantenerlo al borrar).
    for (int row = 0; row < m_jobs.size(); ++row) {
        if (m_jobs.at(row).id == jobId)
            return row;
    }
    return -1;
}
