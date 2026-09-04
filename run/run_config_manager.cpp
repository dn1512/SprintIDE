#include "run_config_manager.h"
#include "../path_utils.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

namespace SprintIDE {

RunConfigManager& RunConfigManager::instance() {
    static RunConfigManager instance;
    return instance;
}

bool RunConfigManager::loadConfigs(const QString& projectPath) {
    m_currentProjectPath = projectPath;
    QString configFile = PathUtils::joinPaths({projectPath, ".SprintIDE", "run_configs.json"});
    if (!QFile::exists(configFile)) {
        m_configs.clear();
        return true; // 没有配置文件，视为空
    }
    QFile file(configFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open run configs file:" << file.errorString();
        return false;
    }
    QByteArray data = file.readAll();
    file.close();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) return false;
    QJsonArray arr = doc.array();
    m_configs.clear();
    for (const QJsonValue& val : arr) {
        QJsonObject obj = val.toObject();
        RunConfiguration cfg;
        cfg.name = obj["name"].toString();
        cfg.mainClass = obj["mainClass"].toString();
        cfg.workingDir = obj["workingDir"].toString();
        cfg.jvmArgs = obj["jvmArgs"].toVariant().toStringList();
        cfg.programArgs = obj["programArgs"].toVariant().toStringList();
        cfg.isFatJar = obj["isFatJar"].toBool(false);
        cfg.fatJarPath = obj["fatJarPath"].toString();
        cfg.debugPort = obj["debugPort"].toInt(5005);
        cfg.enableDebug = obj["enableDebug"].toBool(false);
        // 环境变量 map
        QVariantMap envMap = obj["environmentVariables"].toObject().toVariantMap();
        cfg.environmentVariables = envMap;
        m_configs.append(cfg);
    }
    return true;
}

bool RunConfigManager::saveConfigs(const QString& projectPath) const {
    QDir dir(PathUtils::joinPaths({projectPath, ".SprintIDE"}));
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "Cannot create .SprintIDE directory";
        return false;
    }
    QString configFile = dir.absoluteFilePath("run_configs.json");
    QJsonArray arr;
    for (const RunConfiguration& cfg : m_configs) {
        QJsonObject obj;
        obj["name"] = cfg.name;
        obj["mainClass"] = cfg.mainClass;
        obj["workingDir"] = cfg.workingDir;
        obj["jvmArgs"] = QJsonArray::fromStringList(cfg.jvmArgs);
        obj["programArgs"] = QJsonArray::fromStringList(cfg.programArgs);
        obj["isFatJar"] = cfg.isFatJar;
        obj["fatJarPath"] = cfg.fatJarPath;
        obj["debugPort"] = cfg.debugPort;
        obj["enableDebug"] = cfg.enableDebug;
        // 环境变量
        QJsonObject envObj;
        for (auto it = cfg.environmentVariables.begin(); it != cfg.environmentVariables.end(); ++it) {
            envObj[it.key()] = it.value().toString();
        }
        obj["environmentVariables"] = envObj;
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    QFile file(configFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot write run configs file:" << file.errorString();
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void RunConfigManager::setConfigs(const QList<RunConfiguration>& configs) {
    m_configs = configs;
}

void RunConfigManager::addConfig(const RunConfiguration& config) {
    m_configs.append(config);
}

void RunConfigManager::removeConfig(const QString& name) {
    m_configs.removeIf([&](const RunConfiguration& c){ return c.name == name; });
}

RunConfiguration* RunConfigManager::findConfig(const QString& name) {
    for (RunConfiguration& c : m_configs) {
        if (c.name == name) return &c;
    }
    return nullptr;
}

} // namespace SprintIDE
