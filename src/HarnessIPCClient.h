#pragma once

#include <QProcess>
#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include "ILLMClient.h"

// ---------------------------------------------------------------------------
// HarnessIPCClient
// ---------------------------------------------------------------------------
// Implements the ILLMClient interface for communication with the Automatron
// Rust harness over Windows Named Pipe (QLocalSocket).
// Manages a child process (automatron-harness.exe) and bidirectional JSON-RPC.
class HarnessIPCClient : public ILLMClient
{
    Q_OBJECT

public:
    explicit HarnessIPCClient(QObject *parent = nullptr);
    ~HarnessIPCClient() override;

public slots:
    // ILLMClient implementation
    void sendPrompt(const QString &prompt) override;
    void cancel() override;

    // Consent response submission
    void submitConsentResponse(
        const QString &requestId,
        bool approved,
        int scopeExpansion,    // 0=Once, 1=Session, 2=Persistent
        const QString &passwordProvided,
        const QString &operatorNotes
    );

    // Emergency kill switch
    void sendKill();

signals:
    // Extended harness-specific signals
    void harnessStateChanged(int agentState);          // AgentState enum value
    void consentRequested(
        const QString &requestId,
        const QString &path,
        const QString &operation,
        const QString &reason,
        bool isHighRisk,
        bool requiresPassword,
        int tier
    );
    void transcriptMessage(const QString &sender, const QString &text);
    void toolEvent(
        const QString &eventKind,
        const QString &toolName,
        const QJsonObject &args,
        const QString &output,
        const QString &error,
        const QString &riskLevel
    );

private slots:
    void onPipeReadyRead();
    void onPipeConnected();
    void onPipeError(QLocalSocket::LocalSocketError socketError);
    void onHarnessProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void retryConnect();

private:
    void startHarnessProcess();
    void writeJsonLine(const QJsonObject &json);
    void parseAndDispatch(const QByteArray &line);
    void changeStateFromHarness(const QString &stateStr);

private:
    QProcess *m_harnessProcess = nullptr;
    QLocalSocket *m_socket = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QByteArray m_lineBuffer;
    QString m_pipeName;
    bool m_isConnected = false;
    int m_connectAttempts = 0;
};
