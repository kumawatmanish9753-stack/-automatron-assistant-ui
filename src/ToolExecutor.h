#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <functional>
#include <map>
#include <memory>

// Forward declaration to break the circular dependency with AgentWorker.h
class CancellationToken;

// ---------------------------------------------------------------------------
// Tool execution data structures
// ---------------------------------------------------------------------------
struct ToolCall {
    QString name;
    QVariantMap arguments;
    QString callId; // Unique identifier if provided by LLM
};

struct ToolResult {
    bool success = false;
    QString output;
    QString error;
    bool cancelled = false;
};

// ---------------------------------------------------------------------------
// Typedef for registered tool functions
// ---------------------------------------------------------------------------
// A tool handler takes the call args and a cancellation token, and returns a ToolResult
using ToolHandler = std::function<ToolResult(const QVariantMap&, CancellationToken&)>;

// ---------------------------------------------------------------------------
// ToolExecutor
// ---------------------------------------------------------------------------
// Safely executes allowed, registered tools. No arbitrary shell execution.
// Designed to run on a background thread.
class ToolExecutor : public QObject
{
    Q_OBJECT

public:
    explicit ToolExecutor(QObject *parent = nullptr);
    ~ToolExecutor() override = default;

    // Execute a tool call synchronously on the current thread
    // Must be called from the worker thread, NEVER the GUI thread
    ToolResult execute(const ToolCall &call, CancellationToken &cancelToken);

    // Check if a tool is registered
    bool hasTool(const QString &name) const;

private:
    void registerDefaultTools();

    // The registered tools dictionary
    std::map<QString, ToolHandler> m_registeredTools;
};