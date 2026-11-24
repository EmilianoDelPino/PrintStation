#include "PrintEngine.h"
#include <QDebug>
#include <QThread>

using PrinterState::State;

PrintEngine::PrintEngine(QObject *parent)
    : QObject(parent), m_timer(new QTimer(this))
{
    m_timer->setInterval(400); // "tiempo por página" simulado
    connect(m_timer, &QTimer::timeout, this, &PrintEngine::onTick);
}

void PrintEngine::startJob(int jobId, const QString &name, int pages)
{
    if (m_state == State::Printing || m_state == State::Paused) {
        // No debería pasar: el ViewModel sólo despacha cuando no hay trabajo activo.
        qWarning() << "[PrintEngine] startJob ignorado, ya hay un trabajo en curso:" << m_jobId;
        return;
    }

    qDebug() << "[PrintEngine] startJob" << jobId << name << "en hilo:" << QThread::currentThread();

    m_jobId = jobId;
    m_jobName = name;
    m_total = qMax(1, pages);
    m_page = 0;
    emit progressChanged(m_jobId, m_page, m_total);
    setState(State::Printing);
    m_timer->start();
}

void PrintEngine::pause()
{
    if (m_state != State::Printing)
        return;
    m_timer->stop();
    setState(State::Paused);
}

void PrintEngine::resume()
{
    if (m_state != State::Paused)
        return;
    setState(State::Printing);
    m_timer->start();
}

void PrintEngine::cancel()
{
    if (m_state != State::Printing && m_state != State::Paused)
        return;
    m_timer->stop();
    setState(State::Idle);
    emit jobFinished(m_jobId, false);
}

void PrintEngine::onTick()
{
    ++m_page;
    emit progressChanged(m_jobId, m_page, m_total);

    if (m_page >= m_total) {
        m_timer->stop();
        setState(State::Idle);
        emit jobFinished(m_jobId, true);
    }
}

void PrintEngine::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged(m_state);
}
