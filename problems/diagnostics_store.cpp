#include "diagnostics_store.h"

#include <QDebug>

namespace SprintIDE {

    DiagnosticsStore::DiagnosticsStore(QObject* parent)
    : QObject(parent)
    {
    }

    void DiagnosticsStore::replace(const QString& filePath, const QString& source,
        const QList<Diagnostic>& diags)
    {
        if (filePath.isEmpty() || source.isEmpty()) return;

        auto& sourceMap = m_data[filePath];

        if (diags.isEmpty()) {
            // 空列表：删除该 source（而不是保留空项）
            sourceMap.remove(source);
            if (sourceMap.isEmpty()) {
                m_data.remove(filePath);
            }
        } else {
            sourceMap[source] = diags;
        }

        emit fileDiagnosticsChanged(filePath);
        emit diagnosticsChanged();
    }

    void DiagnosticsStore::clearSource(const QString& source)
    {
        if (source.isEmpty()) return;

        bool anyChanged = false;
        for (auto it = m_data.begin(); it != m_data.end(); ) {
            auto& sourceMap = it.value();
            if (sourceMap.remove(source) > 0) {
                anyChanged = true;
            }
            if (sourceMap.isEmpty()) {
                it = m_data.erase(it);
            } else {
                ++it;
            }
        }

        if (anyChanged) {
            emit diagnosticsChanged();
        }
    }

    void DiagnosticsStore::clearFile(const QString& filePath)
    {
        if (m_data.remove(filePath) > 0) {
            emit fileDiagnosticsChanged(filePath);
            emit diagnosticsChanged();
        }
    }

    void DiagnosticsStore::clearAll()
    {
        if (m_data.isEmpty()) return;
        m_data.clear();
        emit diagnosticsChanged();
    }

    QList<Diagnostic> DiagnosticsStore::all() const
    {
        QList<Diagnostic> out;
        for (auto fileIt = m_data.constBegin(); fileIt != m_data.constEnd(); ++fileIt) {
            for (auto srcIt = fileIt.value().constBegin();
                srcIt != fileIt.value().constEnd(); ++srcIt) {
                out.append(srcIt.value());
            }
        }
        return out;
    }

    QList<Diagnostic> DiagnosticsStore::forFile(const QString& filePath) const
    {
        QList<Diagnostic> out;
        auto it = m_data.constFind(filePath);
        if (it == m_data.constEnd()) return out;

        for (auto srcIt = it.value().constBegin();
            srcIt != it.value().constEnd(); ++srcIt) {
            out.append(srcIt.value());
        }
        return out;
    }

    int DiagnosticsStore::errorCount() const
    {
        int n = 0;
        for (auto fileIt = m_data.constBegin(); fileIt != m_data.constEnd(); ++fileIt) {
            for (auto srcIt = fileIt.value().constBegin();
                srcIt != fileIt.value().constEnd(); ++srcIt) {
                for (const auto& d : srcIt.value()) {
                    if (d.isError()) ++n;
                }
            }
        }
        return n;
    }

    int DiagnosticsStore::warningCount() const
    {
        int n = 0;
        for (auto fileIt = m_data.constBegin(); fileIt != m_data.constEnd(); ++fileIt) {
            for (auto srcIt = fileIt.value().constBegin();
                srcIt != fileIt.value().constEnd(); ++srcIt) {
                for (const auto& d : srcIt.value()) {
                    if (d.isWarning()) ++n;
                }
            }
        }
        return n;
    }

} // namespace SprintIDE
