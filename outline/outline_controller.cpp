#include "outline_controller.h"
#include "outline_provider_factory.h"
#include "text_editor.h"

#include <QTextDocument>

namespace SprintIDE {

    OutlineController::OutlineController(TextEditor* editor, QObject* parent)
    : QObject(parent)
    , m_editor(editor)
    {
        m_debounce = new QTimer(this);
        m_debounce->setSingleShot(true);
        m_debounce->setInterval(300);
        connect(m_debounce, &QTimer::timeout, this, &OutlineController::scan);

        if (m_editor) {
            connect(m_editor, &QPlainTextEdit::textChanged,
                this, &OutlineController::onTextChanged);
            connect(m_editor, &TextEditor::cursorPositionChanged,
                this, &OutlineController::onCursorPositionChanged);
        }

        // 立即扫一次
        scan();
    }

    OutlineController::~OutlineController() = default;

    void OutlineController::refreshNow()
    {
        m_debounce->stop();
        scan();
    }

    void OutlineController::onTextChanged()
    {
        // 大文件直接跳过（避免卡顿）
        if (m_editor && m_editor->document()->characterCount() > 500000) {
            return;
        }
        m_debounce->start();
    }

    void OutlineController::onCursorPositionChanged(int /*line*/, int /*column*/)
    {
        emit currentPathChanged();
    }

    void OutlineController::scan()
    {
        if (!m_editor) return;

        const QString languageId = m_editor->currentLanguageId();
        if (!m_provider || languageId != m_editor->currentLanguageId()) {
            m_provider = OutlineProviderFactory::forLanguage(languageId);
        }

        if (!m_provider) {
            m_symbols.clear();
            emit symbolsChanged();
            return;
        }

        const QString text = m_editor->toPlainText();
        m_symbols = m_provider->scan(text, m_editor->filePath());
        emit symbolsChanged();
        emit currentPathChanged();
    }

} // namespace SprintIDE