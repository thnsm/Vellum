#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QProgressBar>
#include <QLineEdit>
#include <QTextEdit>
#include <QTextBrowser>
#include <QSizeGrip>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QStringList>

class QuickLookupPopover : public QWidget {
    Q_OBJECT
public:
    enum class Modo {
        Traducao = 0,
        Tutor = 1,
        AssistenteIA = 2,
        BlocoNotas = 3
    };

    enum class BordaRedimensionamento {
        Nenhuma,
        Topo,
        Base,
        Esquerda,
        Direita,
        TopoEsquerda,
        TopoDireita,
        BaseEsquerda,
        BaseDireita
    };

    explicit QuickLookupPopover(QWidget *parent = nullptr);

    void setModo(Modo modo);
    Modo modoAtual() const { return m_modoAtual; }

    // Exibições individuais por função
    void exibirTraducao(const QString &textoEn, const QString &textoPt, const QString &modelo, const QPoint &posGlobal);
    void exibirTutor(const QString &textoEsperado, const QPoint &posGlobal);
    void exibirAssistenteIA(const QString &textoContexto, const QString &imagemBase64, const QPoint &posGlobal);
    void exibirAnotacoes(const QString &textoInicial, const QPoint &posGlobal);

    // Atualizações assíncronas do backend
    void atualizarResultadoTraducao(const QString &traducaoPt, const QString &modelo);
    void atualizarResultadoTutor(int nota, const QString &feedback, const QString &textoFalado, const QStringList &ausentes);
    void atualizarRespostaChat(const QString &resposta, const QString &modelo);
    void setGravandoTutor(bool gravando);

signals:
    void solicitarRetraducao(const QString &novoTextoEn);
    void ouvirPronunciaClicada(const QString &texto);
    void gravarVozTutorClicada(const QString &textoEsperado);
    void enviarMensagemChatClicada(const QString &pergunta, const QString &imagemBase64);
    void salvarNotaNoPainelClicada(const QString &texto);
    void expandirNoPainelClicado(int modoIndex, const QString &dado1, const QString &dado2);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void setupUi();
    void posicionarNaTela(const QPoint &posGlobal);
    BordaRedimensionamento detectarBorda(const QPoint &pos) const;
    void atualizarCursorBorda(BordaRedimensionamento borda);

    Modo             m_modoAtual = Modo::Traducao;

    // Movimentação & Redimensionamento
    bool                     m_dragging = false;
    QPoint                   m_dragPos;
    bool                     m_resizing = false;
    BordaRedimensionamento   m_bordaAtiva = BordaRedimensionamento::Nenhuma;
    QRect                    m_resizeStartGeom;
    QPoint                   m_resizeStartPos;

    // Header compartilhado arrastável
    QWidget         *m_headerBar         = nullptr;
    QLabel          *m_lblHeaderTitulo   = nullptr;
    QLabel          *m_lblHeaderBadge    = nullptr;
    QPushButton     *m_btnHeaderExpandir = nullptr;
    QPushButton     *m_btnHeaderFechar   = nullptr;

    QStackedWidget  *m_stackModos        = nullptr;

    // ── Página 0: Tradução (Texto em inglês editável para retradução dinâmica)
    QWidget         *m_pagTraducao       = nullptr;
    QTextEdit       *m_txtTradOriginal   = nullptr;
    QTextBrowser    *m_txtTradTraduzido  = nullptr;
    QPushButton     *m_btnTradAtualizar  = nullptr;
    QPushButton     *m_btnTradOuvir      = nullptr;
    QPushButton     *m_btnTradCopiar     = nullptr;
    QTimer          *m_timerDebounceTraducao = nullptr;
    bool             m_bloquearDebounce  = false;
    QString          m_ultimoTextoEnSolicitado;
    QString          m_textoTradEn;
    QString          m_textoTradPt;

    // ── Página 1: Tutor de Fala ───────────────────────────────────────────
    QWidget         *m_pagTutor          = nullptr;
    QLabel          *m_lblTutorTexto     = nullptr;
    QPushButton     *m_btnTutorOuvir     = nullptr;
    QPushButton     *m_btnTutorGravar    = nullptr;
    QProgressBar    *m_barTutorAcuracia  = nullptr;
    QLabel          *m_lblTutorNota      = nullptr;
    QLabel          *m_lblTutorFeedback  = nullptr;
    bool             m_tutorGravando     = false;

    // ── Página 2: Assistente IA ───────────────────────────────────────────
    QWidget         *m_pagChat           = nullptr;
    QLabel          *m_lblChatThumb      = nullptr;
    QTextBrowser    *m_txtChatHistorico  = nullptr;
    QLineEdit       *m_inputChatPergunta = nullptr;
    QPushButton     *m_btnChatEnviar     = nullptr;
    QString          m_chatImagemB64;

    // ── Página 3: Bloco de Notas ──────────────────────────────────────────
    QWidget         *m_pagNotas          = nullptr;
    QTextEdit       *m_txtNotas          = nullptr;
    QPushButton     *m_btnNotasCopiar    = nullptr;
    QPushButton     *m_btnNotasSalvar    = nullptr;
    QPushButton     *m_btnNotasLimpar    = nullptr;
};
