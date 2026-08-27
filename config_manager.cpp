#include "config_manager.h"

#include <QSettings>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

namespace SprintIDE {

class ConfigManager::Private
{
public:
    QScopedPointer<QSettings> settings;
};

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    qDebug() << "ConfigManager constructor";
}

ConfigManager::~ConfigManager()
{
    qDebug() << "ConfigManager destructor";
    // 自动清理d指针
}

bool ConfigManager::initialize()
{
    qDebug() << "Initializing ConfigManager";

    // 设置INI格式
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // 创建配置目录
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir configDir(configPath);
    if (!configDir.exists() && !configDir.mkpath(".")) {
        qWarning() << "Failed to create config directory:" << configPath;
        return false;
    }

    // 初始化QSettings
    QString configFile = configDir.filePath("sprintide.ini");
    d->settings.reset(new QSettings(configFile, QSettings::IniFormat));

    qDebug() << "Config file:" << configFile;

    // 设置默认值
    if (!d->settings->contains("General/FirstRun")) {
        d->settings->setValue("General/FirstRun", true);
        d->settings->setValue("General/Version", "1.0.0");
        d->settings->setValue("Editor/UseSpaces", true);
        d->settings->setValue("Editor/TabWidth", 4);
        d->settings->setValue("Editor/FontFamily", "Monaco");
        d->settings->setValue("Editor/FontSize", 12);
        d->settings->sync();
    }

    return true;
}

// 其他方法的简化实现...

void ConfigManager::setValue(const QString &key, const QVariant &value)
{
    if (d->settings) {
        d->settings->setValue(key, value);
        d->settings->sync();
        emit configChanged(key, value);
    }
}

QVariant ConfigManager::value(const QString &key, const QVariant &defaultValue) const
{
    if (d->settings) {
        return d->settings->value(key, defaultValue);
    }
    return defaultValue;
}

// 暂时实现其他必需的方法...

bool ConfigManager::contains(const QString &key) const
{
    if (d->settings) {
        return d->settings->contains(key);
    }
    return false;
}

void ConfigManager::remove(const QString &key)
{
    if (d->settings) {
        d->settings->remove(key);
        d->settings->sync();
    }
}

void ConfigManager::beginGroup(const QString &prefix)
{
    if (d->settings) {
        d->settings->beginGroup(prefix);
    }
}

void ConfigManager::endGroup()
{
    if (d->settings) {
        d->settings->endGroup();
    }
}

QString ConfigManager::group() const
{
    if (d->settings) {
        return d->settings->group();
    }
    return QString();
}

} // namespace SprintIDE
