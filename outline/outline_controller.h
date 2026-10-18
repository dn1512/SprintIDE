#ifndef OUTLINE_CONTROLLER_H
#define OUTLINE_CONTROLLER_H

#include <QObject>
#include <QTimer>
#include "outline_types.h"

namespace SprintIDE {

    class TextEditor;
    class IOutlineProvider;

    class OutlineController : public QObject
    {
        Q_OBJECT
        public:
        explicit OutlineController(TextEditor* editor, QObject* parent = nullptr);
        ~OutlineController();

        const QList<OutlineSymbol>& symbols() const { return m_symbols; }

        /// 立即扫描一次（切换标签、保存等时调用）
        void refreshNow();

        signals:
        void symbolsChanged();
        void currentPathChanged();

        private slots:
        void onTextChanged();
        void onCursorPositionChanged(int line, int column);

        private:
        void scan();

        TextEditor*                   m_editor = nullptr;
        QTimer*                       m_debounce = nullptr;
        std::unique_ptr<IOutlineProvider> m_provider;
        QList<OutlineSymbol>          m_symbols;
    };

} // namespace SprintIDE

#endif