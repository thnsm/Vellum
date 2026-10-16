#pragma once

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>

class PromptInputWidget;

/**
 * TranslationWidget
 * ─────────────────
 * Aba modular de Tradução Direta e Retradução Dinâmica (Inglês -> Português Técnico).
 */
class TranslationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TranslationWidget(QWidget *parent = nullptr);

    QPushButton* btnModoCaptura() const { return m_btnModoCaptura; }
    QComboBox* comboFormatoSelecao() const { return m_comboFormatoSelecao; }
    QPushButton* btnTraduzir() const { return m_btnTraduzir; }
    QCheckBox* chkTraducaoAuto() const { return m_chkTraducaoAuto; }
    QTextEdit* txtTraducaoOriginalEn() const { return m_txtTraducaoOriginalEn; }
    QPushButton* btnRetraduzirTexto() const { return m_btnRetraduzirTexto; }
    PromptInputWidget* promptTraducao() const { return m_promptTraducao; }
    QPushButton* btnCopiarTraducao() const { return m_btnCopiarTraducao; }

    void exibirResultado(const QString &textoIngles, const QString &traducaoPortugues, const QString &modeloUsado);
    void setOriginalTextSilent(const QString &textoIngles);

signals:
    void toggleModoCapturaRequested(bool ativo);
    void formatoSelecaoChanged(int idx);
    void traduzirSelecaoRequested();
    void retraduzirTextoRequested(const QString &novoTextoIngles);
    void copiarTraducaoRequested();

private:
    void setupUi();

    QPushButton       *m_btnModoCaptura          = nullptr;
    QComboBox         *m_comboFormatoSelecao     = nullptr;
    QPushButton       *m_btnTraduzir             = nullptr;
    QCheckBox         *m_chkTraducaoAuto         = nullptr;
    QTextEdit         *m_txtTraducaoOriginalEn   = nullptr;
    QPushButton       *m_btnRetraduzirTexto      = nullptr;
    QTimer            *m_timerDebounceTraducao   = nullptr;
    QString            m_ultimoTextoSolicitado;
    PromptInputWidget *m_promptTraducao          = nullptr;
    QPushButton       *m_btnCopiarTraducao       = nullptr;
};
