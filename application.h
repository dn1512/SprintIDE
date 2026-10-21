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
#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
class QSettings;
class QTranslator;
QT_END_NAMESPACE

namespace SprintIDE {

// 前向声明
class ConfigManager;
class PluginManager;
class ThemeManager;
class MainWindow;
class LogBuffer;

/**
 * @class Application
 * @brief 应用程序单例类，管理全局资源和模块协调
 *
 * 这是整个SprintIDE的核心类，负责：
 * 1. 应用程序的生命周期管理
 * 2. 各个模块的初始化和协调
 * 3. 全局事件处理
 * 4. 资源管理
 */
class Application : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Application)  // 禁止拷贝，确保单例

public:
    // 应用程序状态枚举
    enum class AppState {
        Starting,      // 正在启动
        Running,       // 运行中
        ShuttingDown,  // 正在关闭
        Error          // 错误状态
    };

    /**
     * @brief 获取Application单例实例
     * @return Application的引用
     */
    static Application& instance();

    // ========== 生命周期管理 ==========

    /**
     * @brief 初始化应用程序
     * @param argc 命令行参数个数
     * @param argv 命令行参数数组
     * @return 初始化是否成功
     */
    bool initialize(int argc, char *argv[]);

    /**
     * @brief 关闭应用程序
     */
    void shutdown();

    /**
     * @brief 重启应用程序
     * @return 重启是否成功
     */
    bool restart();

    // ========== 模块访问接口 ==========

    /**
     * @brief 获取配置管理器
     * @return ConfigManager指针
     */
    ConfigManager* configManager() const;

    /**
     * @brief 获取全局日志缓冲区（N1：AI 上下文总线）
     * @return LogBuffer指针；未初始化时返回 nullptr
     */
    LogBuffer*     logBuffer() const;

    /**
     * @brief 获取插件管理器
     * @return PluginManager指针
     */
    PluginManager* pluginManager() const;

    /**
     * @brief 获取主题管理器
     * @return ThemeManager指针
     */
    ThemeManager* themeManager() const;

    /**
     * @brief 获取主窗口
     * @return MainWindow指针
     */
    MainWindow* mainWindow() const;

    // ========== 全局状态查询 ==========

    /**
     * @brief 获取当前应用程序状态
     * @return 应用程序状态
     */
    AppState currentState() const;

    /**
     * @brief 获取应用程序版本
     * @return 版本字符串
     */
    QString version() const;

    /**
     * @brief 获取构建信息
     * @return 构建信息字符串
     */
    QString buildInfo() const;

    /**
     * @brief 获取应用程序启动时间
     * @return 启动时间
     */
    QDateTime startTime() const;

    // ========== 事件系统 ==========

    /**
     * @brief 发布事件
     * @param type 事件类型
     * @param data 事件数据
     */
    void postEvent(const QString &type, const QVariant &data);

    /**
     * @brief 订阅事件
     * @param type 事件类型
     * @param receiver 接收者对象
     * @param slot 槽函数
     */
    void subscribeEvent(const QString &type, QObject *receiver, const char *slot);

    /**
     * @brief 取消订阅事件
     * @param type 事件类型
     * @param receiver 接收者对象
     */
    void unsubscribeEvent(const QString &type, QObject *receiver);

    // ========== 资源管理 ==========

    /**
     * @brief 注册资源
     * @param name 资源名称
     * @param resource 资源对象
     */
    void registerResource(const QString &name, QObject *resource);

    /**
     * @brief 获取资源
     * @param name 资源名称
     * @return 资源对象指针
     */
    QObject* getResource(const QString &name) const;

    /**
     * @brief 释放资源
     * @param name 资源名称
     */
    void releaseResource(const QString &name);

    // ========== 会话管理 ==========

    /**
     * @brief 保存当前会话
     * @return 保存是否成功
     */
    bool saveSession();

    /**
     * @brief 恢复会话
     * @return 恢复是否成功
     */
    bool restoreSession();

    /**
     * @brief 检查是否有未保存的会话
     * @return 是否有未保存的会话
     */
    bool hasUnsavedSession() const;

    // ========== 国际化 ==========

    /**
     * @brief 设置当前语言
     * @param languageCode 语言代码 (如 "zh_CN", "en_US")
     */
    void setLanguage(const QString &languageCode);

    /**
     * @brief 获取当前语言
     * @return 当前语言代码
     */
    QString currentLanguage() const;

    /**
     * @brief 获取可用的语言列表
     * @return 语言代码列表
     */
    QStringList availableLanguages() const;

signals:
    /**
     * @brief 应用程序初始化完成信号
     */
    void initialized();

    /**
     * @brief 应用程序即将关闭信号
     */
    void aboutToShutdown();

    /**
     * @brief 应用程序状态改变信号
     * @param newState 新状态
     */
    void stateChanged(AppState newState);

    /**
     * @brief 事件发布信号
     * @param type 事件类型
     * @param data 事件数据
     */
    void eventPosted(const QString &type, const QVariant &data);

    /**
     * @brief 发生错误信号
     * @param error 错误信息
     * @param severity 错误严重程度
     */
    void errorOccurred(const QString &error, int severity);

    /**
     * @brief 资源警告信号
     * @param usagePercent 资源使用百分比
     */
    void resourceWarning(int usagePercent);

private:
    /**
     * @brief 私有构造函数，确保单例模式
     * @param parent 父对象
     */
    Application();

    /**
     * @brief 析构函数
     */
    ~Application();

    // 私有实现方法
    bool loadCoreModules();      // 加载核心模块
    bool loadConfigurations();   // 加载配置
    bool setupSignalHandlers();  // 设置信号处理器
    void cleanupResources();     // 清理资源

    class Private;               // 私有实现类
    QScopedPointer<Private> d;   // 私有实现指针
};

} // namespace SprintIDE

#endif // APPLICATION_H
