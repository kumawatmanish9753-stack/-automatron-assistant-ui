#include "ToolExecutor.h"
#include "AgentWorker.h"
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfoList>
#include <QThread>

ToolExecutor::ToolExecutor(QObject *parent)
    : QObject(parent)
{
    registerDefaultTools();
}

bool ToolExecutor::hasTool(const QString &name) const
{
    return m_registeredTools.find(name) != m_registeredTools.end();
}

ToolResult ToolExecutor::execute(const ToolCall &call, CancellationToken &cancelToken)
{
    ToolResult result;

    if (cancelToken.stop_requested())
    {
        result.cancelled = true;
        result.error = "Cancelled before execution";
        return result;
    }

    auto it = m_registeredTools.find(call.name);
    if (it == m_registeredTools.end())
    {
        result.success = false;
        result.error = QString("Unknown or unregistered tool: %1").arg(call.name);
        return result;
    }

    try
    {
        // Execute the registered handler
        result = it->second(call.arguments, cancelToken);
    }
    catch (const std::exception &e)
    {
        result.success = false;
        result.error = QString("Tool execution threw exception: %1").arg(e.what());
    }
    catch (...)
    {
        result.success = false;
        result.error = "Tool execution threw unknown exception";
    }

    return result;
}

void ToolExecutor::registerDefaultTools()
{
    // ---------------------------------------------------------
    // Tool: get_current_time
    // Description: Returns the system's current date and time
    // ---------------------------------------------------------
    m_registeredTools["get_current_time"] = [](const QVariantMap& /*args*/, CancellationToken& cancelToken) -> ToolResult {
        ToolResult res;
        if (cancelToken.stop_requested()) {
            res.cancelled = true;
            return res;
        }

        res.success = true;
        res.output = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
        return res;
    };

    // ---------------------------------------------------------
    // Tool: list_files
    // Description: Lists files in a specified directory safely.
    // ---------------------------------------------------------
    m_registeredTools["list_files"] = [](const QVariantMap& args, CancellationToken& cancelToken) -> ToolResult {
        ToolResult res;

        if (cancelToken.stop_requested()) {
            res.cancelled = true;
            return res;
        }

        QString dirPath = args.value("path").toString();
        if (dirPath.isEmpty()) {
            // Default to current directory if none provided
            dirPath = QDir::currentPath();
        }

        QDir dir(dirPath);
        if (!dir.exists()) {
            res.success = false;
            res.error = "Directory does not exist.";
            return res;
        }

        // Simulating a slightly long-running operation to test cancellation check mid-execution
        QStringList outputLines;
        outputLines << "Contents of " + dirPath + ":";

        QFileInfoList list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo &fileInfo : list)
        {
            // Cooperative cancellation check during loop
            if (cancelToken.stop_requested()) {
                res.cancelled = true;
                return res;
            }

            QString type = fileInfo.isDir() ? "[DIR]" : "[FILE]";
            outputLines << QString("%1 %2").arg(type, -6).arg(fileInfo.fileName());

            // Optional: Simulate heavy I/O for testing cancellation responsiveness
            // QThread::usleep(5000); // 5ms per file
        }

        res.success = true;
        res.output = outputLines.join("\n");
        return res;
    };

    // ---------------------------------------------------------
    // Tool: open_url_or_app
    // Description: Safely asks the OS to open a URL or file path using QDesktopServices
    // ---------------------------------------------------------
    m_registeredTools["open_url"] = [](const QVariantMap& args, CancellationToken& cancelToken) -> ToolResult {
        ToolResult res;

        if (cancelToken.stop_requested()) {
            res.cancelled = true;
            return res;
        }

        QString target = args.value("url").toString();
        if (target.isEmpty()) {
            res.success = false;
            res.error = "Argument 'url' is required";
            return res;
        }

        QUrl url(target);
        // Only allow safe schemes
        if (url.scheme() != "http" && url.scheme() != "https" && url.scheme() != "mailto") {
             // If no scheme, treat as a file path if it exists locally, but for safety in this demo, restrict it.
             res.success = false;
             res.error = "Only http, https, and mailto schemes are allowed for security.";
             return res;
        }

        bool opened = QDesktopServices::openUrl(url);
        if (opened) {
            res.success = true;
            res.output = "Successfully opened target.";
        } else {
            res.success = false;
            res.error = "OS refused to open the target.";
        }
        return res;
    };
}