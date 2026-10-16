#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * SettingsWidget
 * ──────────────
 * Aba modular de Desempenho, Perfil de Hardware, Layout e Log de Atividades.
 */
class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

    QComboBox* comboPerfil() const { return m_comboPerfil; }
    QPushButton* btnSalvarLayout() const { return m_btnSalvarLayout; }
    QPushButton* btnRestaurarLayout() const { return m_btnRestaurarLayout; }
    QTextEdit* logBox() const { return m_logBox; }
    QLabel* lblStatus() const { return m_lblStatus; }

    void setStatusText(const QString &texto);
    void appendLogEntry(const QString &timestamp, const QString &corHex, const QString &mensagem);

signals:
    void perfilHardwareChanged(int index);
    void salvarLayoutRequested();
    void restaurarLayoutRequested();
    void aplicarProporcaoRequested(int leitorPercent);

private:
    void setupUi();

    QComboBox   *m_comboPerfil        = nullptr;
    QPushButton *m_btnSalvarLayout    = nullptr;
    QPushButton *m_btnRestaurarLayout = nullptr;
    QPushButton *m_btnProporcao100    = nullptr;
    QPushButton *m_btnProporcao70     = nullptr;
    QPushButton *m_btnProporcao50     = nullptr;
    QTextEdit   *m_logBox             = nullptr;
    QLabel      *m_lblStatus          = nullptr;
};
