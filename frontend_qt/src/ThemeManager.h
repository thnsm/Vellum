#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QString>
#include <QObject>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    enum class Theme {
        Dark
    };

    explicit ThemeManager(QObject* parent = nullptr);

    // Carrega o tema escuro padrão do Vellum
    bool applyTheme(Theme theme = Theme::Dark);

    // Retorna o tema atual
    Theme currentTheme() const;

signals:
    void themeChanged(Theme newTheme);

private:
    Theme m_currentTheme = Theme::Dark;
    bool loadQssFile(const QString& filePath);
};

#endif // THEMEMANAGER_H
