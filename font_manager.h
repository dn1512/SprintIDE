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
#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <QFontDatabase>
#include <QString>
#include <QDebug>

class FontManager
{
public:
    static bool loadEmbeddedFonts();
    static QFont getDefaultMonoFont(int pointSize = 12);
    static bool isJetBrainsMonoAvailable();
    static QString primaryMonoFamily(); // 返回实际可用的等宽字体家族名
};

#endif // FONT_MANAGER_H
