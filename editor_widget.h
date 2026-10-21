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
#ifndef EDITOR_WIDGET_H
#define EDITOR_WIDGET_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QTextCursor>

namespace SprintIDE {

/**
 * @class EditorWidget
 * @brief 文本编辑器核心组件，支持代码编辑和基础语法分析
 *
 * 这个类是基于QPlainTextEdit的扩展，提供了：
 * 1. 代码编辑功能（复制行、删除行、移动行等）
 * 2. 行号显示和当前行高亮
 * 3. 代码注释功能
 * 4. 查找替换功能
 * 5. 文件加载和保存
 * 6. 光标位置跟踪
 */
class EditorWidget : public QPlainTextEdit
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口
     */
    explicit EditorWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~EditorWidget();

    // ========== 文件操作 ==========

    /**
     * @brief 加载文件
     * @param filePath 文件路径
     * @return 加载是否成功
     */
    bool loadFile(const QString &filePath);

    /**
     * @brief 保存文件
     * @return 保存是否成功
     */
    bool saveFile();

    /**
     * @brief 另存文件
     * @param filePath 新文件路径
     * @return 保存是否成功
     */
    bool saveAs(const QString &filePath);

    /**
     * @brief 关闭文件
     */
    void closeFile();

    // ========== 文本操作 ==========

    /**
     * @brief 获取选中的文本
     * @return 选中的文本
     */
    QString selectedText() const;

    /**
     * @brief 复制当前行到下一行
     */
    void duplicateLine();

    /**
     * @brief 删除当前行
     */
    void deleteLine();

    /**
     * @brief 向上移动当前行
     */
    void moveLineUp();

    /**
     * @brief 向下移动当前行
     */
    void moveLineDown();

    /**
     * @brief 注释选中的代码
     */
    void commentSelection();

    /**
     * @brief 取消注释选中的代码
     */
    void uncommentSelection();

    // ========== 光标和位置操作 ==========

    /**
     * @brief 获取当前光标所在行
     * @return 行号（从1开始）
     */
    int cursorLine() const;

    /**
     * @brief 获取当前光标所在列
     * @return 列号（从1开始）
     */
    int cursorColumn() const;

    /**
     * @brief 跳转到指定行
     * @param line 行号（从1开始）
     */
    void gotoLine(int line);

    /**
     * @brief 跳转到指定位置
     * @param line 行号（从1开始）
     * @param column 列号（从1开始）
     */
    void gotoPosition(int line, int column);

    // ========== 查找替换 ==========

    /**
     * @brief 查找文本
     * @param text 要查找的文本
     * @param caseSensitive 是否区分大小写
     * @param wholeWord 是否全词匹配
     */
    void find(const QString &text, bool caseSensitive = false, bool wholeWord = false);

    /**
     * @brief 替换文本
     * @param findStr 要查找的文本
     * @param replaceStr 要替换的文本
     * @param caseSensitive 是否区分大小写
     * @param wholeWord 是否全词匹配
     */
    void replace(const QString &findStr, const QString &replaceStr, bool caseSensitive = false, bool wholeWord = false);

    /**
     * @brief 替换所有匹配的文本
     * @param findStr 要查找的文本
     * @param replaceStr 要替换的文本
     * @param caseSensitive 是否区分大小写
     * @param wholeWord 是否全词匹配
     */
    void replaceAll(const QString &findStr, const QString &replaceStr, bool caseSensitive = false, bool wholeWord = false);

    // ========== 状态查询 ==========

    /**
     * @brief 检查文件是否被修改
     * @return 是否被修改
     */
    bool isModified() const;

    /**
     * @brief 获取文件路径
     * @return 文件路径
     */
    QString filePath() const;

    /**
     * @brief 获取文件名（不含路径）
     * @return 文件名
     */
    QString fileName() const;

    /**
     * @brief 获取文件语言类型
     * @return 语言类型
     */
    QString language() const;

    /**
     * @brief 获取总行数
     * @return 行数
     */
    int lineCount() const;

    // ========== 编辑器设置 ==========

    /**
     * @brief 设置Tab宽度
     * @param width Tab宽度（空格数）
     */
    void setTabWidth(int width);

    /**
     * @brief 设置是否使用空格代替Tab
     * @param useSpaces 是否使用空格
     */
    void setUseSpaces(bool useSpaces);

    /**
     * @brief 设置是否显示行号
     * @param show 是否显示
     */
    void setShowLineNumbers(bool show);

    /**
     * @brief 设置是否高亮当前行
     * @param highlight 是否高亮
     */
    void setHighlightCurrentLine(bool highlight);

    /**
     * @brief 设置是否自动换行
     * @param wrap 是否换行
     */
    void setWordWrap(bool wrap);

signals:
    /**
     * @brief 文件加载完成信号
     * @param path 文件路径
     */
    void fileLoaded(const QString &path);

    /**
     * @brief 文件保存完成信号
     * @param path 文件路径
     */
    void fileSaved(const QString &path);

    /**
     * @brief 文件关闭信号
     * @param path 文件路径
     */
    void fileClosed(const QString &path);

    /**
     * @brief 文件修改状态改变信号
     * @param modified 是否被修改
     */
    void modificationChanged(bool modified);

    /**
     * @brief 光标位置改变信号
     * @param line 行号
     * @param column 列号
     */
    void cursorPositionChanged(int line, int column);

    /**
     * @brief 文本内容改变信号
     */
    void textChanged();

protected:
    // 重写父类的事件处理函数
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    /**
     * @brief 更新行号区域的宽度
     */
    void updateLineNumberAreaWidth();

    /**
     * @brief 更新行号区域
     * @param rect 更新区域
     * @param dy 垂直偏移
     */
    void updateLineNumberArea(const QRect &rect, int dy);

    /**
     * @brief 高亮当前行
     */
    void highlightCurrentLine();

private:
    // 行号区域类（内部类）
    class LineNumberArea;

    // 初始化方法
    void setupEditor();              // 设置编辑器
    void setupLineNumberArea();      // 设置行号区域
    void setupHighlighter();         // 设置语法高亮

    // 行号区域辅助方法
    int lineNumberAreaWidth() const;                    // 计算行号区域宽度
    void lineNumberAreaPaintEvent(QPaintEvent *event);  // 行号区域绘制事件

    // 私有实现类
    class Private;
    Private* d;
};

/**
 * @class EditorWidget::LineNumberArea
 * @brief 行号区域小部件
 *
 * 这个类负责显示编辑器左侧的行号区域
 */
class EditorWidget::LineNumberArea : public QWidget
{
public:
    /**
     * @brief 构造函数
     * @param editor 所属的编辑器
     */
    explicit LineNumberArea(EditorWidget *editor) : QWidget(editor), editor(editor) {}

    /**
     * @brief 获取建议大小
     * @return 建议的大小
     */
    QSize sizeHint() const override {
        return QSize(editor->lineNumberAreaWidth(), 0);
    }

protected:
    /**
     * @brief 绘制事件
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent *event) override {
        editor->lineNumberAreaPaintEvent(event);
    }

private:
    EditorWidget *editor;  // 所属的编辑器
};

} // namespace SprintIDE

#endif // EDITOR_WIDGET_H
