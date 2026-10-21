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
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QStringList>

namespace SprintIDE {

/**
 * @class ConfigManager
 * @brief 配置管理系统
 *
 * 完全按照设计文档实现，只是文件放在根目录
 */
class ConfigManager : public QObject
{
    Q_OBJECT

public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();

    // 初始化
    bool initialize();

    // 基本配置操作
    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    bool contains(const QString &key) const;
    void remove(const QString &key);

    // 配置组操作
    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;

    // 配置类型操作
    void setArray(const QString &prefix, const QVariantList &array);
    QVariantList getArray(const QString &prefix) const;

    void setMap(const QString &prefix, const QVariantMap &map);
    QVariantMap getMap(const QString &prefix) const;

    // 文件操作
    bool loadConfig(const QString &filePath = QString());
    bool saveConfig(const QString &filePath = QString());
    bool importConfig(const QString &filePath);
    bool exportConfig(const QString &filePath);

    // 项目配置
    bool loadProjectConfig(const QString &projectFilePath);
    bool saveProjectConfig(const QString &projectFilePath = QString());

    // 配置版本管理
    bool migrateConfig(int fromVersion, int toVersion);
    int configVersion() const;

    // 配置验证
    bool validateConfig() const;
    QStringList validateErrors() const;

    // 配置监控
    void setAutoSave(bool enable);
    bool autoSave() const;

    // 备份与恢复
    bool backupConfig();
    bool restoreBackup();
    QList<QString> availableBackups() const;

signals:
    void configChanged(const QString &key, const QVariant &value);
    void configLoaded(const QString &filePath);
    void configSaved(const QString &filePath);
    void configError(const QString &error);
    void configRestored();

    // 批量操作信号
    void beginBatchUpdate();
    void endBatchUpdate();

private:
    class Private;
    QScopedPointer<Private> d;
};

} // namespace SprintIDE

#endif // CONFIG_MANAGER_H
