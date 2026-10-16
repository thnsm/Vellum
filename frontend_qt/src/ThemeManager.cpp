#include "ThemeManager.h"
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QDebug>

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {
}

bool ThemeManager::applyTheme(Theme theme) {
    m_currentTheme = theme;
    if (loadQssFile(":/themes/dark.qss")) {
        emit themeChanged(m_currentTheme);
        return true;
    }
    return false;
}

ThemeManager::Theme ThemeManager::currentTheme() const {
    return m_currentTheme;
}

bool ThemeManager::loadQssFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning() << "Falha ao abrir arquivo QSS:" << filePath;
        return false;
    }
    QTextStream stream(&file);
    qApp->setStyleSheet(stream.readAll());
    file.close();
    return true;
}
