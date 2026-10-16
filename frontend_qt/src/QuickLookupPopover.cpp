#include "QuickLookupPopover.h"
#include "AiAssistantWidget.h"
#include <QScreen>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QClipboard>
#include <QApplication>
#include <QBuffer>
#include <QPixmap>

QuickLookupPopover::QuickLookupPopover(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setObjectName("quickLookupPopover");
    setAttribute(Qt::WA_ShowWithoutActivating, false);
    setMouseTracking(true);
    setMinimumSize(380, 260);
    setMaximumSize(920, 740);
    resize(460, 320);

    setupUi();
    hide();
}

void QuickLookupPopover::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    // ── Cabeçalho Draggable Compartilhado ─────────────────────────────────
    m_headerBar = new QWidget(this);
    m_headerBar->setObjectName("popoverHeaderBar");
    m_headerBar->setCursor(Qt::SizeAllCursor);

    auto *headerLayout = new QHBoxLayout(m_headerBar);
    headerLayout->setContentsMargins(8, 4, 8, 4);
    headerLayout->setSpacing(8);

    m_lblHeaderTitulo = new QLabel("CONSULTA RÁPIDA", m_headerBar);
    m_lblHeaderTitulo->setObjectName("lblPopoverHeader");
    m_lblHeaderTitulo->setCursor(Qt::SizeAllCursor);

    m_lblHeaderBadge = new QLabel("IA", m_headerBar);
    m_lblHeaderBadge->setObjectName("badgePopoverModelo");
    m_lblHeaderBadge->setCursor(Qt::SizeAllCursor);

    m_btnHeaderExpandir = new QPushButton("Abrir Painel", m_headerBar);
    m_btnHeaderExpandir->setObjectName("btnPopoverExpandir");
    m_btnHeaderExpandir->setToolTip("Transferir conteúdo para o painel lateral completo");
    m_btnHeaderExpandir->setCursor(Qt::PointingHandCursor);

    m_btnHeaderFechar = new QPushButton("×", m_headerBar);
    m_btnHeaderFechar->setObjectName("btnPopoverFechar");
    m_btnHeaderFechar->setToolTip("Fechar janela flutuante (Esc)");
    m_btnHeaderFechar->setCursor(Qt::PointingHandCursor);

    headerLayout->addWidget(m_lblHeaderTitulo);
    headerLayout->addWidget(m_lblHeaderBadge);
    headerLayout->addStretch();
    headerLayout->addWidget(m_btnHeaderExpandir);
    headerLayout->addWidget(m_btnHeaderFechar);
    rootLayout->addWidget(m_headerBar);

    // Registra filtros de eventos no cabeçalho para movimentação direta
    m_headerBar->installEventFilter(this);
    m_lblHeaderTitulo->installEventFilter(this);
    m_lblHeaderBadge->installEventFilter(this);

    // ── Stack de Modos ────────────────────────────────────────────────────
    m_stackModos = new QStackedWidget(this);
    m_stackModos->setObjectName("popoverStack");

    // ── 0. Página Tradução (Com texto em inglês editável para retradução) ──
    m_pagTraducao = new QWidget(m_stackModos);
    auto *tradLayout = new QVBoxLayout(m_pagTraducao);
    tradLayout->setContentsMargins(4, 4, 4, 4);
    tradLayout->setSpacing(6);

    auto *lblDicaOriginal = new QLabel("Texto Original em Inglês (Editável):", m_pagTraducao);
    lblDicaOriginal->setObjectName("lblPopoverSubtitulo");
    tradLayout->addWidget(lblDicaOriginal);

    m_txtTradOriginal = new QTextEdit(m_pagTraducao);
    m_txtTradOriginal->setObjectName("txtPopoverOriginal");
    m_txtTradOriginal->setPlaceholderText("Texto original em inglês (adicione ou remova palavras para retraduzir)...");
    m_txtTradOriginal->setFixedHeight(65);
    tradLayout->addWidget(m_txtTradOriginal);

    auto *lblDicaTraduzido = new QLabel("Tradução Técnica (PT-BR):", m_pagTraducao);
    lblDicaTraduzido->setObjectName("lblPopoverSubtitulo");
    tradLayout->addWidget(lblDicaTraduzido);

    m_txtTradTraduzido = new QTextBrowser(m_pagTraducao);
    m_txtTradTraduzido->setObjectName("txtPopoverTraducao");
    m_txtTradTraduzido->setOpenExternalLinks(false);
    m_txtTradTraduzido->setPlaceholderText("A tradução em português aparecerá aqui...");
    tradLayout->addWidget(m_txtTradTraduzido, 1);

    auto *tradActions = new QHBoxLayout();
    tradActions->setSpacing(6);

    m_btnTradAtualizar = new QPushButton("Traduzir", m_pagTraducao);
    m_btnTradAtualizar->setObjectName("btnPopoverPrimario");
    m_btnTradAtualizar->setToolTip("Traduzir o texto em inglês editado acima");
    m_btnTradAtualizar->setCursor(Qt::PointingHandCursor);

    m_btnTradOuvir = new QPushButton("Ouvir Pronúncia", m_pagTraducao);
    m_btnTradOuvir->setObjectName("btnPopoverOuvir");
    m_btnTradOuvir->setCursor(Qt::PointingHandCursor);

    m_btnTradParar = new QPushButton("Parar", m_pagTraducao);
    m_btnTradParar->setObjectName("btnPopoverAcao");
    m_btnTradParar->setToolTip("Interromper reprodução de áudio");
    m_btnTradParar->setCursor(Qt::PointingHandCursor);

    m_btnTradCopiar = new QPushButton("Copiar Tradução", m_pagTraducao);
    m_btnTradCopiar->setObjectName("btnPopoverAcao");
    m_btnTradCopiar->setCursor(Qt::PointingHandCursor);

    tradActions->addWidget(m_btnTradAtualizar);
    tradActions->addWidget(m_btnTradOuvir);
    tradActions->addWidget(m_btnTradParar);
    tradActions->addWidget(m_btnTradCopiar);
    tradActions->addStretch();
    tradLayout->addLayout(tradActions);

    m_stackModos->addWidget(m_pagTraducao);

    // Timer de debounce para retradução ao editar palavras
    m_timerDebounceTraducao = new QTimer(this);
    m_timerDebounceTraducao->setSingleShot(true);
    m_timerDebounceTraducao->setInterval(600);

    connect(m_txtTradOriginal, &QTextEdit::textChanged, this, [this]() {
        if (!m_bloquearDebounce) {
            m_timerDebounceTraducao->start(600);
        }
    });

    auto dispararRetraducao = [this]() {
        const QString novoTexto = m_txtTradOriginal->toPlainText().trimmed();
        if (!novoTexto.isEmpty() && novoTexto != m_ultimoTextoEnSolicitado) {
            m_ultimoTextoEnSolicitado = novoTexto;
            m_textoTradEn = novoTexto;
            m_txtTradTraduzido->setHtml("<span style='color: #8B949E; font-style: italic;'>Traduzindo trecho modificado...</span>");
            emit solicitarRetraducao(novoTexto);
        }
    };

    connect(m_timerDebounceTraducao, &QTimer::timeout, this, dispararRetraducao);
    connect(m_btnTradAtualizar, &QPushButton::clicked, this, dispararRetraducao);

    // ── 1. Página Tutor de Fala ───────────────────────────────────────────
    m_pagTutor = new QWidget(m_stackModos);
    auto *tutorLayout = new QVBoxLayout(m_pagTutor);
    tutorLayout->setContentsMargins(4, 4, 4, 4);
    tutorLayout->setSpacing(6);

    m_lblTutorTexto = new QLabel(m_pagTutor);
    m_lblTutorTexto->setObjectName("lblPopoverTutorTexto");
    m_lblTutorTexto->setWordWrap(true);
    tutorLayout->addWidget(m_lblTutorTexto);

    auto *tutorBtns = new QHBoxLayout();
    tutorBtns->setSpacing(6);

    m_btnTutorOuvir = new QPushButton("Ouvir Referência", m_pagTutor);
    m_btnTutorOuvir->setObjectName("btnPopoverOuvir");
    m_btnTutorOuvir->setCursor(Qt::PointingHandCursor);

    m_btnTutorParar = new QPushButton("Parar", m_pagTutor);
    m_btnTutorParar->setObjectName("btnPopoverAcao");
    m_btnTutorParar->setToolTip("Interromper reprodução de áudio");
    m_btnTutorParar->setCursor(Qt::PointingHandCursor);

    m_btnTutorGravar = new QPushButton("Gravar Minha Voz", m_pagTutor);
    m_btnTutorGravar->setObjectName("btnPopoverGravar");
    m_btnTutorGravar->setCursor(Qt::PointingHandCursor);

    tutorBtns->addWidget(m_btnTutorOuvir);
    tutorBtns->addWidget(m_btnTutorParar);
    tutorBtns->addWidget(m_btnTutorGravar);
    tutorBtns->addStretch();
    tutorLayout->addLayout(tutorBtns);

    auto *tutorBarLayout = new QHBoxLayout();
    tutorBarLayout->setSpacing(8);
    m_barTutorAcuracia = new QProgressBar(m_pagTutor);
    m_barTutorAcuracia->setObjectName("popoverAcuraciaBar");
    m_barTutorAcuracia->setRange(0, 100);
    m_barTutorAcuracia->setValue(0);
    m_barTutorAcuracia->setTextVisible(false);
    m_barTutorAcuracia->setFixedHeight(8);

    m_lblTutorNota = new QLabel("--%", m_pagTutor);
    m_lblTutorNota->setObjectName("lblPopoverTutorNota");

    tutorBarLayout->addWidget(m_barTutorAcuracia, 1);
    tutorBarLayout->addWidget(m_lblTutorNota);
    tutorLayout->addLayout(tutorBarLayout);

    m_lblTutorFeedback = new QLabel("Clique em 'Gravar Minha Voz' para pronunciar o texto.", m_pagTutor);
    m_lblTutorFeedback->setObjectName("lblPopoverTutorFeedback");
    m_lblTutorFeedback->setWordWrap(true);
    tutorLayout->addWidget(m_lblTutorFeedback);

    m_stackModos->addWidget(m_pagTutor);

    // ── 2. Página Assistente IA ───────────────────────────────────────────
    m_pagChat = new QWidget(m_stackModos);
    auto *chatLayout = new QVBoxLayout(m_pagChat);
    chatLayout->setContentsMargins(4, 4, 4, 4);
    chatLayout->setSpacing(6);

    m_lblChatThumb = new QLabel(m_pagChat);
    m_lblChatThumb->setObjectName("lblPopoverChatThumb");
    m_lblChatThumb->setFixedHeight(60);
    m_lblChatThumb->setScaledContents(false);
    m_lblChatThumb->setAlignment(Qt::AlignCenter);
    m_lblChatThumb->hide();
    chatLayout->addWidget(m_lblChatThumb);

    m_txtChatHistorico = new QTextBrowser(m_pagChat);
    m_txtChatHistorico->setObjectName("txtPopoverChat");
    m_txtChatHistorico->setOpenExternalLinks(true);
    m_txtChatHistorico->setPlaceholderText("A resposta da IA aparecerá aqui...");
    chatLayout->addWidget(m_txtChatHistorico, 1);

    auto *chatInputLayout = new QHBoxLayout();
    chatInputLayout->setSpacing(6);
    m_inputChatPergunta = new QLineEdit(m_pagChat);
    m_inputChatPergunta->setObjectName("inputPopoverChat");
    m_inputChatPergunta->setPlaceholderText("Faça uma pergunta sobre o trecho ou circuito...");

    m_btnChatEnviar = new QPushButton("Enviar", m_pagChat);
    m_btnChatEnviar->setObjectName("btnPopoverEnviar");
    m_btnChatEnviar->setCursor(Qt::PointingHandCursor);

    chatInputLayout->addWidget(m_inputChatPergunta, 1);
    chatInputLayout->addWidget(m_btnChatEnviar);
    chatLayout->addLayout(chatInputLayout);

    m_stackModos->addWidget(m_pagChat);

    // ── 3. Página Bloco de Notas ──────────────────────────────────────────
    m_pagNotas = new QWidget(m_stackModos);
    auto *notasLayout = new QVBoxLayout(m_pagNotas);
    notasLayout->setContentsMargins(4, 4, 4, 4);
    notasLayout->setSpacing(6);

    m_txtNotas = new QTextEdit(m_pagNotas);
    m_txtNotas->setObjectName("txtPopoverNotas");
    m_txtNotas->setPlaceholderText("Escreva ou edite sua anotação técnica aqui...");
    notasLayout->addWidget(m_txtNotas, 1);

    auto *notasActions = new QHBoxLayout();
    notasActions->setSpacing(6);

    m_btnNotasCopiar = new QPushButton("Copiar", m_pagNotas);
    m_btnNotasCopiar->setObjectName("btnPopoverAcao");
    m_btnNotasCopiar->setCursor(Qt::PointingHandCursor);

    m_btnNotasSalvar = new QPushButton("Salvar no Painel", m_pagNotas);
    m_btnNotasSalvar->setObjectName("btnPopoverAcao");
    m_btnNotasSalvar->setCursor(Qt::PointingHandCursor);

    m_btnNotasLimpar = new QPushButton("Limpar", m_pagNotas);
    m_btnNotasLimpar->setObjectName("btnPopoverAcao");
    m_btnNotasLimpar->setCursor(Qt::PointingHandCursor);

    notasActions->addWidget(m_btnNotasCopiar);
    notasActions->addWidget(m_btnNotasSalvar);
    notasActions->addWidget(m_btnNotasLimpar);
    notasActions->addStretch();
    notasLayout->addLayout(notasActions);

    m_stackModos->addWidget(m_pagNotas);

    rootLayout->addWidget(m_stackModos, 1);

    // ── Rodapé com Alça de Redimensionamento e Grips ───────────────────────
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->addStretch();

    m_sizeGrip = new QSizeGrip(this);
    m_sizeGrip->setObjectName("popoverSizeGrip");
    m_sizeGrip->setFixedSize(16, 16);
    m_sizeGrip->setCursor(Qt::SizeFDiagCursor);
    m_sizeGrip->installEventFilter(this);
    bottomLayout->addWidget(m_sizeGrip);
    rootLayout->addLayout(bottomLayout);

    // ── Conexões de Eventos ───────────────────────────────────────────────
    connect(m_btnHeaderFechar, &QPushButton::clicked, this, &QWidget::hide);

    connect(m_btnHeaderExpandir, &QPushButton::clicked, this, [this]() {
        hide();
        if (m_modoAtual == Modo::Traducao) {
            emit expandirNoPainelClicado(0, m_txtTradOriginal->toPlainText(), m_textoTradPt);
        } else if (m_modoAtual == Modo::Tutor) {
            emit expandirNoPainelClicado(1, m_textoTutorEsperado, QString());
        } else if (m_modoAtual == Modo::AssistenteIA) {
            emit expandirNoPainelClicado(2, m_inputChatPergunta->text(), m_chatImagemB64);
        } else {
            emit expandirNoPainelClicado(3, m_txtNotas->toPlainText(), QString());
        }
    });

    // Tradução
    connect(m_btnTradOuvir, &QPushButton::clicked, this, [this]() {
        emit ouvirPronunciaClicada(m_txtTradOriginal->toPlainText().trimmed());
    });
    connect(m_btnTradParar, &QPushButton::clicked, this, &QuickLookupPopover::pararAudioClicado);
    connect(m_btnTradCopiar, &QPushButton::clicked, this, [this]() {
        const QString pt = m_txtTradTraduzido->toPlainText().trimmed();
        if (!pt.isEmpty()) {
            QApplication::clipboard()->setText(pt);
        }
    });

    // Tutor
    connect(m_btnTutorOuvir, &QPushButton::clicked, this, [this]() {
        emit ouvirPronunciaClicada(m_textoTutorEsperado);
    });
    connect(m_btnTutorParar, &QPushButton::clicked, this, &QuickLookupPopover::pararAudioClicado);
    connect(m_btnTutorGravar, &QPushButton::clicked, this, [this]() {
        emit gravarVozTutorClicada(m_textoTutorEsperado);
    });

    // Chat
    auto enviarChat = [this]() {
        const QString msg = m_inputChatPergunta->text().trimmed();
        if (msg.isEmpty()) return;
        m_txtChatHistorico->append("<div style='margin: 4px 0;'><b style='color: #60a5fa;'>Você:</b> " + msg.toHtmlEscaped() + "</div>");
        m_inputChatPergunta->clear();
        m_btnChatEnviar->setEnabled(false);
        emit enviarMensagemChatClicada(msg, m_chatImagemB64);
    };
    connect(m_btnChatEnviar, &QPushButton::clicked, this, enviarChat);
    connect(m_inputChatPergunta, &QLineEdit::returnPressed, this, enviarChat);

    // Notas
    connect(m_btnNotasCopiar, &QPushButton::clicked, this, [this]() {
        const QString txt = m_txtNotas->toPlainText();
        if (!txt.isEmpty()) {
            QApplication::clipboard()->setText(txt);
        }
    });
    connect(m_btnNotasSalvar, &QPushButton::clicked, this, [this]() {
        const QString txt = m_txtNotas->toPlainText();
        emit salvarNotaNoPainelClicada(txt);
    });
    connect(m_btnNotasLimpar, &QPushButton::clicked, this, [this]() {
        m_txtNotas->clear();
    });
}

void QuickLookupPopover::setModo(Modo modo)
{
    m_modoAtual = modo;
    m_stackModos->setCurrentIndex(static_cast<int>(modo));

    switch (modo) {
    case Modo::Traducao:
        m_lblHeaderTitulo->setText("TRADUÇÃO TÉCNICA");
        break;
    case Modo::Tutor:
        m_lblHeaderTitulo->setText("TUTOR DE FALA");
        m_lblHeaderBadge->setText("VOZ");
        break;
    case Modo::AssistenteIA:
        m_lblHeaderTitulo->setText("ASSISTENTE IA");
        m_lblHeaderBadge->setText("CHAT");
        break;
    case Modo::BlocoNotas:
        m_lblHeaderTitulo->setText("BLOCO DE NOTAS");
        m_lblHeaderBadge->setText("NOTAS");
        break;
    }
}

QRect QuickLookupPopover::obterGeometriaTelaUtil() const
{
    QScreen *scr = screen();
    if (!scr && parentWidget()) {
        scr = parentWidget()->screen();
    }
    if (!scr) {
        scr = QGuiApplication::screenAt(geometry().center());
    }
    if (!scr) {
        scr = QGuiApplication::primaryScreen();
    }
    if (!scr) {
        return QRect(0, 0, 1920, 1080);
    }
    if (parentWidget() && parentWidget()->isFullScreen()) {
        return scr->geometry();
    }
    return scr->availableGeometry();
}

void QuickLookupPopover::aplicarGeometriaSegura(QRect novaGeom)
{
    const QRect tela = obterGeometriaTelaUtil();
    const int maxW = qBound(minimumWidth(), qMin(maximumWidth(), tela.width() - 24), 1200);
    const int maxH = qBound(minimumHeight(), qMin(maximumHeight(), tela.height() - 24), 900);

    const int w = qBound(minimumWidth(), novaGeom.width(), maxW);
    const int h = qBound(minimumHeight(), novaGeom.height(), maxH);

    int x = novaGeom.x();
    int y = novaGeom.y();

    // Mantém sempre dentro dos limites visíveis da tela (especialmente em tela cheia)
    if (x + w > tela.right() - 8) {
        x = tela.right() - w - 8;
    }
    if (x < tela.left() + 8) {
        x = tela.left() + 8;
    }
    if (y + h > tela.bottom() - 8) {
        y = tela.bottom() - h - 8;
    }
    if (y < tela.top() + 8) {
        y = tela.top() + 8;
    }

    setGeometry(x, y, w, h);
}

void QuickLookupPopover::posicionarNaTela(const QPoint &posGlobal)
{
    QScreen *scr = QGuiApplication::screenAt(posGlobal);
    if (!scr && parentWidget()) scr = parentWidget()->screen();
    if (!scr) scr = QGuiApplication::primaryScreen();

    const QRect screenGeom = scr
        ? ((parentWidget() && parentWidget()->isFullScreen()) ? scr->geometry() : scr->availableGeometry())
        : QRect(0, 0, 1920, 1080);

    // Garante tamanho dentro dos limites ao abrir
    const int maxW = qBound(minimumWidth(), qMin(maximumWidth(), screenGeom.width() - 24), 1200);
    const int maxH = qBound(minimumHeight(), qMin(maximumHeight(), screenGeom.height() - 24), 900);
    const int w = qBound(minimumWidth(), width(), maxW);
    const int h = qBound(minimumHeight(), height(), maxH);

    int posX = posGlobal.x();
    int posY = posGlobal.y() + 15;

    if (posX + w > screenGeom.right() - 10) {
        posX = screenGeom.right() - w - 10;
    }
    if (posY + h > screenGeom.bottom() - 10) {
        posY = posGlobal.y() - h - 15;
    }
    if (posX < screenGeom.left() + 10) {
        posX = screenGeom.left() + 10;
    }
    if (posY < screenGeom.top() + 10) {
        posY = screenGeom.top() + 10;
    }

    m_dragging = false;
    m_resizing = false;
    m_bordaAtiva = BordaRedimensionamento::Nenhuma;

    setGeometry(posX, posY, w, h);
    show();
    raise();
    activateWindow();
}

void QuickLookupPopover::exibirTraducao(const QString &textoEn, const QString &textoPt, const QString &modelo, const QPoint &posGlobal)
{
    m_textoTradEn = textoEn;
    m_textoTradPt = textoPt;
    m_ultimoTextoEnSolicitado = textoEn;

    setModo(Modo::Traducao);
    m_lblHeaderBadge->setText(modelo.isEmpty() ? "IA" : modelo);

    m_bloquearDebounce = true;
    m_txtTradOriginal->setPlainText(textoEn);
    m_bloquearDebounce = false;

    m_txtTradTraduzido->setPlainText(textoPt);

    posicionarNaTela(posGlobal);
}

void QuickLookupPopover::atualizarResultadoTraducao(const QString &traducaoPt, const QString &modelo)
{
    m_textoTradPt = traducaoPt;
    if (!modelo.isEmpty()) {
        m_lblHeaderBadge->setText(modelo);
    }
    m_txtTradTraduzido->setPlainText(traducaoPt);
}

void QuickLookupPopover::exibirTutor(const QString &textoEsperado, const QPoint &posGlobal)
{
    setModo(Modo::Tutor);
    m_textoTutorEsperado = textoEsperado;
    m_lblTutorTexto->setTextFormat(Qt::RichText);
    m_lblTutorTexto->setText(textoEsperado.toHtmlEscaped());
    m_barTutorAcuracia->setValue(0);
    m_lblTutorNota->setText("--%");
    m_lblTutorFeedback->setText("Clique em 'Gravar Minha Voz' para pronunciar o texto em voz alta.");
    m_btnTutorGravar->setText("Gravar Minha Voz");
    m_tutorGravando = false;

    posicionarNaTela(posGlobal);
}

void QuickLookupPopover::exibirAssistenteIA(const QString &textoContexto, const QString &imagemBase64, const QPoint &posGlobal)
{
    setModo(Modo::AssistenteIA);
    m_chatImagemB64 = imagemBase64;

    if (!imagemBase64.isEmpty()) {
        QPixmap pix;
        pix.loadFromData(QByteArray::fromBase64(imagemBase64.toLatin1()));
        if (!pix.isNull()) {
            m_lblChatThumb->setPixmap(pix.scaled(200, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            m_lblChatThumb->show();
        } else {
            m_lblChatThumb->hide();
        }
    } else {
        m_lblChatThumb->hide();
    }

    m_txtChatHistorico->clear();
    if (!textoContexto.isEmpty()) {
        m_txtChatHistorico->append("<div style='margin-bottom: 6px; color: #8B949E; font-size: 11px;'><i>Trecho selecionado:</i> " + textoContexto.left(180).toHtmlEscaped() + "...</div>");
    }
    m_inputChatPergunta->clear();
    m_btnChatEnviar->setEnabled(true);

    posicionarNaTela(posGlobal);
}

void QuickLookupPopover::exibirAnotacoes(const QString &textoInicial, const QPoint &posGlobal)
{
    setModo(Modo::BlocoNotas);
    if (!textoInicial.isEmpty()) {
        m_txtNotas->setPlainText(textoInicial);
    }

    posicionarNaTela(posGlobal);
}

void QuickLookupPopover::atualizarResultadoTutor(int nota, const QString &feedback, const QString &textoFalado, const QStringList &ausentes, const QString &htmlDestacado)
{
    Q_UNUSED(textoFalado);
    Q_UNUSED(ausentes);
    m_barTutorAcuracia->setValue(nota);
    m_lblTutorNota->setText(QString("%1%").arg(nota));

    if (!htmlDestacado.isEmpty()) {
        m_lblTutorTexto->setText(htmlDestacado);
    }

    const QString color = (nota >= 80) ? "#10b981" : (nota >= 60) ? "#f59e0b" : "#ef4444";
    m_lblTutorFeedback->setText(QString("<span style='color: %1; font-weight: 600;'>Nota %2%</span>: %3").arg(color).arg(nota).arg(feedback.toHtmlEscaped()));
    m_btnTutorGravar->setText("Gravar Novamente");
    m_tutorGravando = false;
}

void QuickLookupPopover::atualizarRespostaChat(const QString &resposta, const QString &modelo)
{
    m_lblHeaderBadge->setText(modelo.isEmpty() ? "IA" : modelo);
    const QString htmlFormatado = AiAssistantWidget::renderizarMarkdownELatex(resposta);
    m_txtChatHistorico->append("<div style='margin: 6px 0;'><b style='color: #388bfd;'>Assistente:</b><br>" + htmlFormatado + "</div>");
    m_btnChatEnviar->setEnabled(true);
}

void QuickLookupPopover::setGravandoTutor(bool gravando)
{
    m_tutorGravando = gravando;
    if (gravando) {
        m_btnTutorGravar->setText("Gravando... Fale agora");
    } else {
        m_btnTutorGravar->setText("Gravar Minha Voz");
    }
}

// ── Detecção e Aplicação de Bordas de Redimensionamento ────────────────────────
QuickLookupPopover::BordaRedimensionamento QuickLookupPopover::detectarBorda(const QPoint &pos) const
{
    const int margem = 8;
    const bool esq = pos.x() <= margem;
    const bool dir = pos.x() >= width() - margem;
    const bool top = pos.y() <= margem;
    const bool bot = pos.y() >= height() - margem;

    if (top && esq) return BordaRedimensionamento::TopoEsquerda;
    if (top && dir) return BordaRedimensionamento::TopoDireita;
    if (bot && esq) return BordaRedimensionamento::BaseEsquerda;
    if (bot && dir) return BordaRedimensionamento::BaseDireita;
    if (esq)        return BordaRedimensionamento::Esquerda;
    if (dir)        return BordaRedimensionamento::Direita;
    if (top)        return BordaRedimensionamento::Topo;
    if (bot)        return BordaRedimensionamento::Base;
    return BordaRedimensionamento::Nenhuma;
}

void QuickLookupPopover::atualizarCursorBorda(BordaRedimensionamento borda)
{
    switch (borda) {
    case BordaRedimensionamento::TopoEsquerda:
    case BordaRedimensionamento::BaseDireita:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case BordaRedimensionamento::TopoDireita:
    case BordaRedimensionamento::BaseEsquerda:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case BordaRedimensionamento::Esquerda:
    case BordaRedimensionamento::Direita:
        setCursor(Qt::SizeHorCursor);
        break;
    case BordaRedimensionamento::Topo:
    case BordaRedimensionamento::Base:
        setCursor(Qt::SizeVerCursor);
        break;
    case BordaRedimensionamento::Nenhuma:
    default:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

// ── Filtro de Eventos para Arrastar pelo Cabeçalho e Alça de Redimensionamento ──
bool QuickLookupPopover::eventFilter(QObject *watched, QEvent *event)
{
    // Intercepta o QSizeGrip para usar nosso redimensionamento seguro (com limites máximos)
    // em vez do comportamento nativo do QSizeGrip que pode redimensionar a MainWindow pai!
    if (watched == m_sizeGrip) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_resizing = true;
                m_dragging = false;
                m_bordaAtiva = BordaRedimensionamento::BaseDireita;
                m_resizeStartGeom = geometry();
                m_resizeStartPos = me->globalPosition().toPoint();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent*>(event);
            if (m_resizing && (me->buttons() & Qt::LeftButton)) {
                const QPoint delta = me->globalPosition().toPoint() - m_resizeStartPos;
                QRect newGeom = m_resizeStartGeom;
                newGeom.setWidth(m_resizeStartGeom.width() + delta.x());
                newGeom.setHeight(m_resizeStartGeom.height() + delta.y());
                aplicarGeometriaSegura(newGeom);
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_resizing = false;
                m_bordaAtiva = BordaRedimensionamento::Nenhuma;
                return true;
            }
            break;
        }
        default:
            break;
        }
    }

    if (watched == m_headerBar || watched == m_lblHeaderTitulo || watched == m_lblHeaderBadge) {
        switch (event->type()) {
        case QEvent::MouseButtonPress: {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                // Se clicou nos primeiros pixels do topo do cabeçalho, prioriza redimensionamento superior
                const QPoint posNoPopover = mapFromGlobal(me->globalPosition().toPoint());
                const BordaRedimensionamento borda = detectarBorda(posNoPopover);
                if (borda != BordaRedimensionamento::Nenhuma) {
                    m_resizing = true;
                    m_dragging = false;
                    m_bordaAtiva = borda;
                    m_resizeStartGeom = geometry();
                    m_resizeStartPos = me->globalPosition().toPoint();
                    return true;
                }
                m_dragging = true;
                m_resizing = false;
                m_dragStartGlobalPos = me->globalPosition().toPoint();
                m_dragStartWidgetPos = pos();
                raise();
                return true;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent*>(event);
            if (m_resizing && (me->buttons() & Qt::LeftButton)) {
                QMouseEvent mappedEvent(
                    QEvent::MouseMove,
                    mapFromGlobal(me->globalPosition().toPoint()),
                    me->globalPosition(),
                    me->button(),
                    me->buttons(),
                    me->modifiers()
                );
                mouseMoveEvent(&mappedEvent);
                return true;
            }
            if (m_dragging && (me->buttons() & Qt::LeftButton)) {
                const QPoint delta = me->globalPosition().toPoint() - m_dragStartGlobalPos;
                QRect novaGeom = geometry();
                novaGeom.moveTopLeft(m_dragStartWidgetPos + delta);
                aplicarGeometriaSegura(novaGeom);
                return true;
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_dragging = false;
                m_resizing = false;
                m_bordaAtiva = BordaRedimensionamento::Nenhuma;
                return true;
            }
            break;
        }
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ── Manipulação de Mouse no Popover (Bordas e Movimentação) ───────────────────
void QuickLookupPopover::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_bordaAtiva = detectarBorda(event->pos());
        if (m_bordaAtiva != BordaRedimensionamento::Nenhuma) {
            m_resizing = true;
            m_dragging = false;
            m_resizeStartGeom = geometry();
            m_resizeStartPos = event->globalPosition().toPoint();
            event->accept();
            return;
        } else {
            m_dragging = true;
            m_resizing = false;
            m_dragStartGlobalPos = event->globalPosition().toPoint();
            m_dragStartWidgetPos = pos();
            raise();
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void QuickLookupPopover::mouseMoveEvent(QMouseEvent *event)
{
    if (m_resizing && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->globalPosition().toPoint() - m_resizeStartPos;
        const QRect tela = obterGeometriaTelaUtil();
        const int minW = minimumWidth();
        const int minH = minimumHeight();
        const int maxW = qBound(minW, qMin(maximumWidth(), tela.width() - 24), 1200);
        const int maxH = qBound(minH, qMin(maximumHeight(), tela.height() - 24), 900);

        int left = m_resizeStartGeom.left();
        int top  = m_resizeStartGeom.top();
        int w    = m_resizeStartGeom.width();
        int h    = m_resizeStartGeom.height();
        const int anchorRight  = m_resizeStartGeom.left() + m_resizeStartGeom.width();
        const int anchorBottom = m_resizeStartGeom.top() + m_resizeStartGeom.height();

        switch (m_bordaAtiva) {
        case BordaRedimensionamento::Direita:
            w = qBound(minW, m_resizeStartGeom.width() + delta.x(), maxW);
            break;
        case BordaRedimensionamento::Base:
            h = qBound(minH, m_resizeStartGeom.height() + delta.y(), maxH);
            break;
        case BordaRedimensionamento::BaseDireita:
            w = qBound(minW, m_resizeStartGeom.width() + delta.x(), maxW);
            h = qBound(minH, m_resizeStartGeom.height() + delta.y(), maxH);
            break;
        case BordaRedimensionamento::Esquerda:
            w = qBound(minW, m_resizeStartGeom.width() - delta.x(), maxW);
            left = anchorRight - w;
            break;
        case BordaRedimensionamento::BaseEsquerda:
            w = qBound(minW, m_resizeStartGeom.width() - delta.x(), maxW);
            left = anchorRight - w;
            h = qBound(minH, m_resizeStartGeom.height() + delta.y(), maxH);
            break;
        case BordaRedimensionamento::Topo:
            h = qBound(minH, m_resizeStartGeom.height() - delta.y(), maxH);
            top = anchorBottom - h;
            break;
        case BordaRedimensionamento::TopoDireita:
            h = qBound(minH, m_resizeStartGeom.height() - delta.y(), maxH);
            top = anchorBottom - h;
            w = qBound(minW, m_resizeStartGeom.width() + delta.x(), maxW);
            break;
        case BordaRedimensionamento::TopoEsquerda:
            w = qBound(minW, m_resizeStartGeom.width() - delta.x(), maxW);
            h = qBound(minH, m_resizeStartGeom.height() - delta.y(), maxH);
            left = anchorRight - w;
            top  = anchorBottom - h;
            break;
        default:
            break;
        }

        aplicarGeometriaSegura(QRect(left, top, w, h));
        event->accept();
        return;
    } else if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        const QPoint delta = event->globalPosition().toPoint() - m_dragStartGlobalPos;
        QRect novaGeom = geometry();
        novaGeom.moveTopLeft(m_dragStartWidgetPos + delta);
        aplicarGeometriaSegura(novaGeom);
        event->accept();
        return;
    } else {
        // Apenas passando o mouse: atualiza o cursor de acordo com a borda
        BordaRedimensionamento borda = detectarBorda(event->pos());
        atualizarCursorBorda(borda);
    }

    QWidget::mouseMoveEvent(event);
}

void QuickLookupPopover::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_resizing = false;
        m_bordaAtiva = BordaRedimensionamento::Nenhuma;
        atualizarCursorBorda(detectarBorda(event->pos()));
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}
