// SPDX-License-Identifier: GPL-3.0-or-later
/*
* SprintIDE - SprintIDE is a cross-platform IDE built from scratch with Qt 6 and C++20. Its core capability is to provide AI Agents with an executable, verifiable code execution environment.
* Copyright (C) 2026  Liu Tengjun (刘腾骏)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
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