#include "application.h"
#include "config_manager.h"
#include "theme_manager.h"
#include "core/log_buffer.h"

#include <QCoreApplication>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QObject>

#include "builder/builder_factory.h"
#include "translation/language_manager.h"

namespace SprintIDE {

class Application::Private
{
public:
    ConfigManager* configManager = nullptr;
    ThemeManager* themeManager = nullptr;
    LogBuffer*     logBuffer     = nullptr;
    bool initialized = false;
    Application::AppState state = Application::AppState::Starting;
    QDateTime startTime;
};

Application& Application::instance()
{
    static Application instance;
    return instance;
}

Application::Application()
    : d(new Private)
{
    d->startTime = QDateTime::currentDateTime();
}

Application::~Application()
{
    qDebug() << "[Application] 析构开始";

    // （LogBuffer 不是 QObject，不会被 Qt parent 机制自动清理）
    if (d->logBuffer) {
        delete d->logBuffer;
        d->logBuffer = nullptr;
    }

    // 清理 BuilderFactory（纯静态资源，必须在 Qt 基础设施还存在时清理）
    SprintIDE::BuilderFactory::shutdown();

    qDebug() << "[Application] 析构完成";
}

bool Application::initialize(int argc, char *argv[])
{
    if (d->initialized) {
        qWarning() << "Application already initialized";
        return true;
    }

    d->state = AppState::Starting;
    qDebug() << "Initializing SprintIDE Application";

    // 记录命令行参数（调试用）
    qDebug() << "Command line arguments:";
    for (int i = 0; i < argc; ++i) {
        qDebug() << "  [" << i << "]" << argv[i];
    }

    // 初始化配置管理器
    d->configManager = new ConfigManager(this);
    if (!d->configManager->initialize()) {
        qCritical() << "Failed to initialize ConfigManager";
        d->state = AppState::Error;
        return false;
    }

    // 初始化日志缓冲区（N1：AI 上下文总线）
    {
        const int cfgCap = d->configManager->value("AI/LogBufferSize", 2000).toInt();
        const int cap    = qBound(200, cfgCap, 100000);
        d->logBuffer = new LogBuffer(cap);
        qDebug() << "[Application] LogBuffer 已创建，容量:" << cap;
    }

    // 设置应用程序信息（如果还没有设置）
    if (QCoreApplication::applicationName().isEmpty()) {
        QCoreApplication::setApplicationName("SprintIDE");
        QCoreApplication::setApplicationVersion("1.0.0");
        QCoreApplication::setOrganizationName("SprintIDE");
        QCoreApplication::setOrganizationDomain("sprintide.org");
    }

    d->initialized = true;
    d->state = AppState::Running;

    // 初始化主题管理器
    d->themeManager = &ThemeManager::instance(); // ThemeManager是单例

    if (d->configManager) {
        const QString fontFamily = d->configManager->value(
                                                       "Editor/FontFamily", "Monaco").toString();
        const int fontSize = d->configManager->value(
                                                 "Editor/FontSize", 12).toInt();

        // 范围硬约束，避免配置被改坏
        const int clampedSize = qBound(8, fontSize, 24);

        d->themeManager->setEditorFont(fontFamily, clampedSize);


        {
            const QString savedTheme = d->configManager->value(
                                                           "General/Theme", "Dark").toString();

            if (savedTheme.compare(QLatin1String("Light"), Qt::CaseInsensitive) == 0) {
                d->themeManager->setTheme(ThemeManager::ThemeType::Light);
                qDebug() << "[Application] 已恢复主题: Light";
            } else {
                d->themeManager->setTheme(ThemeManager::ThemeType::Dark);
                qDebug() << "[Application] 已恢复主题: Dark";
            }
        }
        qDebug() << "[Application] 编辑器字体已从配置加载:"
                 << fontFamily << clampedSize << "pt";
    }

    // ============================================================
    //  初始化语言（放在 ConfigManager / 主题之后，MainWindow 之前）
    //  - ConfigManager 已就绪，可从 General/Language 读取配置
    //  - .qm 不存在时 LanguageManager 内部会静默 fallback 到中文硬编码
    // ============================================================
    SprintIDE::LanguageManager::instance().applyFromConfig();
    qDebug() << "[Application] 已应用语言:"
             << SprintIDE::LanguageManager::instance().currentLanguage();

    emit initialized();
    qDebug() << "Application initialized successfully";
    return true;
}

void Application::shutdown()
{
    if (!d->initialized) {
        return;
    }

    d->state = AppState::ShuttingDown;
    qDebug() << "Shutting down Application";

    // 清理资源
    if (d->logBuffer) {
        delete d->logBuffer;
        d->logBuffer = nullptr;
        qDebug() << "[Application] LogBuffer 已释放";
    }

    // 清理资源
    if (d->configManager) {
        delete d->configManager;
        d->configManager = nullptr;
    }

    d->initialized = false;
    d->state = AppState::Starting;

    emit aboutToShutdown();
    qDebug() << "Application shutdown complete";
}

ConfigManager* Application::configManager() const
{
    return d->configManager;
}

LogBuffer* Application::logBuffer() const
{
    return d->logBuffer;
}

Application::AppState Application::currentState() const
{
    return d->state;
}

ThemeManager* Application::themeManager() const
{
    return d->themeManager;
}

QString Application::version() const
{
    return "1.0.0-alpha";
}

QString Application::buildInfo() const
{
    return QString("SprintIDE %1, built on %2")
    .arg(version())
        .arg(__DATE__);
}

QDateTime Application::startTime() const
{
    return d->startTime;
}

// 其他方法的简化实现...

PluginManager* Application::pluginManager() const
{
    return nullptr; // 暂未实现
}

MainWindow* Application::mainWindow() const
{
    return nullptr; // 暂未实现
}

bool Application::restart()
{
    qDebug() << "Restart requested";
    // 暂未实现真正的重启
    return false;
}

void Application::postEvent(const QString &type, const QVariant &data)
{
    qDebug() << "Event posted:" << type << data;
    emit eventPosted(type, data);
}

// 其他方法暂时留空...

void Application::subscribeEvent(const QString &type, QObject *receiver, const char *slot)
{
    Q_UNUSED(type)
    Q_UNUSED(receiver)
    Q_UNUSED(slot)
    // 暂未实现
}

void Application::unsubscribeEvent(const QString &type, QObject *receiver)
{
    Q_UNUSED(type)
    Q_UNUSED(receiver)
    // 暂未实现
}

void Application::registerResource(const QString &name, QObject *resource)
{
    Q_UNUSED(name)
    Q_UNUSED(resource)
    // 暂未实现
}

QObject* Application::getResource(const QString &name) const
{
    Q_UNUSED(name)
    return nullptr; // 暂未实现
}

void Application::releaseResource(const QString &name)
{
    Q_UNUSED(name)
    // 暂未实现
}

bool Application::saveSession()
{
    return false; // 暂未实现
}

bool Application::restoreSession()
{
    return false; // 暂未实现
}

bool Application::hasUnsavedSession() const
{
    return false; // 暂未实现
}

void Application::setLanguage(const QString &languageCode)
{
    // 转发给 LanguageManager，同时把选择持久化到配置
    SprintIDE::LanguageManager::instance().applyLanguage(languageCode);

    if (d->configManager) {
        d->configManager->setValue("General/Language", languageCode);
    }
}

QString Application::currentLanguage() const
{
    return SprintIDE::LanguageManager::instance().currentLanguage();
}

QStringList Application::availableLanguages() const
{
    QStringList codes;
    for (const auto& info : SprintIDE::LanguageManager::availableLanguages()) {
        codes.append(info.code);
    }
    return codes;
}

} // namespace SprintIDE
