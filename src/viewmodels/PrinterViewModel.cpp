#include "PrinterViewModel.h"
#include "PrintEngine.h"
#include <QDebug>
#include <QMetaEnum>

using PrinterState::State;
using JobStatus::Status;

PrinterViewModel::PrinterViewModel(QObject *parent)
    : QObject(parent)
    , m_engine(new PrintEngine)          // sin parent: se va a mover a otro hilo
    , m_queue(new JobQueueModel(this))   // con parent: vive y muere con el ViewModel, en el hilo de UI
{
    qDebug() << "[PrinterViewModel] hilo de UI:" << QThread::currentThread();

    // 1) mover el engine al hilo worker  2) liberar el engine cuando el hilo termine
    m_engine->moveToThread(&m_thread);
    connect(&m_thread, &QThread::finished, m_engine, &QObject::deleteLater);

    // UI -> worker
    connect(this, &PrinterViewModel::startRequested, m_engine, &PrintEngine::startJob);
    connect(this, &PrinterViewModel::pauseRequested, m_engine, &PrintEngine::pause);
    connect(this, &PrinterViewModel::resumeRequested, m_engine, &PrintEngine::resume);
    connect(this, &PrinterViewModel::cancelRequested, m_engine, &PrintEngine::cancel);

    // worker -> UI (el receptor 'this' vive en el hilo de UI => queued)
    connect(m_engine, &PrintEngine::stateChanged, this, &PrinterViewModel::onEngineStateChanged);
    connect(m_engine, &PrintEngine::progressChanged, this, &PrinterViewModel::onEngineProgress);
    connect(m_engine, &PrintEngine::jobFinished, this, &PrinterViewModel::onEngineJobFinished);

    m_thread.start();
}

PrinterViewModel::~PrinterViewModel()
{
    // Cierre ordenado: sale del event loop del worker y espera a que termine.
    m_thread.quit();
    m_thread.wait();
}

QString PrinterViewModel::stateText() const
{
    return QString::fromLatin1(QMetaEnum::fromType<State>().valueToKey(int(m_state)));
}

bool PrinterViewModel::busy() const
{
    return m_state == State::Printing || m_state == State::Paused;
}

bool PrinterViewModel::paused() const { return m_state == State::Paused; }

// ---------------------------------------------------------------------------
// Acciones de la UI
// ---------------------------------------------------------------------------

void PrinterViewModel::addJob(const QString &name, int pages)
{
    ++m_docCounter;
    const QString jobName = name.trimmed().isEmpty()
        ? QStringLiteral("Document_%1.pdf").arg(m_docCounter)
        : name.trimmed();

    m_queue->addJob(jobName, qBound(1, pages, 999));
    dispatchNext(); // si la impresora estaba libre, arranca ya
}

void PrinterViewModel::cancelJob(int jobId)
{
    if (jobId == m_activeJobId) {
        // El trabajo está en la impresora: le pedimos al engine que lo corte.
        // NO lo marcamos como cancelado acá; esperamos a que el engine lo confirme
        // con jobFinished (puede que justo haya terminado la última página).
        emit cancelRequested();
        return;
    }

    // Todavía en espera: el engine ni se enteró, basta con actualizar la cola.
    const auto job = m_queue->job(jobId);
    if (job && job->status == Status::Queued)
        m_queue->setStatus(jobId, Status::Cancelled);
}

void PrinterViewModel::clearFinished() { m_queue->removeFinished(); }

void PrinterViewModel::pause() { emit pauseRequested(); }
void PrinterViewModel::resume() { emit resumeRequested(); }

void PrinterViewModel::cancel()
{
    if (m_activeJobId != -1)
        cancelJob(m_activeJobId);
}

// ---------------------------------------------------------------------------
// Novedades del worker (llegan por conexión queued => ya estamos en el hilo de UI)
// ---------------------------------------------------------------------------

void PrinterViewModel::onEngineStateChanged(State state)
{
    m_state = state;

    // Reflejamos la pausa de la impresora en la fila del trabajo activo.
    if (m_activeJobId != -1) {
        if (state == State::Paused)
            m_queue->setStatus(m_activeJobId, Status::Paused);
        else if (state == State::Printing)
            m_queue->setStatus(m_activeJobId, Status::Printing);
    }

    emit stateChanged();
}

void PrinterViewModel::onEngineProgress(int jobId, int page, int total)
{
    if (jobId != m_activeJobId)
        return; // defensivo: ignorar novedades de un trabajo que ya no es el activo

    m_page = page;
    m_total = total;
    m_queue->setPrintedPages(jobId, page);
    emit progressChanged();
}

void PrinterViewModel::onEngineJobFinished(int jobId, bool completed)
{
    m_queue->setStatus(jobId, completed ? Status::Completed : Status::Cancelled);

    if (jobId == m_activeJobId) {
        m_activeJobId = -1;
        m_jobName.clear();
        m_page = 0;
        m_total = 0;
        emit progressChanged();
    }

    dispatchNext(); // la impresora quedó libre: ¿hay otro esperando?
}

// ---------------------------------------------------------------------------

void PrinterViewModel::dispatchNext()
{
    if (m_activeJobId != -1)
        return; // la impresora ya tiene trabajo

    const auto next = m_queue->nextQueued();
    if (!next)
        return; // cola vacía

    // Marcamos el trabajo como activo ANTES de que el engine responda. Si el usuario
    // agrega otro trabajo en el medio, dispatchNext() ve m_activeJobId != -1 y no
    // manda un segundo startJob.
    m_activeJobId = next->id;
    m_jobName = next->name;
    m_page = 0;
    m_total = next->pages;
    m_queue->setStatus(next->id, Status::Printing);
    emit progressChanged();

    emit startRequested(next->id, next->name, next->pages);
}
