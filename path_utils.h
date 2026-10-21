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
#pragma once

#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace SprintIDE {

/**
 * @brief 路径工具类
 *
 * 提供跨平台的路径处理功能，确保在Windows、macOS和Linux上的兼容性
 * 包含路径验证、规范化、转换和相对路径计算等功能
 */
class PathUtils {
public:
    /**
     * @brief 规范化路径
     *
     * 将路径转换为标准格式，移除多余的分隔符和'.'、'..'引用
     *
     * @param path 原始路径
     * @return QString 规范化后的路径
     */
    static QString normalizePath(const QString& path);

    /**
     * @brief 转换为原生分隔符
     *
     * 将路径中的分隔符转换为当前平台的原生分隔符
     *
     * @param path 原始路径
     * @return QString 转换后的路径
     */
    static QString toNativeSeparators(const QString& path);

    /**
     * @brief 转换为通用分隔符
     *
     * 将路径中的分隔符转换为通用分隔符（/）
     *
     * @param path 原始路径
     * @return QString 转换后的路径
     */
    static QString toUniversalSeparators(const QString& path);

    /**
     * @brief 获取相对路径
     *
     * 计算从源路径到目标路径的相对路径
     *
     * @param fromPath 源路径
     * @param toPath 目标路径
     * @return QString 相对路径，如果无法计算则返回空字符串
     */
    static QString relativePath(const QString& fromPath, const QString& toPath);

    /**
     * @brief 获取绝对路径
     *
     * 将相对路径转换为绝对路径，基于当前工作目录或指定基目录
     *
     * @param path 原始路径
     * @param baseDir 基目录（默认为当前工作目录）
     * @return QString 绝对路径
     */
    static QString absolutePath(const QString& path, const QString& baseDir = QString());

    /**
     * @brief 验证路径是否有效
     *
     * 检查路径是否包含非法字符，格式是否正确
     *
     * @param path 要验证的路径
     * @return bool 有效返回true
     */
    static bool isValidPath(const QString& path);

    /**
     * @brief 验证文件路径
     *
     * 检查文件路径是否有效且文件是否存在（可选）
     *
     * @param filePath 文件路径
     * @param checkExists 是否检查文件存在
     * @return bool 有效返回true
     */
    static bool isValidFilePath(const QString& filePath, bool checkExists = false);

    /**
     * @brief 验证目录路径
     *
     * 检查目录路径是否有效且目录是否存在（可选）
     *
     * @param dirPath 目录路径
     * @param checkExists 是否检查目录存在
     * @return bool 有效返回true
     */
    static bool isValidDirPath(const QString& dirPath, bool checkExists = false);

    /**
     * @brief 确保目录存在
     *
     * 如果目录不存在则创建它（包括父目录）
     *
     * @param dirPath 目录路径
     * @return bool 创建成功或已存在返回true
     */
    static bool ensureDirExists(const QString& dirPath);

    /**
     * @brief 获取文件扩展名
     *
     * 提取文件路径的扩展名（不包含点）
     *
     * @param filePath 文件路径
     * @return QString 扩展名，没有扩展名则返回空字符串
     */
    static QString fileExtension(const QString& filePath);

    /**
     * @brief 获取文件名（不含扩展名）
     *
     * 提取文件路径的文件名（不含目录和扩展名）
     *
     * @param filePath 文件路径
     * @return QString 文件名
     */
    static QString fileNameWithoutExtension(const QString& filePath);

    /**
     * @brief 获取平台特定的配置目录
     *
     * 根据操作系统返回合适的配置目录
     *
     * @return QString 配置目录路径
     */
    static QString platformConfigPath();

    /**
     * @brief 获取平台特定的数据目录
     *
     * 根据操作系统返回合适的数据目录
     *
     * @return QString 数据目录路径
     */
    static QString platformDataPath();

    /**
     * @brief 获取平台特定的临时目录
     *
     * 根据操作系统返回临时目录
     *
     * @return QString 临时目录路径
     */
    static QString platformTempPath();

    /**
     * @brief 路径比较（跨平台安全）
     *
     * 跨平台比较两个路径是否指向相同位置
     *
     * @param path1 路径1
     * @param path2 路径2
     * @return bool 相同返回true
     */
    static bool pathsEqual(const QString& path1, const QString& path2);

    /**
     * @brief 转换为URL格式
     *
     * 将文件路径转换为file:// URL格式
     *
     * @param filePath 文件路径
     * @return QUrl URL对象
     */
    static QUrl toFileUrl(const QString& filePath);

    /**
     * @brief 从URL获取路径
     *
     * 从file:// URL提取文件路径
     *
     * @param url URL对象
     * @return QString 文件路径
     */
    static QString fromFileUrl(const QUrl& url);

    /**
     * @brief 清理文件名
     *
     * 移除文件名中的非法字符，使其适合作为文件名
     *
     * @param fileName 原始文件名
     * @return QString 清理后的文件名
     */
    static QString sanitizeFileName(const QString& fileName);

    /**
     * @brief 合并路径
     *
     * 将多个路径部分合并为一个完整路径
     *
     * @param parts 路径部分列表
     * @return QString 合并后的路径
     */
    static QString joinPaths(const QStringList& parts);

private:
    PathUtils() = delete; ///< 禁止实例化，所有方法都是静态的
};

} // namespace SprintIDE
