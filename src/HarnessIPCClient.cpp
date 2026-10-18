#include "HarnessIPCClient.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

HarnessIPCClient::HarnessIPCClient(QObject *parent)
    : ILLMClient(parent)
    , m_pipeName("automatron-harness")
{
    m_socket = new QLocalSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(300);

    connect(m_socket, &QLocalSocket::readyRead, this, &HarnessIPCClient::onPipeReadyRead);
    connect(m_socket, &QLocalSocket::connected, this, &HarnessIPCClient::onPipeConnected);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &HarnessIPCClient::onPipeError);
    connect(m_reconnectTimer, &QTimer::timeout, this, &HarnessIPCClient::retryConnect);

    startHarnessProcess();
}

HarnessIPCClient::~HarnessIPCClient()
{
    if (m_socket && m_socket->isOpen()) {
        m_socket->close();
    }
    if (m_harnessProcess && m_harnessProcess->state() != QProcess::NotRunning) {
        m_harnessProcess->terminate();
        if (!m_harnessProcess->waitForFinished(1000)) {
            m_harnessProcess->kill();
        }
    }
}

void HarnessIPCClient::startHarnessProcess()
{
    // Search candidates for the harness executable
    QStringList candidates = {
        QCoreApplication::applicationDirPath() + "/automatron-harness.exe",
        QCoreApplication::applicationDirPath() + "/automatron-harness.bat",
        QCoreApplication::applicationDirPath() + "/automatron-harness.cmd",
        QCoreApplication::applicationDirPath() + "/harness.exe",
        "D:/Omniroute/agentic-harness-automatron/target/release/automatron-harness.exe",
        "D:/Omniroute/agentic-harness-automatron/target/debug/automatron-harness.exe"
    };

    QString foundBinary;
    for (const QString &path : candidates) {
        if (QFileInfo::exists(path)) {
            foundBinary = path;
            break;
        }
    }

    if (!foundBinary.isEmpty()) {
        qDebug() << "[HarnessIPC] Launching harness process from:" << foundBinary;
        m_harnessProcess = new QProcess(this);
        m_harnessProcess->setProgram(foundBinary);
        m_harnessProcess->setArguments({"--pipe-name", "\\\\.\\pipe\\" + m_pipeName});
        m_harnessProcess->setProcessChannelMode(QProcess::ForwardedChannels);

        connect(m_harnessProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &HarnessIPCClient::onHarnessProcessFinished);

        m_harnessProcess->start();
    } else {
        qDebug() << "[HarnessIPC] Harness binary not found in candidates; connecting to external pipe server...";
    }

    // Begin connection attempts
    m_connectAttempts = 0;
    m_reconnectTimer->start();
}

void HarnessIPCClient::retryConnect()
{
    m_connectAttempts++;
    if (m_isConnected || m_socket->state() == QLocalSocket::ConnectedState) {
        m_reconnectTimer->stop();
        return;
    }

    m_socket->connectToServer(m_pipeName);

    if (m_connectAttempts > 15) {
        m_reconnectTimer->stop();
        qWarning() << "[HarnessIPC] Could not connect to named pipe after" << m_connectAttempts << "attempts.";
    }
}

void HarnessIPCClient::onPipeConnected()
{
    m_isConnected = true;
    m_reconnectTimer->stop();
    qDebug() << "[HarnessIPC] Connected to Automatron Harness named pipe:" << m_pipeName;
}

void HarnessIPCClient::onPipeError(QLocalSocket::LocalSocketError socketError)
{
    if (m_isConnected) {
        qWarning() << "[HarnessIPC] Named pipe error:" << socketError << "-" << m_socket->errorString();
    }
}

void HarnessIPCClient::onHarnessProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    qDebug() << "[HarnessIPC] Harness process exited with code:" << exitCode << "status:" << exitStatus;
    m_isConnected = false;
    emit harnessStateChanged(4); // 4 = Terminated
}

void HarnessIPCClient::sendPrompt(const QString &prompt)
{
    QJsonObject obj;
    obj["type"] = "prompt_submit";
    obj["text"] = prompt;
    writeJsonLine(obj);
}

void HarnessIPCClient::cancel()
{
    QJsonObject obj;
    obj["type"] = "action_chip";
    obj["chip"] = "Stop";
    writeJsonLine(obj);
    emit cancelled();
}

void HarnessIPCClient::sendKill()
{
    QJsonObject obj;
    obj["type"] = "kill";
    writeJsonLine(obj);

    // If harness process does not exit within 1.5 seconds, force kill
    if (m_harnessProcess && m_harnessProcess->state() != QProcess::NotRunning) {
        QTimer::singleShot(1500, this, [this]() {
            if (m_harnessProcess && m_harnessProcess->state() != QProcess::NotRunning) {
                qDebug() << "[HarnessIPC] Force killing unresponsive harness process...";
                m_harnessProcess->kill();
            }
        });
    }
}

void HarnessIPCClient::submitConsentResponse(
    const QString &requestId,
    bool approved,
    int scopeExpansion,
    const QString &passwordProvided,
    const QString &operatorNotes)
{
    QJsonObject obj;
    obj["type"] = "consent_response";
    obj["request_id"] = requestId;
    obj["approved"] = approved;

    switch (scopeExpansion) {
        case 1:  obj["scope_expansion"] = "session"; break;
        case 2:  obj["scope_expansion"] = "persistent"; break;
        default: obj["scope_expansion"] = "once"; break;
    }

    if (!passwordProvided.isEmpty()) {
        obj["password_provided"] = passwordProvided;
    }
    if (!operatorNotes.isEmpty()) {
        obj["operator_notes"] = operatorNotes;
    }

    writeJsonLine(obj);
}

void HarnessIPCClient::writeJsonLine(const QJsonObject &json)
{
    if (!m_socket || m_socket->state() != QLocalSocket::ConnectedState) {
        // Fallback: If pipe is not yet connected, re-attempt
        m_socket->connectToServer(m_pipeName);
        if (!m_socket->waitForConnected(200)) {
            qWarning() << "[HarnessIPC] Cannot send message, socket not connected:" << json["type"].toString();
            return;
        }
    }

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
    m_socket->flush();
}

void HarnessIPCClient::onPipeReadyRead()
{
    m_lineBuffer.append(m_socket->readAll());

    while (m_lineBuffer.contains('\n')) {
        int newlineIdx = m_lineBuffer.indexOf('\n');
        QByteArray line = m_lineBuffer.left(newlineIdx).trimmed();
        m_lineBuffer.remove(0, newlineIdx + 1);

        if (!line.isEmpty()) {
            parseAndDispatch(line);
        }
    }
}

void HarnessIPCClient::parseAndDispatch(const QByteArray &line)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[HarnessIPC] JSON parse error on incoming message:" << err.errorString();
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();

    if (type == "state_change") {
        QString stateStr = obj["state"].toString();
        changeStateFromHarness(stateStr);
    }
    else if (type == "response_chunk") {
        QString chunk = obj["chunk"].toString();
        emit responseChunk(chunk);
    }
    else if (type == "response_complete") {
        emit responseComplete();
    }
    else if (type == "transcript") {
        QString sender = obj["sender"].toString();
        QString text = obj["text"].toString();
        emit transcriptMessage(sender, text);
    }
    else if (type == "tool_event") {
        emit toolEvent(
            obj["event_kind"].toString(),
            obj["tool_name"].toString(),
            obj["args"].toObject(),
            obj["output"].toString(),
            obj["error"].toString(),
            obj["risk_level"].toString()
        );
    }
    else if (type == "consent_request") {
        emit consentRequested(
            obj["id"].toString(),
            obj["path"].toString(),
            obj["operation"].toString(),
            obj["reason"].toString(),
            obj["is_high_risk"].toBool(),
            obj["requires_password"].toBool(),
            obj["tier"].toInt(1)
        );
    }
    else if (type == "error") {
        emit error(obj["message"].toString());
    }
    else if (type == "session_terminated") {
        emit harnessStateChanged(4); // Terminated
        emit cancelled();
    }
}

void HarnessIPCClient::changeStateFromHarness(const QString &stateStr)
{
    // AgentState mapping:
    // 0: Idle, 1: Listening, 2: Thinking, 3: Executing, 4: Terminated
    int stateInt = 0;
    if (stateStr.compare("Listening", Qt::CaseInsensitive) == 0) {
        stateInt = 1;
    } else if (stateStr.compare("Thinking", Qt::CaseInsensitive) == 0) {
        stateInt = 2;
    } else if (stateStr.compare("Executing", Qt::CaseInsensitive) == 0) {
        stateInt = 3;
    } else if (stateStr.compare("Terminated", Qt::CaseInsensitive) == 0) {
        stateInt = 4;
    }

    emit harnessStateChanged(stateInt);
}
