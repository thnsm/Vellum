#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * NotesWidget
 * ───────────
 * Aba modular do Bloco de Notas & Anotações de Estudo com formatação Rich Text.
 */
class NotesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NotesWidget(QWidget *parent = nullptr);

    QTextEdit* txtAnotacoes() const { return m_txtAnotacoes; }
    void setText(const QString &texto);
    void appendQuickNote(const QString &texto);

signals:
    void logMessage(const QString &mensagem, const QString &tipo);

private slots:
    void onSalvarAnotacoes();
    void onAbrirAnotacoes();
    void onLimparAnotacoes();
    void onCopiarAnotacoes();
    void onMudarFonteAnotacoes(const QString &familia);
    void onMudarTamanhoAnotacoes(int pt);
    void onAumentarFonteAnotacoes();
    void onDiminuirFonteAnotacoes();
    void onToggleBoldAnotacoes();
    void onToggleItalicAnotacoes();
    void onToggleUnderlineAnotacoes();
    void onEscolherCorAnotacoes();
    void onAtualizarContadorNotas();

private:
    void setupUi();

    QTextEdit   *m_txtAnotacoes      = nullptr;
    QPushButton *m_btnNovoNota       = nullptr;
    QPushButton *m_btnAbrirNota      = nullptr;
    QPushButton *m_btnSalvarNota     = nullptr;
    QPushButton *m_btnCopiarNota     = nullptr;
    QComboBox   *m_comboFonteNota    = nullptr;
    QSpinBox    *m_spinTamanhoNota   = nullptr;
    QPushButton *m_btnFonteMaisNota  = nullptr;
    QPushButton *m_btnFonteMenosNota = nullptr;
    QPushButton *m_btnBoldNota       = nullptr;
    QPushButton *m_btnItalicNota     = nullptr;
    QPushButton *m_btnUnderlineNota  = nullptr;
    QPushButton *m_btnCorNota        = nullptr;
    QLabel      *m_lblStatusNota     = nullptr;
    QLabel      *m_lblArquivoNota    = nullptr;
    QString      m_caminhoArquivoNota;
};
