#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>

class PromptInputWidget;

/**
 * AiAssistantWidget
 * ─────────────────
 * Aba modular do Assistente Técnico IA & Chat Multimodal (com renderização Markdown + LaTeX + Tabelas).
 */
class AiAssistantWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AiAssistantWidget(QWidget *parent = nullptr);

    QComboBox* comboChatIA() const { return m_comboChatIA; }
    QLabel* lblModeloAtivoChat() const { return m_lblModeloAtivoChat; }
    QPushButton* btnConfigurarIA() const { return m_btnConfigurarIA; }
    QPushButton* btnLimparChat() const { return m_btnLimparChat; }
    QTextBrowser* chatHistorico() const { return m_chatHistorico; }
    PromptInputWidget* promptInput() const { return m_promptInput; }

    void resetarHistoricoVisual();
    void adicionarMensagemUsuario(const QString &texto, bool temImagem);
    void adicionarMensagemAssistente(const QString &resposta, const QString &provedor, const QString &modelo);
    void adicionarMensagemErro(const QString &mensagemErro);

    static QString renderizarMarkdownELatex(const QString &texto);

signals:
    void provedorIaChanged(int index);
    void configurarIaRequested();
    void limparChatRequested();
    void enviarChatRequested();
    void capturarCircuitoRequested();
    void anexarImagemRequested();
    void removerImagemRequested();
    void colarTrechoRequested();

private:
    void setupUi();
    static QString converterFormulaLatex(QString formula, bool isBlock);
    static QString converterTabelaMarkdown(const QString &tableBlock);

    QComboBox         *m_comboChatIA        = nullptr;
    QLabel            *m_lblModeloAtivoChat = nullptr;
    QPushButton       *m_btnConfigurarIA    = nullptr;
    QPushButton       *m_btnLimparChat      = nullptr;
    QTextBrowser      *m_chatHistorico      = nullptr;
    PromptInputWidget *m_promptInput        = nullptr;
};
