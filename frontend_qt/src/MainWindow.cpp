#include "MainWindow.h"
#include "NetworkManager.h"
#include "ThemeManager.h"
#include "PromptInputWidget.h"
#include "TitleBar.h"
#include "FloatingHUD.h"
#include "QuickLookupPopover.h"
#include "SelectionOverlayWidget.h"
#include "TranslationWidget.h"
#include "SpeechTutorWidget.h"
#include "AiAssistantWidget.h"
#include "NotesWidget.h"
#include "SettingsWidget.h"

#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QToolButton>
#include <QTabBar>
#include <QStyle>
#include <QScrollBar>
#include <QScreen>
#include <QBuffer>
#include <QPixmap>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QClipboard>
#include <QPdfPageNavigator>
#include <QMessageBox>
#include <QDateTime>
#include <QKeySequence>
#include <QSettings>
#include <QCloseEvent>
#include <QTimer>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QAbstractSpinBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStatusBar>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QColorDialog>
#include <QTextStream>
#include <cmath>
#include <algorithm>

#include <QTcpServer>

// ═════════════════════════════════════════════════════════════════════════════
// Construtor e Inicialização
// ═════════════════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowFlags(Qt::FramelessWindowHint);
    setWindowTitle("Vellum — Leitor Técnico & Tutor");
    setWindowIcon(QIcon(":/app_icon.png"));
    setMinimumSize(960, 640);

    m_timerProgresso = new QTimer(this);
    connect(m_timerProgresso, &QTimer::timeout, this, &MainWindow::atualizarProgressoPasso);

    QTcpServer tempServer;
    if (tempServer.listen(QHostAddress::LocalHost, 0)) {
        m_backendPort = tempServer.serverPort();
        tempServer.close();
    }
    m_net = new NetworkManager(QString("http://localhost:%1").arg(m_backendPort), this);

    setupUi();
    m_themeManager = new ThemeManager(this);
    m_themeManager->applyTheme(ThemeManager::Theme::Dark);
    connectSignals();

    carregarConfiguracoes();

    setAcceptDrops(true);

    // Garante painel fechado por padrão ao iniciar
    m_tabWidget->setVisible(false);
    m_actTogglePainel->setChecked(false);

    m_net->verificarConexao();

    // Inicia o motor diretamente junto com a interface
    verificarEIniciarBackend();

    // Registra ou atualiza o atalho do aplicativo no lançador do Linux
    registrarLancadorLinux();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_isFechando = true;
    salvarConfiguracoes();
    pararBackend();
    event->accept();
}

// ═════════════════════════════════════════════════════════════════════════════
// Barra de Menus Superior (Controle Central sem Poluição na Tela)
// ═════════════════════════════════════════════════════════════════════════════

QMenuBar* MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = new QMenuBar(this);

    // ── Menu Arquivo ──────────────────────────────────────────────────────
    auto *menuArquivo = menuBar->addMenu("&Arquivo");

    auto *actAbrir = menuArquivo->addAction("Abrir Documento...");
    actAbrir->setShortcut(QKeySequence::Open);
    connect(actAbrir, &QAction::triggered, this, &MainWindow::onAbrirPdf);

    auto *actFechar = menuArquivo->addAction("Fechar Documento");
    connect(actFechar, &QAction::triggered, this, &MainWindow::onFecharPdf);

    menuArquivo->addSeparator();

    auto *actSair = menuArquivo->addAction("Sair");
    actSair->setShortcut(QKeySequence::Quit);
    connect(actSair, &QAction::triggered, this, &QWidget::close);

    // ── Menu Exibir ───────────────────────────────────────────────────────
    auto *menuExibir = menuBar->addMenu("&Exibir");

    auto *actModo1 = menuExibir->addAction("Página Individual");
    actModo1->setShortcut(QKeySequence("Ctrl+1"));
    connect(actModo1, &QAction::triggered, this, &MainWindow::onModo1PagClicado);

    auto *actModo2 = menuExibir->addAction("Duas Páginas (Livro)");
    actModo2->setShortcut(QKeySequence("Ctrl+2"));
    connect(actModo2, &QAction::triggered, this, &MainWindow::onModo2PagClicado);

    auto *actModoCont = menuExibir->addAction("Rolagem Contínua");
    actModoCont->setShortcut(QKeySequence("Ctrl+3"));
    connect(actModoCont, &QAction::triggered, this, &MainWindow::onModoContClicado);

    menuExibir->addSeparator();

    auto *actZoomMais = menuExibir->addAction("Aumentar Zoom");
    actZoomMais->setShortcut(QKeySequence::ZoomIn);
    connect(actZoomMais, &QAction::triggered, this, &MainWindow::onZoomMais);

    auto *actZoomMenos = menuExibir->addAction("Diminuir Zoom");
    actZoomMenos->setShortcut(QKeySequence::ZoomOut);
    connect(actZoomMenos, &QAction::triggered, this, &MainWindow::onZoomMenos);

    auto *actZoomReset = menuExibir->addAction("Tamanho Real (100%)");
    actZoomReset->setShortcut(QKeySequence("Ctrl+0"));
    connect(actZoomReset, &QAction::triggered, this, &MainWindow::onZoomReset);

    auto *actFitWidth = menuExibir->addAction("Ajustar à Largura");
    connect(actFitWidth, &QAction::triggered, this, &MainWindow::onAjustarLargura);

    auto *actFitPage = menuExibir->addAction("Ajustar à Página");
    connect(actFitPage, &QAction::triggered, this, &MainWindow::onAjustarPagina);

    menuExibir->addSeparator();

    m_actTelaCheia = menuExibir->addAction("Alternar Tela Cheia");
    m_actTelaCheia->setShortcut(QKeySequence(Qt::Key_F11));
    m_actTelaCheia->setShortcutContext(Qt::ApplicationShortcut);
    addAction(m_actTelaCheia);
    connect(m_actTelaCheia, &QAction::triggered, this, &MainWindow::onToggleTelaCheia);

    m_actBarraLeitor = menuExibir->addAction("Barra de Navegação do Leitor");
    m_actBarraLeitor->setCheckable(true);
    m_actBarraLeitor->setChecked(true);
    connect(m_actBarraLeitor, &QAction::toggled, this, [this](bool visivel) {
        if (m_barraVis) m_barraVis->setVisible(visivel);
    });

    // ── Menu Painel ───────────────────────────────────────────────────────
    auto *menuPainel = menuBar->addMenu("&Painel");

    m_actTogglePainel = menuPainel->addAction("Alternar Painel Lateral");
    m_actTogglePainel->setCheckable(true);
    m_actTogglePainel->setChecked(false); // Inicia fechado!
    m_actTogglePainel->setShortcut(QKeySequence("F4"));
    connect(m_actTogglePainel, &QAction::toggled, this, &MainWindow::onTogglePainelLateral);

    menuPainel->addSeparator();

    auto *actAbaTrad = menuPainel->addAction("Ir para Tradução");
    actAbaTrad->setShortcut(QKeySequence("Ctrl+T"));
    actAbaTrad->setShortcutContext(Qt::ApplicationShortcut);
    connect(actAbaTrad, &QAction::triggered, this, [this]() { executarComandoAba(0); });
    addAction(actAbaTrad);

    auto *actAbaTutor = menuPainel->addAction("Ir para Tutor & Pronúncia");
    actAbaTutor->setShortcut(QKeySequence("Ctrl+U"));
    actAbaTutor->setShortcutContext(Qt::ApplicationShortcut);
    connect(actAbaTutor, &QAction::triggered, this, [this]() { executarComandoAba(1); });
    addAction(actAbaTutor);

    auto *actAbaChat = menuPainel->addAction("Ir para Assistente IA & Dúvidas");
    actAbaChat->setShortcut(QKeySequence("Ctrl+I"));
    actAbaChat->setShortcutContext(Qt::ApplicationShortcut);
    connect(actAbaChat, &QAction::triggered, this, [this]() { executarComandoAba(2); });
    addAction(actAbaChat);

    auto *actAbaNotas = menuPainel->addAction("Ir para Bloco de Notas");
    actAbaNotas->setShortcut(QKeySequence("Ctrl+N"));
    actAbaNotas->setShortcutContext(Qt::ApplicationShortcut);
    connect(actAbaNotas, &QAction::triggered, this, [this]() { executarComandoAba(3); });
    addAction(actAbaNotas);

    auto *actAbaConfig = menuPainel->addAction("Ir para Desempenho & Layout");
    actAbaConfig->setShortcut(QKeySequence("Ctrl+D"));
    actAbaConfig->setShortcutContext(Qt::ApplicationShortcut);
    connect(actAbaConfig, &QAction::triggered, this, [this]() { executarComandoAba(4); });
    addAction(actAbaConfig);

    // ── Menu Layout ───────────────────────────────────────────────────────
    auto *menuLayout = menuBar->addMenu("&Layout");

    auto *actSalvarLayout = menuLayout->addAction("Salvar Layout Atual");
    actSalvarLayout->setShortcut(QKeySequence("Ctrl+S"));
    connect(actSalvarLayout, &QAction::triggered, this, &MainWindow::onSalvarLayout);

    auto *actRestaurarLayout = menuLayout->addAction("Restaurar Layout Padrão");
    connect(actRestaurarLayout, &QAction::triggered, this, &MainWindow::onRestaurarLayoutPadrao);

    menuLayout->addSeparator();

    auto *actProp100 = menuLayout->addAction("Foco no Leitor (100%)");
    connect(actProp100, &QAction::triggered, this, [this]() { onAplicarProporcao(100); });

    auto *actProp70 = menuLayout->addAction("Leitura & Painel (70% / 30%)");
    connect(actProp70, &QAction::triggered, this, [this]() { onAplicarProporcao(70); });

    auto *actProp50 = menuLayout->addAction("Divisão Equilibrada (50% / 50%)");
    connect(actProp50, &QAction::triggered, this, [this]() { onAplicarProporcao(50); });

    // ── Menu Configurações ────────────────────────────────────────────────
    auto *menuConfig = menuBar->addMenu("&Configurações");

    auto *actConfigurarIA = menuConfig->addAction("Chaves de API e IA (Gemini / Ollama)...");
    actConfigurarIA->setShortcut(QKeySequence("Ctrl+Shift+I"));
    connect(actConfigurarIA, &QAction::triggered, this, &MainWindow::onConfigurarIA);

    // ── Indicador de Modelo Ativo e Progresso no Canto Superior Direito ───
    auto *cornerWidget = new QWidget(this);
    auto *cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 10, 0);
    cornerLayout->setSpacing(8);

    m_lblProgressoTopo = new QLabel(cornerWidget);
    m_lblProgressoTopo->setObjectName("lblProgressoTopo");
    m_lblProgressoTopo->setVisible(false);
    cornerLayout->addWidget(m_lblProgressoTopo);

    m_badgeModeloAtivo = new QLabel("Tradução: —", cornerWidget);
    m_badgeModeloAtivo->setObjectName("badgeModeloAtivo");
    cornerLayout->addWidget(m_badgeModeloAtivo);

    menuBar->setCornerWidget(cornerWidget, Qt::TopRightCorner);

    return menuBar;
}

// ═════════════════════════════════════════════════════════════════════════════
// Construção da Interface
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::setupUi()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    
    m_titleBar = new TitleBar(this);
    mainLayout->addWidget(m_titleBar, 0);

    m_customMenuBar = setupMenuBar();
    m_customMenuBar->setFixedHeight(32);
    m_customMenuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mainLayout->addWidget(m_customMenuBar, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);
    m_splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(m_splitter, 1);
    
    setCentralWidget(centralWidget);

    // ── Painel Esquerdo: Visualizador de Documentos PDF ───────────────────
    m_painelEsq = new QWidget(this);
    m_painelEsq->setObjectName("painelEsquerdo");
    auto *layoutEsq = new QVBoxLayout(m_painelEsq);
    layoutEsq->setContentsMargins(0, 0, 0, 0);
    layoutEsq->setSpacing(0);

    m_barraVis = criarBarraVisualizacao();
    layoutEsq->addWidget(m_barraVis);

    auto *viewContainer = new QWidget(m_painelEsq);
    viewContainer->setObjectName("viewContainer");
    auto *viewLayout = new QHBoxLayout(viewContainer);
    viewLayout->setContentsMargins(0, 0, 0, 0);
    viewLayout->setSpacing(2);

    m_pdfDoc = new QPdfDocument(this);

    m_pdfView = new QPdfView(viewContainer);
    m_pdfView->setDocument(m_pdfDoc);
    m_pdfView->setPageMode(QPdfView::PageMode::SinglePage);
    m_pdfView->setZoomMode(QPdfView::ZoomMode::FitInView);
    m_pdfView->installEventFilter(this);
    m_pdfView->viewport()->installEventFilter(this);

    m_pdfView2 = new QPdfView(viewContainer);
    m_pdfView2->setDocument(m_pdfDoc);
    m_pdfView2->setPageMode(QPdfView::PageMode::SinglePage);
    m_pdfView2->setZoomMode(QPdfView::ZoomMode::FitInView);
    m_pdfView2->setVisible(false);
    m_pdfView2->installEventFilter(this);
    m_pdfView2->viewport()->installEventFilter(this);

    m_activePdfView = m_pdfView;

    m_emptyStateWidget = new QWidget(viewContainer);
    m_emptyStateWidget->setObjectName("emptyStateWidget");
    auto *emptyLayout = new QVBoxLayout(m_emptyStateWidget);
    emptyLayout->setAlignment(Qt::AlignCenter);
    emptyLayout->setSpacing(14);

    auto *lblIcone = new QLabel(m_emptyStateWidget);
    lblIcone->setPixmap(QPixmap(":/app_icon.png").scaled(92, 92, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    lblIcone->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(lblIcone);

    auto *lblTitulo = new QLabel("Vellum — Leitor Técnico & Tutor", m_emptyStateWidget);
    lblTitulo->setObjectName("lblTituloEmpty");
    lblTitulo->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(lblTitulo);

    auto *lblSub = new QLabel("Arraste e solte um livro técnico PDF aqui ou use o menu Arquivo > Abrir (Ctrl+O)", m_emptyStateWidget);
    lblSub->setObjectName("lblSubEmpty");
    lblSub->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(lblSub);

    auto *btnAbrirInicial = new QPushButton("Selecionar Livro PDF...", m_emptyStateWidget);
    btnAbrirInicial->setObjectName("btnPrimario");
    btnAbrirInicial->setCursor(Qt::PointingHandCursor);
    btnAbrirInicial->setFixedSize(200, 38);
    connect(btnAbrirInicial, &QPushButton::clicked, this, &MainWindow::onAbrirPdf);
    emptyLayout->addWidget(btnAbrirInicial, 0, Qt::AlignCenter);

    viewLayout->addWidget(m_emptyStateWidget, 1);
    viewLayout->addWidget(m_pdfView, 1);
    viewLayout->addWidget(m_pdfView2, 1);

    m_pdfView->setVisible(false);
    m_emptyStateWidget->setVisible(true);

    layoutEsq->addWidget(viewContainer, 1);

    m_rubberBand = new QRubberBand(QRubberBand::Rectangle, m_pdfView->viewport());
    m_overlaySelecao = new SelectionOverlayWidget(m_pdfView->viewport());

    m_pdfView->viewport()->setMouseTracking(true);
    m_pdfView2->viewport()->setMouseTracking(true);

    m_splitter->addWidget(m_painelEsq);

    // ── Painel Direito: QTabWidget de ferramentas sob demanda ────────────
    m_tabTraducao = new TranslationWidget(this);
    m_tabTutor = new SpeechTutorWidget(this);
    m_tabChatIA = new AiAssistantWidget(this);
    m_tabAnotacoes = new NotesWidget(this);
    m_tabConfiguracoes = new SettingsWidget(this);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName("tabWidgetPrincipal");
    m_tabWidget->setDocumentMode(true);
    m_tabWidget->setTabsClosable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onFecharAbaPainel);

    m_splitter->addWidget(m_tabWidget);

    // Inicia fechado por padrão para tela 100% limpa
    m_tabWidget->setVisible(false);

    m_splitter->setStretchFactor(0, 70);
    m_splitter->setStretchFactor(1, 30);

    // ── Conceito 1: Floating HUD (Dock de Leitura em Tela Cheia) ──────────
    m_floatingHud = new FloatingHUD(this);
    m_floatingHud->hide();
    connect(m_floatingHud, &FloatingHUD::paginaAnteriorClicada, this, &MainWindow::onPaginaAnterior);
    connect(m_floatingHud, &FloatingHUD::paginaProximaClicada, this, &MainWindow::onPaginaProxima);
    connect(m_floatingHud, &FloatingHUD::zoomMenosClicado, this, [this]() {
        if (m_pdfView) {
            m_pdfView->setZoomFactor(std::max(0.2, m_pdfView->zoomFactor() - 0.15));
            if (m_floatingHud) m_floatingHud->atualizarZoom(m_pdfView->zoomFactor());
        }
    });
    connect(m_floatingHud, &FloatingHUD::zoomMaisClicado, this, [this]() {
        if (m_pdfView) {
            m_pdfView->setZoomFactor(std::min(5.0, m_pdfView->zoomFactor() + 0.15));
            if (m_floatingHud) m_floatingHud->atualizarZoom(m_pdfView->zoomFactor());
        }
    });
    connect(m_floatingHud, &FloatingHUD::sairTelaCheiaClicado, this, &MainWindow::onToggleTelaCheia);

    // ── Janela Flutuante Versátil (Tradução, Tutor, Assistente IA, Bloco de Notas) ──
    m_quickLookupPopover = new QuickLookupPopover(this);
    m_quickLookupPopover->hide();

    connect(m_quickLookupPopover, &QuickLookupPopover::solicitarRetraducao, this, [this](const QString &novoTextoEn) {
        if (!novoTextoEn.isEmpty()) {
            m_textoOriginalEn = novoTextoEn;
            if (m_tabTraducao) {
                m_tabTraducao->setOriginalTextSilent(novoTextoEn);
            }
            dispararTraducaoTexto(novoTextoEn);
        }
    });

    connect(m_quickLookupPopover, &QuickLookupPopover::ouvirPronunciaClicada, this, [this](const QString &texto) {
        if (m_net && !texto.isEmpty()) {
            const QString voz = (m_tabTutor && m_tabTutor->comboVoz()) ? m_tabTutor->comboVoz()->currentData().toString() : QStringLiteral("en-US-JennyNeural");
            const QString vel = (m_tabTutor && m_tabTutor->comboVelocidade()) ? m_tabTutor->comboVelocidade()->currentData().toString() : QStringLiteral("+0%");
            m_net->falarTexto(texto, voz, vel);
        }
    });

    connect(m_quickLookupPopover, &QuickLookupPopover::pararAudioClicado, this, &MainWindow::onPararAudio);

    connect(m_quickLookupPopover, &QuickLookupPopover::gravarVozTutorClicada, this, [this](const QString &textoEsperado) {
        if (!textoEsperado.isEmpty()) {
            m_textoOriginalEn = textoEsperado;
            if (m_tabTutor && m_tabTutor->promptTextoTutor()) {
                m_tabTutor->promptTextoTutor()->setText(textoEsperado);
            }
        }
        onGravarVozTutor();
    });

    connect(m_quickLookupPopover, &QuickLookupPopover::enviarMensagemChatClicada, this, [this](const QString &pergunta, const QString &imagemB64) {
        if (m_net) {
            const int tierIdx = (m_tabConfiguracoes && m_tabConfiguracoes->comboPerfil()) ? m_tabConfiguracoes->comboPerfil()->currentIndex() : 0;
            QString apiKey = (tierIdx == 0) ? m_iaApiKey.trimmed() : QString();
            QString modelo = (tierIdx == 0) ? m_iaModelo.trimmed() : (tierIdx == 1 ? "phi3:mini" : (tierIdx == 2 ? "llama3.2:3b" : "llama3"));
            m_net->enviarMensagemChat(pergunta, imagemB64, m_chatHistoricoJson, m_iaProvedor, apiKey, modelo);
        }
    });

    connect(m_quickLookupPopover, &QuickLookupPopover::salvarNotaNoPainelClicada, this, [this](const QString &texto) {
        if (m_tabAnotacoes && !texto.isEmpty()) {
            m_tabAnotacoes->appendQuickNote(texto);
            appendLog("Anotação sincronizada com o Bloco de Notas principal.", "success");
        }
    });

    connect(m_quickLookupPopover, &QuickLookupPopover::expandirNoPainelClicado, this, [this](int modoIndex, const QString &d1, const QString &d2) {
        abrirOuAtivarAba(modoIndex);
        if (modoIndex == 0) {
            if (m_tabTraducao && !d1.isEmpty()) {
                m_tabTraducao->setOriginalTextSilent(d1);
            }
        } else if (modoIndex == 1) {
            if (m_tabTutor && m_tabTutor->promptTextoTutor() && !d1.isEmpty()) {
                m_tabTutor->promptTextoTutor()->setText(d1);
            }
        } else if (modoIndex == 2) {
            if (m_tabChatIA && m_tabChatIA->promptInput()) {
                if (!d1.isEmpty()) m_tabChatIA->promptInput()->setText(d1);
                if (!d2.isEmpty()) m_tabChatIA->promptInput()->showImagePreview(d2, "Circuito");
            }
        } else if (modoIndex == 3) {
            if (m_tabAnotacoes && !d1.isEmpty()) {
                m_tabAnotacoes->setText(d1);
            }
        }
    });


    // ── Barra de Status Inferior Minimalista ──────────────────────────────
    auto *bar = statusBar();

    m_lblStatusGeral = new QLabel("Pronto", bar);
    m_lblStatusGeral->setObjectName("lblStatusGeral");
    bar->addWidget(m_lblStatusGeral, 1);

    m_lblProgressoNumerico = new QLabel("", bar);
    m_lblProgressoNumerico->setObjectName("lblProgressoNumerico");
    bar->addPermanentWidget(m_lblProgressoNumerico);

    // ── Conceito 5: Badge de Métricas de IA em Tempo Real ──────────────────
    m_badgeMetricasIA = new QLabel("IA: Pronta", bar);
    m_badgeMetricasIA->setObjectName("badgeMetricasIA");
    bar->addPermanentWidget(m_badgeMetricasIA);
}

// ─────────────────────────────────────────────────────────────────────────────
// Barra Superior de Visualização do Documento
// ─────────────────────────────────────────────────────────────────────────────

QWidget* MainWindow::criarBarraVisualizacao()
{
    auto *bar = new QFrame(this);
    bar->setObjectName("barraVisualizacao");
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(8);

    // Navegação
    m_btnPagAnterior = new QPushButton("Anterior", bar);
    m_btnPagAnterior->setObjectName("btnNav");
    m_btnPagAnterior->setEnabled(false);

    auto *lblPagTexto = new QLabel("Página", bar);
    lblPagTexto->setObjectName("lblToolbar");

    m_spinPagina = new QSpinBox(bar);
    m_spinPagina->setObjectName("spinPagina");
    m_spinPagina->setMinimum(1);
    m_spinPagina->setMaximum(1);
    m_spinPagina->setValue(1);
    m_spinPagina->setEnabled(false);

    m_lblTotalPag = new QLabel("/ 0", bar);
    m_lblTotalPag->setObjectName("lblToolbar");

    m_btnPagProxima = new QPushButton("Próxima", bar);
    m_btnPagProxima->setObjectName("btnNav");
    m_btnPagProxima->setEnabled(false);

    layout->addWidget(m_btnPagAnterior);
    layout->addWidget(lblPagTexto);
    layout->addWidget(m_spinPagina);
    layout->addWidget(m_lblTotalPag);
    layout->addWidget(m_btnPagProxima);

    auto *vsep1 = new QFrame(bar);
    vsep1->setFrameShape(QFrame::VLine);
    vsep1->setObjectName("vseparador");
    layout->addWidget(vsep1);

    // Modos de Exibição
    m_btnModo1Pag = new QPushButton("Individual", bar);
    m_btnModo1Pag->setObjectName("btnModoVis");
    m_btnModo1Pag->setCheckable(true);
    m_btnModo1Pag->setChecked(true);

    m_btnModo2Pag = new QPushButton("Lado a Lado", bar);
    m_btnModo2Pag->setObjectName("btnModoVis");
    m_btnModo2Pag->setCheckable(true);

    m_btnModoCont = new QPushButton("Contínuo", bar);
    m_btnModoCont->setObjectName("btnModoVis");
    m_btnModoCont->setCheckable(true);

    layout->addWidget(m_btnModo1Pag);
    layout->addWidget(m_btnModo2Pag);
    layout->addWidget(m_btnModoCont);

    auto *vsep2 = new QFrame(bar);
    vsep2->setFrameShape(QFrame::VLine);
    vsep2->setObjectName("vseparador");
    layout->addWidget(vsep2);

    // Zoom
    m_btnZoomMenos = new QPushButton("-", bar);
    m_btnZoomMenos->setObjectName("btnNav");

    m_btnZoomReset = new QPushButton("100%", bar);
    m_btnZoomReset->setObjectName("btnZoomReset");

    m_btnZoomMais = new QPushButton("+", bar);
    m_btnZoomMais->setObjectName("btnNav");

    m_btnAjustarLarg = new QPushButton("Largura", bar);
    m_btnAjustarLarg->setObjectName("btnNav");

    m_btnAjustarPag = new QPushButton("Página", bar);
    m_btnAjustarPag->setObjectName("btnNav");

    m_btnTelaCheia = new QPushButton("Tela Cheia", bar);
    m_btnTelaCheia->setObjectName("btnNav");
    m_btnTelaCheia->setToolTip("Alternar modo Tela Cheia sem as barras superiores (F11)");

    layout->addWidget(m_btnZoomMenos);
    layout->addWidget(m_btnZoomReset);
    layout->addWidget(m_btnZoomMais);
    layout->addWidget(m_btnAjustarLarg);
    layout->addWidget(m_btnAjustarPag);
    layout->addWidget(m_btnTelaCheia);

    layout->addStretch();
    return bar;
}

// ═════════════════════════════════════════════════════════════════════════════
// Folha de Estilos (Design Sóbrio, Minimalista, Dark Slate)
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::setupStyleSheet()
{
    // O estilo agora é gerenciado pelo ThemeManager.
    // O CSS original foi movido para resources/themes/dark.qss.
}

// ═════════════════════════════════════════════════════════════════════════════
// Conexão de Sinais
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::connectSignals()
{
    // Aba 1: Tradução
    if (m_tabTraducao) {
        connect(m_tabTraducao, &TranslationWidget::toggleModoCapturaRequested, this, &MainWindow::onToggleModoCaptura);
        connect(m_tabTraducao, &TranslationWidget::formatoSelecaoChanged, this, [this](int idx) {
            if (m_overlaySelecao) {
                m_overlaySelecao->setFormato(idx == 0 ? SelectionOverlayWidget::Formato::Retangular : SelectionOverlayWidget::Formato::Livre);
            }
        });
        connect(m_tabTraducao, &TranslationWidget::traduzirSelecaoRequested, this, &MainWindow::onTraduzirDireto);
        connect(m_tabTraducao, &TranslationWidget::retraduzirTextoRequested, this, [this](const QString &txt) {
            if (txt.isEmpty()) return;
            m_textoOriginalEn = txt;
            dispararTraducaoTexto(txt);
        });
        connect(m_tabTraducao, &TranslationWidget::copiarTraducaoRequested, this, &MainWindow::onCopiarTraducao);
    }

    // Aba 2: Tutor e Áudio
    if (m_tabTutor) {
        connect(m_tabTutor, &SpeechTutorWidget::ouvirPronunciaRequested, this, &MainWindow::onOuvirPronuncia);
        connect(m_tabTutor, &SpeechTutorWidget::pararAudioRequested,     this, &MainWindow::onPararAudio);
        connect(m_tabTutor, &SpeechTutorWidget::gravarVozRequested,      this, &MainWindow::onGravarVozTutor);
    }

    // Aba 3: Assistente IA & Chat
    if (m_tabChatIA) {
        connect(m_tabChatIA, &AiAssistantWidget::provedorIaChanged, this, [this](int) {
            if (!m_tabChatIA || !m_tabChatIA->comboChatIA()) return;
            const QString prov = m_tabChatIA->comboChatIA()->currentData().toString();
            m_iaProvedor = prov;
            if (m_tabChatIA->lblModeloAtivoChat()) {
                if (prov == "gemini") {
                    if (m_iaModelo.isEmpty() || m_iaModelo.contains("llama") || m_iaModelo.contains("phi")) {
                        m_iaModelo = "gemini-3.5-flash-lite";
                    }
                    m_tabChatIA->lblModeloAtivoChat()->setText(m_iaModelo);
                } else {
                    m_tabChatIA->lblModeloAtivoChat()->setText(
                        m_ollamaModelTier.isEmpty() ? "llama3" : m_ollamaModelTier
                    );
                }
            }
            atualizarBadgeModeloAtivo();
            salvarConfiguracoes();
        });
        connect(m_tabChatIA, &AiAssistantWidget::configurarIaRequested,     this, &MainWindow::onConfigurarIA);
        connect(m_tabChatIA, &AiAssistantWidget::limparChatRequested,       this, &MainWindow::onLimparChat);
        connect(m_tabChatIA, &AiAssistantWidget::enviarChatRequested,       this, &MainWindow::onEnviarChat);
        connect(m_tabChatIA, &AiAssistantWidget::capturarCircuitoRequested, this, &MainWindow::onCapturarCircuitoChat);
        connect(m_tabChatIA, &AiAssistantWidget::anexarImagemRequested,     this, &MainWindow::onAnexarImagemChat);
        connect(m_tabChatIA, &AiAssistantWidget::removerImagemRequested,    this, &MainWindow::onRemoverImagemChat);
        connect(m_tabChatIA, &AiAssistantWidget::colarTrechoRequested,      this, &MainWindow::onColarTrechoChat);
    }

    // Aba 4: Bloco de Notas
    if (m_tabAnotacoes) {
        connect(m_tabAnotacoes, &NotesWidget::logMessage, this, &MainWindow::appendLog);
    }

    // Aba 5: Configurações & Layout
    if (m_tabConfiguracoes) {
        connect(m_tabConfiguracoes, &SettingsWidget::perfilHardwareChanged,    this, &MainWindow::onPerfilAlterado);
        connect(m_tabConfiguracoes, &SettingsWidget::salvarLayoutRequested,    this, &MainWindow::onSalvarLayout);
        connect(m_tabConfiguracoes, &SettingsWidget::restaurarLayoutRequested, this, &MainWindow::onRestaurarLayoutPadrao);
        connect(m_tabConfiguracoes, &SettingsWidget::aplicarProporcaoRequested, this, &MainWindow::onAplicarProporcao);
    }

    // Navegação
    connect(m_btnPagAnterior,  &QPushButton::clicked, this, &MainWindow::onPaginaAnterior);
    connect(m_btnPagProxima,   &QPushButton::clicked, this, &MainWindow::onPaginaProxima);
    connect(m_spinPagina, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onPaginaSpinChanged);

    // Modos
    connect(m_btnModo1Pag,     &QPushButton::clicked, this, &MainWindow::onModo1PagClicado);
    connect(m_btnModo2Pag,     &QPushButton::clicked, this, &MainWindow::onModo2PagClicado);
    connect(m_btnModoCont,     &QPushButton::clicked, this, &MainWindow::onModoContClicado);

    // Zoom e Janela
    connect(m_btnZoomMenos,    &QPushButton::clicked, this, &MainWindow::onZoomMenos);
    connect(m_btnZoomMais,     &QPushButton::clicked, this, &MainWindow::onZoomMais);
    connect(m_btnZoomReset,    &QPushButton::clicked, this, &MainWindow::onZoomReset);
    connect(m_btnAjustarLarg,  &QPushButton::clicked, this, &MainWindow::onAjustarLargura);
    connect(m_btnAjustarPag,   &QPushButton::clicked, this, &MainWindow::onAjustarPagina);
    connect(m_btnTelaCheia,    &QPushButton::clicked, this, &MainWindow::onToggleTelaCheia);

    // NetworkManager
    connect(m_net, &NetworkManager::traducaoDiretaResultado,      this, &MainWindow::onTraducaoDiretaResultado);
    connect(m_net, &NetworkManager::limparOcrResultado,           this, &MainWindow::onLimparOcrResultado);
    connect(m_net, &NetworkManager::transcricaoResultado,         this, &MainWindow::onTranscricaoResultado);
    connect(m_net, &NetworkManager::servidorOnline,               this, &MainWindow::onServidorOnline);
    connect(m_net, &NetworkManager::modelosStatusRecebido,        this, &MainWindow::onModelosStatusRecebido);
    connect(m_net, &NetworkManager::requisicaoIniciada,           this, &MainWindow::onRequisicaoIniciada);
    connect(m_net, &NetworkManager::requisicaoConcluida,          this, &MainWindow::onRequisicaoConcluida);
    connect(m_net, &NetworkManager::erroRequisicao,               this, &MainWindow::onErroRequisicao);
    connect(m_net, &NetworkManager::gravacaoIniciada,             this, &MainWindow::onGravacaoIniciada);
    connect(m_net, &NetworkManager::falaIniciada,                 this, &MainWindow::onFalaIniciada);
    connect(m_net, &NetworkManager::avaliacaoPronunciaResultado,  this, &MainWindow::onAvaliacaoPronunciaResultado);
    connect(m_net, &NetworkManager::chatRespostaResultado,        this, &MainWindow::onChatRespostaResultado);
    connect(m_net, &NetworkManager::ocrPaginaCompletaResultado,   this, &MainWindow::onOcrPaginaCompletaResultado);

    connect(m_pdfView->pageNavigator(), &QPdfPageNavigator::currentPageChanged,
            this, [this](int pag) {
        if (m_bloquearSyncPagina) {
            return;
        }
        if (m_modoVis != ModoVisualizacao::Continuo) {
            m_paginaAtual = pag;
            atualizarInfoNavegacao();
        }
    });

    if (m_pdfView->verticalScrollBar()) {
        connect(m_pdfView->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
            if (m_bloquearSyncPagina || m_modoVis != ModoVisualizacao::Continuo || !m_pdfDoc || m_pdfDoc->pageCount() == 0) {
                return;
            }
            const int pagReal = obterPaginaVisivelNoModoContinuo();
            if (pagReal != m_paginaAtual) {
                m_paginaAtual = pagReal;
                atualizarInfoNavegacao();
            }
        });
    }

    // Proteção de salto indevido de página durante zoom (evita oscilação com roda do mouse ou botões)
    m_timerZoomDebounce = new QTimer(this);
    m_timerZoomDebounce->setSingleShot(true);
    connect(m_timerZoomDebounce, &QTimer::timeout, this, [this]() {
        if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) {
            m_bloquearSyncPagina = false;
            return;
        }
        const int pagFoco = qBound(0, m_paginaAtual, m_pdfDoc->pageCount() - 1);
        if (m_modoVis == ModoVisualizacao::Continuo) {
            const int y = calcularScrollVerticalParaPagina(pagFoco, m_pdfView->zoomFactor());
            if (m_pdfView->verticalScrollBar()) {
                m_pdfView->verticalScrollBar()->setValue(y);
            }
        }
        m_pdfView->pageNavigator()->jump(pagFoco, {}, 0);
        m_paginaAtual = pagFoco;
        atualizarInfoNavegacao();
        m_bloquearSyncPagina = false;
    });

    // Proteção de salto indevido de página durante redimensionamento do painel/splitter/janela/tela cheia
    m_timerDebounceResize = new QTimer(this);
    m_timerDebounceResize->setSingleShot(true);
    connect(m_timerDebounceResize, &QTimer::timeout, this, [this]() {
        if (m_pdfDoc && m_pdfDoc->pageCount() > 0 && m_paginaSalvaResize >= 0) {
            const int total = m_pdfDoc->pageCount();
            const int pagFoco = qBound(0, m_paginaSalvaResize, total - 1);
            if (m_modoVis == ModoVisualizacao::Continuo) {
                const int yBase = calcularScrollVerticalParaPagina(pagFoco, m_pdfView->zoomFactor());
                const int yFinal = yBase + m_scrollOffsetNaPagina;
                if (m_pdfView->verticalScrollBar()) {
                    m_pdfView->verticalScrollBar()->setValue(yFinal);
                }
            }
            m_pdfView->pageNavigator()->jump(pagFoco, QPointF(0, 0), 0);
            m_paginaAtual = pagFoco;
            atualizarInfoNavegacao();
        }
        m_bloquearSyncPagina = false;
    });

    connect(m_splitter, &QSplitter::splitterMoved, this, [this](int, int) {
        if (!m_bloquearSyncPagina) {
            const int pagReal = (m_modoVis == ModoVisualizacao::Continuo)
                                ? obterPaginaVisivelNoModoContinuo()
                                : m_paginaAtual;
            m_paginaSalvaResize = pagReal;
            if (m_modoVis == ModoVisualizacao::Continuo && m_pdfView && m_pdfView->verticalScrollBar()) {
                const int yBase = calcularScrollVerticalParaPagina(pagReal, m_pdfView->zoomFactor());
                m_scrollOffsetNaPagina = qMax(0, m_pdfView->verticalScrollBar()->value() - yBase);
            } else {
                m_scrollOffsetNaPagina = 0;
            }
            m_bloquearSyncPagina = true;
        }
        if (m_timerDebounceResize) {
            m_timerDebounceResize->start(200);
        }
    });
}

// ═════════════════════════════════════════════════════════════════════════════
// Persistência de Layout (QSettings)
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::carregarConfiguracoes()
{
    QSettings settings("Vellum", "LeitorTecnico");
    if (!settings.contains("geometry")) {
        QSettings oldSettings("EnsinadorDeIngles", "LeitorTecnico");
        if (oldSettings.contains("geometry") || oldSettings.contains("iaApiKey")) {
            for (const QString &k : oldSettings.allKeys()) {
                settings.setValue(k, oldSettings.value(k));
            }
        }
    }
    restoreGeometry(settings.value("geometry").toByteArray());
    
    if (settings.contains("splitter")) {
        m_splitter->restoreState(settings.value("splitter").toByteArray());
    }

    const bool painelVisivel = settings.value("painelVisivel", false).toBool();
    m_tabWidget->setVisible(painelVisivel);
    m_actTogglePainel->setChecked(painelVisivel);

    const int abaIndex = settings.value("abaAtual", 0).toInt();
    m_tabWidget->setCurrentIndex(abaIndex);

    const int vozIndex = settings.value("vozIndex", 0).toInt();
    if (m_tabTutor && m_tabTutor->comboVoz() && vozIndex >= 0 && vozIndex < m_tabTutor->comboVoz()->count()) {
        m_tabTutor->comboVoz()->setCurrentIndex(vozIndex);
    }

    const int velIndex = settings.value("velIndex", 1).toInt();
    if (m_tabTutor && m_tabTutor->comboVelocidade() && velIndex >= 0 && velIndex < m_tabTutor->comboVelocidade()->count()) {
        m_tabTutor->comboVelocidade()->setCurrentIndex(velIndex);
    }

    const int nivelIndex = settings.value("nivelIndex", 1).toInt();
    if (m_tabTutor && m_tabTutor->comboNivelTutor() && nivelIndex >= 0 && nivelIndex < m_tabTutor->comboNivelTutor()->count()) {
        m_tabTutor->comboNivelTutor()->setCurrentIndex(nivelIndex);
    }

    m_iaProvedor = settings.value("iaProvedor", "gemini").toString();
    m_iaApiKey   = settings.value("iaApiKey", "").toString();
    m_iaModelo   = settings.value("iaModelo", "gemini-3.5-flash-lite").toString();
    // Migrar modelos obsoletos ou descontinuados para o atual estável e ultra rápido
    static const QSet<QString> obsoletos = {
        "gemini-1.5-flash", "gemini-2.0-flash", "gemini-1.5-pro", "gemini-2.5-flash", "gemini-3.8-flash-lite"
    };
    if (m_iaModelo.isEmpty() || obsoletos.contains(m_iaModelo)) {
        m_iaModelo = "gemini-3.5-flash-lite";
    }

    // Tier de hardware (Padrão: 0 - Nuvem / Gemini para funcionamento instantâneo)
    const int tierIdx = settings.value("tierIndex", 0).toInt();
    if (m_tabConfiguracoes && m_tabConfiguracoes->comboPerfil() && tierIdx >= 0 && tierIdx < m_tabConfiguracoes->comboPerfil()->count())
        m_tabConfiguracoes->comboPerfil()->setCurrentIndex(tierIdx);
    m_ollamaModelTier = settings.value("ollamaModelTier", "llama3").toString();
    m_whisperModelTier = settings.value("whisperModelTier", "base.en").toString();

    // Seletor de IA no chat
    if (m_tabChatIA && m_tabChatIA->comboChatIA()) {
        const int iaIdx = m_tabChatIA->comboChatIA()->findData(m_iaProvedor);
        if (iaIdx >= 0) m_tabChatIA->comboChatIA()->setCurrentIndex(iaIdx);
    }

    if (m_iaProvedor == "gemini") {
        if (m_iaModelo.isEmpty() || m_iaModelo.contains("llama") || m_iaModelo.contains("phi")) {
            m_iaModelo = "gemini-3.5-flash-lite";
        }
    }

    if (m_tabChatIA && m_tabChatIA->lblModeloAtivoChat()) {
        if (m_iaProvedor == "gemini") {
            m_tabChatIA->lblModeloAtivoChat()->setText(m_iaModelo);
        } else {
            m_tabChatIA->lblModeloAtivoChat()->setText(
                m_ollamaModelTier.isEmpty() ? "llama3" : m_ollamaModelTier
            );
        }
    }

    atualizarBadgeModeloAtivo();
}

void MainWindow::salvarConfiguracoes()
{
    QSettings settings("Vellum", "LeitorTecnico");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("splitter", m_splitter->saveState());
    settings.setValue("painelVisivel", m_tabWidget->isVisible());
    settings.setValue("abaAtual", m_tabWidget->currentIndex());
    settings.setValue("vozIndex", (m_tabTutor && m_tabTutor->comboVoz()) ? m_tabTutor->comboVoz()->currentIndex() : 0);
    settings.setValue("velIndex", (m_tabTutor && m_tabTutor->comboVelocidade()) ? m_tabTutor->comboVelocidade()->currentIndex() : 1);
    settings.setValue("nivelIndex", (m_tabTutor && m_tabTutor->comboNivelTutor()) ? m_tabTutor->comboNivelTutor()->currentIndex() : 1);
    settings.setValue("iaProvedor", m_iaProvedor);
    settings.setValue("iaApiKey", m_iaApiKey);
    settings.setValue("iaModelo", m_iaModelo);
    settings.setValue("tierIndex",        (m_tabConfiguracoes && m_tabConfiguracoes->comboPerfil()) ? m_tabConfiguracoes->comboPerfil()->currentIndex() : 0);
    settings.setValue("ollamaModelTier",  m_ollamaModelTier);
    settings.setValue("whisperModelTier", m_whisperModelTier);
}

void MainWindow::onSalvarLayout()
{
    salvarConfiguracoes();
    appendLog("Layout atual salvo com sucesso nas preferências.", "success");
}

void MainWindow::onRestaurarLayoutPadrao()
{
    const int pagSalva = m_paginaAtual;
    m_bloquearSyncPagina = true;

    abrirOuAtivarAba(0);

    const int totalWidth = width();
    m_splitter->setSizes({int(totalWidth * 0.70), int(totalWidth * 0.30)});
    salvarConfiguracoes();
    appendLog("Layout restaurado para o padrão ideal (70% leitor, 30% ferramentas).", "info");

    QTimer::singleShot(80, this, [this, pagSalva]() {
        if (m_pdfDoc && m_pdfDoc->pageCount() > 0) {
            m_pdfView->pageNavigator()->jump(pagSalva, QPointF(0, 0), 0);
            m_paginaAtual = pagSalva;
            atualizarInfoNavegacao();
        }
        m_bloquearSyncPagina = false;
    });
}

void MainWindow::onAplicarProporcao(int leitorPercent)
{
    const int pagSalva = m_paginaAtual;
    m_bloquearSyncPagina = true;

    if (leitorPercent >= 100) {
        m_tabWidget->setVisible(false);
        m_actTogglePainel->setChecked(false);
        appendLog("Modo foco total: leitor em 100% da janela.", "info");
    } else {
        m_tabWidget->setVisible(true);
        m_actTogglePainel->setChecked(true);
        const int totalWidth = width();
        const int wLeitor = int(totalWidth * (leitorPercent / 100.0));
        const int wPainel = totalWidth - wLeitor;
        m_splitter->setSizes({wLeitor, wPainel});
    }

    QTimer::singleShot(80, this, [this, pagSalva]() {
        if (m_pdfDoc && m_pdfDoc->pageCount() > 0) {
            m_pdfView->pageNavigator()->jump(pagSalva, QPointF(0, 0), 0);
            m_paginaAtual = pagSalva;
            atualizarInfoNavegacao();
        }
        m_bloquearSyncPagina = false;
    });
}

void MainWindow::onTogglePainelLateral(bool visivel)
{
    const int pagSalva = m_paginaAtual;
    m_bloquearSyncPagina = true;

    if (visivel && m_tabWidget->count() == 0) {
        abrirOuAtivarAba(0);
        return;
    }

    m_tabWidget->setVisible(visivel);
    m_actTogglePainel->setChecked(visivel);
    if (visivel) {
        const int totalWidth = width();
        if (m_splitter->sizes().value(1, 0) < 100) {
            m_splitter->setSizes({int(totalWidth * 0.70), int(totalWidth * 0.30)});
        }
    }

    QTimer::singleShot(80, this, [this, pagSalva]() {
        if (m_pdfDoc && m_pdfDoc->pageCount() > 0) {
            m_pdfView->pageNavigator()->jump(pagSalva, QPointF(0, 0), 0);
            m_paginaAtual = pagSalva;
            atualizarInfoNavegacao();
        }
        m_bloquearSyncPagina = false;
    });
}

void MainWindow::abrirOuAtivarAba(int index)
{
    QWidget *targetWidget = nullptr;
    QString titulo;

    switch (index) {
        case 0:
            targetWidget = m_tabTraducao;
            titulo = "Tradução";
            break;
        case 1:
            targetWidget = m_tabTutor;
            titulo = "Tutor de Fala";
            break;
        case 2:
            targetWidget = m_tabChatIA;
            titulo = "Assistente IA";
            break;
        case 3:
            targetWidget = m_tabAnotacoes;
            titulo = "Bloco de Notas";
            break;
        case 4:
            targetWidget = m_tabConfiguracoes;
            titulo = "Desempenho & Layout";
            break;
        default:
            return;
    }

    if (!targetWidget) return;

    int tabIdx = m_tabWidget->indexOf(targetWidget);
    if (tabIdx == -1) {
        tabIdx = m_tabWidget->addTab(targetWidget, titulo);

        auto *btnClose = new QToolButton();
        btnClose->setObjectName("btnFecharAbaItem");
        btnClose->setText("✕");
        btnClose->setToolTip(QString("Fechar aba \"%1\"").arg(titulo));
        btnClose->setCursor(Qt::PointingHandCursor);
        connect(btnClose, &QToolButton::clicked, this, [this, targetWidget]() {
            int idx = m_tabWidget->indexOf(targetWidget);
            if (idx != -1) {
                onFecharAbaPainel(idx);
            }
        });
        m_tabWidget->tabBar()->setTabButton(tabIdx, QTabBar::RightSide, btnClose);
    }

    m_tabWidget->setCurrentIndex(tabIdx);
    onTogglePainelLateral(true);
}

void MainWindow::onFecharAbaPainel(int index)
{
    if (index >= 0 && index < m_tabWidget->count()) {
        m_tabWidget->removeTab(index);
        if (m_tabWidget->count() == 0) {
            onTogglePainelLateral(false);
        }
    }
}

void MainWindow::onMudarAbaPainel(int index)
{
    abrirOuAtivarAba(index);
}

// ═════════════════════════════════════════════════════════════════════════════
// Funcionalidades de Tradução e Seleção
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::carregarArquivoPdf(const QString &caminho)
{
    if (caminho.isEmpty()) return;

    m_pdfDoc->close();
    m_pdfDoc->load(caminho);

    if (m_pdfDoc->status() != QPdfDocument::Status::Ready) {
        QMessageBox::critical(this, "Erro", "Não foi possível abrir o arquivo PDF selecionado.");
        atualizarVisibilidadeEmptyState();
        atualizarInfoNavegacao();
        return;
    }

    const int total = m_pdfDoc->pageCount();
    irParaPagina(0);
    atualizarVisibilidadeEmptyState();
    atualizarInfoNavegacao();
    setWindowTitle(QString("%1 — Vellum").arg(QFileInfo(caminho).fileName()));
    appendLog(QString("Documento carregado: %1 (%2 páginas)").arg(QFileInfo(caminho).fileName()).arg(total), "success");
}

void MainWindow::onAbrirPdf()
{
    const QString caminho = QFileDialog::getOpenFileName(
        this, "Selecionar Livro ou Documento PDF", QString(), "Documentos PDF (*.pdf);;Todos os Arquivos (*.*)");
    if (!caminho.isEmpty()) {
        carregarArquivoPdf(caminho);
    }
}

void MainWindow::onFecharPdf()
{
    m_pdfDoc->close();
    m_paginaAtual = 0;
    atualizarVisibilidadeEmptyState();
    atualizarInfoNavegacao();
    setWindowTitle("Vellum — Leitor Técnico & Tutor");
    appendLog("Documento fechado.");
}

void MainWindow::atualizarVisibilidadeEmptyState()
{
    const bool temDoc = (m_pdfDoc && m_pdfDoc->pageCount() > 0);
    if (m_emptyStateWidget) {
        m_emptyStateWidget->setVisible(!temDoc);
    }
    if (m_pdfView) {
        m_pdfView->setVisible(temDoc);
    }
    if (m_pdfView2) {
        m_pdfView2->setVisible(temDoc && m_modoVis == ModoVisualizacao::DuasPaginas);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.toLocalFile().endsWith(".pdf", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            const QString localFile = url.toLocalFile();
            if (localFile.endsWith(".pdf", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                carregarArquivoPdf(localFile);
                return;
            }
        }
    }
}

void MainWindow::onToggleModoCaptura(bool ativo)
{
    m_modoCaptura = ativo;
    if (m_tabTraducao && m_tabTraducao->btnModoCaptura()) {
        m_tabTraducao->btnModoCaptura()->setChecked(ativo);
    }

    if (m_tabTraducao && m_tabTraducao->comboFormatoSelecao() && m_overlaySelecao) {
        m_overlaySelecao->setFormato(
            m_tabTraducao->comboFormatoSelecao()->currentIndex() == 0 
                ? SelectionOverlayWidget::Formato::Retangular 
                : SelectionOverlayWidget::Formato::Livre
        );
    }

    if (ativo) {
        if (m_activePdfView && m_overlaySelecao) {
            m_overlaySelecao->setParent(m_activePdfView->viewport());
            m_overlaySelecao->setGeometry(m_activePdfView->viewport()->rect());
        }
        if (m_tipoCaptura == ModoCapturaRubberBand::Nenhum) {
            m_tipoCaptura = ModoCapturaRubberBand::Traducao;
        }
    } else {
        if (m_overlaySelecao) {
            m_overlaySelecao->cancelar();
        }
        if (m_rubberBand && m_rubberBand->isVisible()) {
            m_rubberBand->hide();
        }
        m_rbOrigin = QPoint();
        m_tipoCaptura = ModoCapturaRubberBand::Nenhum;
    }

    const auto cursor = ativo ? Qt::CrossCursor : Qt::OpenHandCursor;
    m_pdfView->viewport()->setCursor(cursor);
    m_pdfView2->viewport()->setCursor(cursor);

    if (ativo) {
        const QString modoStr = (m_overlaySelecao && m_overlaySelecao->isLivre()) ? "Livre (desenho com mouse)" : "Retangular";
        appendLog(QString("Modo de seleção [%1] ativado (Esc para cancelar).").arg(modoStr));
    }
}


void MainWindow::dispararTraducaoTexto(const QString &textoEn, const QByteArray &imagemB64)
{
    if (!m_net) return;

    const int tierIdx = (m_tabConfiguracoes && m_tabConfiguracoes->comboPerfil()) ? m_tabConfiguracoes->comboPerfil()->currentIndex() : 0;
    QString geminiKey;
    QString modeloOllama;

    if (tierIdx == 0) {
        // Tier 0: Nuvem (Gemini)
        geminiKey = m_iaApiKey.trimmed();
        modeloOllama = m_iaModelo.trimmed();
    } else if (tierIdx == 1) {
        // Tier 1: Básico (phi3:mini local)
        modeloOllama = "phi3:mini";
    } else if (tierIdx == 2) {
        // Tier 2: Médio (llama3.2:3b local)
        modeloOllama = "llama3.2:3b";
    } else {
        // Tier 3: Avançado (llama3 local)
        modeloOllama = "llama3";
    }

    m_timerRequisicaoIA.restart();
    if (m_badgeMetricasIA) {
        m_badgeMetricasIA->setText("Processando Tradução...");
    }

    m_net->traduzirDireto(textoEn, imagemB64, geminiKey, modeloOllama);
}

void MainWindow::onTraduzirDireto()
{
    if (m_textoOcrAtual.startsWith("BASE64:")) {
        const QByteArray b64 = m_textoOcrAtual.mid(7).toLatin1();
        dispararTraducaoTexto(QString(), b64);
    } else if (!m_textoOcrAtual.isEmpty()) {
        dispararTraducaoTexto(m_textoOcrAtual, QByteArray());
    }
}

void MainWindow::onTraducaoDiretaResultado(const QString &textoIngles, const QString &traducaoPortugues, const QString &modeloUsado)
{
    m_textoOriginalEn = textoIngles;
    m_textoTraduzidoPt = traducaoPortugues;
    
    const qint64 elapsedMs = m_timerRequisicaoIA.isValid() ? m_timerRequisicaoIA.elapsed() : 0;
    const QString mod = modeloUsado.isEmpty() ? m_iaModelo : modeloUsado;
    if (m_badgeMetricasIA) {
        m_badgeMetricasIA->setText(QString("%1 • %2ms").arg(mod).arg(elapsedMs));
    }

    if (m_tabTutor) {
        if (m_tabTutor->promptTextoTutor()) {
            m_tabTutor->promptTextoTutor()->setText(textoIngles);
        }
        if (m_tabTutor->btnOuvir())  m_tabTutor->btnOuvir()->setEnabled(!textoIngles.isEmpty());
        if (m_tabTutor->btnGravar()) m_tabTutor->btnGravar()->setEnabled(!textoIngles.isEmpty());
    }

    // Controle rigoroso da Janela Flutuante vs Painel Lateral:
    // A janela flutuante só aparece quando acionada via atalho em tela cheia (1º toque)
    if (m_origemCapturaTelaCheiaFlutuante) {
        m_origemCapturaTelaCheiaFlutuante = false;
        if (m_quickLookupPopover) {
            const QPoint posGlobal = QCursor::pos();
            m_quickLookupPopover->exibirTraducao(textoIngles, traducaoPortugues, mod, posGlobal);
        }
        // Não abre a aba lateral em tela cheia para manter leitura imersiva e sem distrações
    } else {
        if (m_quickLookupPopover && m_quickLookupPopover->isVisible() && m_quickLookupPopover->modoAtual() == QuickLookupPopover::Modo::Traducao) {
            m_quickLookupPopover->atualizarResultadoTraducao(traducaoPortugues, mod);
        }
        // Modo janela normal ou solicitação convencional: atualiza/abre a aba lateral
        if (!isFullScreen() || (m_tabWidget && m_tabWidget->isVisible())) {
            abrirOuAtivarAba(0);
        }
    }

    if (m_tabTraducao) {
        m_tabTraducao->exibirResultado(textoIngles, traducaoPortugues, modeloUsado);
    }

    const QString logMsg = !modeloUsado.isEmpty()
        ? QString("Tradução técnica concluída com sucesso via [%1].").arg(modeloUsado)
        : QStringLiteral("Tradução técnica concluída com sucesso.");
    appendLog(logMsg, "success");
}

void MainWindow::onCopiarTraducao()
{
    if (m_textoTraduzidoPt.isEmpty()) {
        appendLog("Nenhuma tradução disponível para copiar.", "warning");
        return;
    }
    QApplication::clipboard()->setText(m_textoTraduzidoPt);
    appendLog("Tradução copiada para a área de transferência.", "success");
}

// ═════════════════════════════════════════════════════════════════════════════
// Funcionalidades do Tutor de Pronúncia (TTS e Avaliação)
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::onOuvirPronuncia()
{
    QString texto;
    if (m_tabTutor && m_tabTutor->promptTextoTutor()) {
        texto = m_tabTutor->promptTextoTutor()->selectedText();
        if (texto.isEmpty()) {
            texto = m_tabTutor->promptTextoTutor()->text().trimmed();
        }
    } else {
        texto = m_textoOriginalEn.isEmpty()
            ? QApplication::clipboard()->text().trimmed()
            : m_textoOriginalEn;
    }

    if (texto.isEmpty()) {
        appendLog("Selecione um texto antes para ouvir a pronúncia.", "warning");
        return;
    }

    const QString voz = (m_tabTutor && m_tabTutor->comboVoz()) ? m_tabTutor->comboVoz()->currentData().toString() : QStringLiteral("en-US-JennyNeural");
    const QString vel = (m_tabTutor && m_tabTutor->comboVelocidade()) ? m_tabTutor->comboVelocidade()->currentData().toString() : QStringLiteral("+0%");
    const QString velTexto = (m_tabTutor && m_tabTutor->comboVelocidade()) ? m_tabTutor->comboVelocidade()->currentText() : QStringLiteral("1.0x");
    appendLog(QString("Reproduzindo pronúncia neural (%1, velocidade %2)...").arg(voz, velTexto), "info");
    m_net->falarTexto(texto, voz, vel);
}

void MainWindow::onPararAudio()
{
    m_net->pararAudio();
    appendLog("Reprodução de áudio interrompida pelo usuário.", "info");
}

void MainWindow::onFalaIniciada(const QString &voz)
{
    appendLog(QString("Reprodução de áudio iniciada (%1).").arg(voz), "success");
}

void MainWindow::onGravarVozTutor()
{
    if (!m_gravando) {
        QString textoParaLer;
        if (m_tabTutor && m_tabTutor->promptTextoTutor()) {
            textoParaLer = m_tabTutor->promptTextoTutor()->selectedText();
            if (textoParaLer.isEmpty()) {
                textoParaLer = m_tabTutor->promptTextoTutor()->text().trimmed();
            }
        }
        
        if (textoParaLer.isEmpty()) {
            textoParaLer = m_textoOriginalEn;
        }

        if (textoParaLer.isEmpty()) {
            textoParaLer = QApplication::clipboard()->text().trimmed();
        }

        if (textoParaLer.isEmpty()) {
            appendLog("Selecione ou digite um parágrafo para praticar a leitura.", "warning");
            return;
        }
        
        m_textoOriginalEn = textoParaLer; // Salva o texto que será usado para a avaliação

        m_gravando = true;
        if (m_tabTutor) m_tabTutor->setRecordingState(true);
        appendLog("Microfone aberto: leia o texto em inglês em voz alta...");
        m_net->iniciarGravacao();
    } else {
        m_gravando = false;
        if (m_tabTutor) m_tabTutor->setRecordingState(false);
        if (m_quickLookupPopover && m_quickLookupPopover->isVisible()) {
            m_quickLookupPopover->setGravandoTutor(false);
        }
        appendLog(QString("Processando áudio com Whisper (%1)...").arg(m_whisperModelTier));
        m_net->pararGravacao("en", m_whisperModelTier); // Transcreve em inglês usando o modelo Whisper do perfil
    }
}

void MainWindow::onTranscricaoResultado(const QString &texto)
{
    appendLog(QString("Whisper captou: \"%1\"").arg(texto), "info");

    // Envia automaticamente para avaliação fonética do tutor com o nível de exigência escolhido
    if (!m_textoOriginalEn.isEmpty()) {
        const QString nivel = (m_tabTutor && m_tabTutor->comboNivelTutor()) ? m_tabTutor->comboNivelTutor()->currentData().toString() : QStringLiteral("intermediario");
        const QString nivelTexto = (m_tabTutor && m_tabTutor->comboNivelTutor()) ? m_tabTutor->comboNivelTutor()->currentText() : QStringLiteral("Intermediário");
        const int tierIdx = (m_tabConfiguracoes && m_tabConfiguracoes->comboPerfil()) ? m_tabConfiguracoes->comboPerfil()->currentIndex() : 0;
        QString modelo;
        QString apiKey;

        if (tierIdx == 0) {
            apiKey = m_iaApiKey.trimmed();
            modelo = "Gemini";
        } else if (tierIdx == 1) {
            modelo = "phi3:mini";
        } else if (tierIdx == 2) {
            modelo = "llama3.2:3b";
        } else {
            modelo = "llama3";
        }

        appendLog(QString("Avaliando precisão da pronúncia (Nível: %1, Modelo: %2)...").arg(nivelTexto, modelo), "info");
        m_net->avaliarPronuncia(m_textoOriginalEn, texto, nivel, modelo, apiKey);
    }
}

void MainWindow::onAvaliacaoPronunciaResultado(int nota, const QString &feedback, const QString &textoFalado, const QStringList &palavrasAusentes, const QString &htmlDestacado)
{
    if (m_tabTutor) {
        m_tabTutor->exibirAvaliacao(nota, feedback, textoFalado, palavrasAusentes, htmlDestacado);
    }
    if (m_quickLookupPopover && m_quickLookupPopover->isVisible()) {
        m_quickLookupPopover->atualizarResultadoTutor(nota, feedback, textoFalado, palavrasAusentes, htmlDestacado);
    }
    appendLog(QString("Avaliação concluída: %1% de acurácia.").arg(nota), "success");
}

void MainWindow::onPerfilAlterado(int index)
{
    // Tier 0 = Nuvem: toda IA vai pela chave API configurada, sem Ollama
    // Tier 1 = Basico, Tier 2 = Medio, Tier 3 = Avancado
    struct Tier { const char *ollama; const char *whisper; const char *descricao; };
    static const Tier tiers[] = {
        { "",            "base.en",  "Nuvem (sem Ollama, usa API cadastrada)" },
        { "phi3:mini",   "tiny.en",  "Basico (phi3:mini + tiny.en)" },
        { "llama3.2:3b", "base.en",  "Medio (llama3.2:3b + base.en)" },
        { "llama3",      "base.en",  "Avancado (llama3 + base.en)" },
    };
    const int idx = qBound(0, index, 3);
    m_ollamaModelTier  = tiers[idx].ollama;   // vazio no tier Nuvem
    m_whisperModelTier = tiers[idx].whisper;

    QSettings settings("Vellum", "LeitorTecnico");
    settings.setValue("tierIndex",        idx);
    settings.setValue("ollamaModelTier",  m_ollamaModelTier);
    settings.setValue("whisperModelTier", m_whisperModelTier);

    const QString aviso = (idx == 0)
        ? QStringLiteral(" — Certifique-se de ter a chave API configurada.")
        : QStringLiteral(" — Aplicado em tempo real.");
    appendLog(QString("Perfil alterado para: %1%2").arg(tiers[idx].descricao, aviso), "info");

    if (m_tabChatIA && m_tabChatIA->lblModeloAtivoChat() && m_iaProvedor == "ollama") {
        m_tabChatIA->lblModeloAtivoChat()->setText(m_ollamaModelTier.isEmpty() ? "llama3" : m_ollamaModelTier);
    }
    atualizarBadgeModeloAtivo();
}

void MainWindow::atualizarBadgeModeloAtivo()
{
    if (!m_badgeModeloAtivo) return;

    QString modeloNome;
    QString whisperNome;
    const int tierIdx = (m_tabConfiguracoes && m_tabConfiguracoes->comboPerfil()) ? m_tabConfiguracoes->comboPerfil()->currentIndex() : 0;

    switch (tierIdx) {
    case 0:
        if (!m_iaApiKey.trimmed().isEmpty()) {
            modeloNome = "Gemini (Nuvem)";
        } else {
            modeloNome = "Nuvem (Chave necessária)";
        }
        whisperNome = "base.en";
        break;
    case 1:
        modeloNome = "phi3:mini (Local)";
        whisperNome = "tiny.en";
        break;
    case 2:
        modeloNome = "llama3.2:3b (Local)";
        whisperNome = "base.en";
        break;
    case 3:
    default:
        modeloNome = "llama3 (Local)";
        whisperNome = "base.en";
        break;
    }

    m_badgeModeloAtivo->setText(QString("Tradução: %1").arg(modeloNome));
    m_badgeModeloAtivo->setToolTip(
        QString("Motor ativo para tradução técnica e OCR:\n• Tradução: %1\n• Reconhecimento de Voz: %2\n(Altere a qualquer momento na aba Desempenho & Layout ou Configurar API)")
            .arg(modeloNome, whisperNome)
    );

    // No modo local/offline, modelos de texto não suportam visão computacional.
    // As opções de capturar circuito ou anexar imagem só aparecem no modo Nuvem (Gemini).
    const bool isNuvem = (tierIdx == 0);
    PromptInputWidget *chatPrompt = m_tabChatIA ? m_tabChatIA->promptInput() : nullptr;
    if (chatPrompt) {
        chatPrompt->setButtonsVisible(isNuvem, isNuvem);
        if (!isNuvem) {
            chatPrompt->hideImagePreview();
            m_chatImagemBase64.clear();
        }
        chatPrompt->setPlaceholderText(
            isNuvem ? "Pergunte sobre o circuito ou texto técnico... (Enter para enviar, Shift+Enter para nova linha)"
                    : "Pergunte sobre o texto técnico... (Enter para enviar, Shift+Enter para nova linha)"
        );
    }
}

void MainWindow::onToggleTelaCheia()
{
    const int pagReal = (m_pdfDoc && m_pdfDoc->pageCount() > 0)
                        ? ((m_modoVis == ModoVisualizacao::Continuo) ? obterPaginaVisivelNoModoContinuo() : m_paginaAtual)
                        : 0;
    m_paginaSalvaResize = pagReal;

    if (m_modoVis == ModoVisualizacao::Continuo && m_pdfView && m_pdfView->verticalScrollBar()) {
        const int yBase = calcularScrollVerticalParaPagina(pagReal, m_pdfView->zoomFactor());
        m_scrollOffsetNaPagina = qMax(0, m_pdfView->verticalScrollBar()->value() - yBase);
    } else {
        m_scrollOffsetNaPagina = 0;
    }

    m_bloquearSyncPagina = true;

    const bool paraTelaCheia = !isFullScreen();
    if (paraTelaCheia) {
        m_barraVisPreviaVisivel = m_barraVis ? m_barraVis->isVisible() : true;
        if (m_titleBar) m_titleBar->setVisible(false);
        if (m_customMenuBar) m_customMenuBar->setVisible(false);
        if (m_barraVis) m_barraVis->setVisible(false);
        if (statusBar()) statusBar()->setVisible(false);
        showFullScreen();
        appendLog("Modo Tela Cheia ativado: documento e aba ativa em foco total. Pressione F11 ou Esc para restaurar.", "info");
    } else {
        if (m_floatingHud) m_floatingHud->hide();
        if (m_titleBar) m_titleBar->setVisible(true);
        if (m_customMenuBar) m_customMenuBar->setVisible(true);
        if (m_barraVis) m_barraVis->setVisible(m_barraVisPreviaVisivel);
        if (statusBar()) statusBar()->setVisible(true);

        // Sai do modo de tela cheia para normal antes de maximizar
        showNormal();

        // No Linux com janelas sem borda (Frameless), força a geometria do espaço de trabalho da tela
        // para que a janela nunca fique encolhida/comprimida na parte inferior mostrando o desktop
        QScreen *scr = screen();
        if (!scr) scr = QGuiApplication::primaryScreen();
        if (scr) {
            setGeometry(scr->availableGeometry());
        }
        showMaximized();

        if (centralWidget()) {
            centralWidget()->updateGeometry();
        }
        updateGeometry();

        // Confirmação para gerenciadores de janelas que demoram a processar o cancelamento do FullScreen
        QTimer::singleShot(50, this, [this]() {
            if (!isFullScreen()) {
                QScreen *s = screen();
                if (!s) s = QGuiApplication::primaryScreen();
                if (s) {
                    const QRect av = s->availableGeometry();
                    if (geometry().width() < av.width() || geometry().height() < av.height()) {
                        setGeometry(av);
                    }
                }
                showMaximized();
            }
        });

        appendLog("Modo Tela Cheia desativado: barras restauradas.", "info");
    }

    if (m_timerDebounceResize) {
        m_timerDebounceResize->stop();
        m_timerDebounceResize->start(350);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    if (!m_bloquearSyncPagina && m_pdfDoc && m_pdfDoc->pageCount() > 0) {
        const int pagReal = (m_modoVis == ModoVisualizacao::Continuo) 
                            ? obterPaginaVisivelNoModoContinuo() 
                            : m_paginaAtual;
        m_paginaSalvaResize = pagReal;
        if (m_modoVis == ModoVisualizacao::Continuo && m_pdfView && m_pdfView->verticalScrollBar()) {
            const int yBase = calcularScrollVerticalParaPagina(pagReal, m_pdfView->zoomFactor());
            m_scrollOffsetNaPagina = qMax(0, m_pdfView->verticalScrollBar()->value() - yBase);
        } else {
            m_scrollOffsetNaPagina = 0;
        }
        m_bloquearSyncPagina = true;
    }
    QMainWindow::resizeEvent(event);
    if (m_timerDebounceResize) {
        m_timerDebounceResize->start(300);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // Navegação de páginas com setas do teclado nos modos individual e lado a lado
    if (m_modoVis == ModoVisualizacao::UmaPagina || m_modoVis == ModoVisualizacao::DuasPaginas) {
        QWidget *focused = focusWidget();
        bool isInput = false;
        if (auto *le = qobject_cast<QLineEdit*>(focused)) isInput = !le->isReadOnly();
        else if (auto *te = qobject_cast<QTextEdit*>(focused)) isInput = !te->isReadOnly();
        else if (auto *pe = qobject_cast<QPlainTextEdit*>(focused)) isInput = !pe->isReadOnly();
        else if (qobject_cast<QAbstractSpinBox*>(focused)) isInput = true;

        if (!isInput) {
            if (event->key() == Qt::Key_Left || event->key() == Qt::Key_PageUp) {
                onPaginaAnterior();
                event->accept();
                return;
            } else if (event->key() == Qt::Key_Right || event->key() == Qt::Key_PageDown) {
                onPaginaProxima();
                event->accept();
                return;
            }
        }
    }

    if (event->key() == Qt::Key_Escape) {
        if (m_overlaySelecao && m_overlaySelecao->isAtivo()) {
            m_overlaySelecao->cancelar();
            appendLog("Desenho da seleção cancelado. Selecione novamente quando desejar.", "info");
            event->accept();
            return;
        }
        if (m_rubberBand && m_rubberBand->isVisible()) {
            m_rubberBand->hide();
            m_rbOrigin = QPoint();
            appendLog("Desenho da seleção cancelado. Selecione novamente quando desejar.", "info");
            event->accept();
            return;
        }
        if (m_modoCaptura) {
            onToggleModoCaptura(false);
            m_tipoCaptura = ModoCapturaRubberBand::Nenhum;
            m_origemCapturaTelaCheiaFlutuante = false;
            m_ultimoAtalhoTelaCheia = -1;
            appendLog("Modo de seleção desativado.", "info");
            event->accept();
            return;
        }
        if (isFullScreen()) {
            onToggleTelaCheia();
            event->accept();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

// ═════════════════════════════════════════════════════════════════════════════
// Manipulação de Eventos do Mouse (Seleção Rubber Band e Desenho Livre)
// ═════════════════════════════════════════════════════════════════════════════

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Eventos do m_chatInput removidos pois o PromptInputWidget gerencia isso.
    QPdfView *view = nullptr;
    if (watched == m_pdfView || watched == m_pdfView->viewport())   view = m_pdfView;
    if (watched == m_pdfView2 || watched == m_pdfView2->viewport()) view = m_pdfView2;

    if (!view) return QMainWindow::eventFilter(watched, event);

    m_activePdfView = view;

    switch (event->type()) {
    case QEvent::KeyPress: {
        auto *ke = static_cast<QKeyEvent*>(event);
        if (m_modoVis == ModoVisualizacao::UmaPagina || m_modoVis == ModoVisualizacao::DuasPaginas) {
            if (ke->key() == Qt::Key_Left || ke->key() == Qt::Key_PageUp) {
                onPaginaAnterior();
                return true;
            } else if (ke->key() == Qt::Key_Right || ke->key() == Qt::Key_PageDown) {
                onPaginaProxima();
                return true;
            }
        }

        if (ke->key() == Qt::Key_Escape) {
            if (m_overlaySelecao && m_overlaySelecao->isAtivo()) {
                m_overlaySelecao->cancelar();
                appendLog("Desenho da seleção cancelado. Selecione novamente quando desejar.", "info");
                return true;
            }
            if (m_rubberBand && m_rubberBand->isVisible()) {
                m_rubberBand->hide();
                m_rbOrigin = QPoint();
                appendLog("Desenho da seleção cancelado. Selecione novamente quando desejar.", "info");
                return true;
            }
            if (m_modoCaptura) {
                onToggleModoCaptura(false);
                m_tipoCaptura = ModoCapturaRubberBand::Nenhum;
                m_origemCapturaTelaCheiaFlutuante = false;
                m_ultimoAtalhoTelaCheia = -1;
                appendLog("Modo de seleção desativado.", "info");
                return true;
            }
            if (isFullScreen()) {
                onToggleTelaCheia();
                return true;
            }
        }
        break;
    }
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent*>(event);
        // Modo de captura (overlay retangular ou livre)
        if (me->button() == Qt::LeftButton && m_modoCaptura) {
            if (m_overlaySelecao) {
                m_overlaySelecao->setParent(view->viewport());
                m_overlaySelecao->setGeometry(view->viewport()->rect());
                m_overlaySelecao->iniciar(me->pos());
            }
            return true;
        }
        // Modo pan (mao): clique e segura para arrastar
        if (me->button() == Qt::LeftButton && !m_modoCaptura) {
            m_panModo  = true;
            m_panOrigin = me->pos();
            view->viewport()->setCursor(Qt::ClosedHandCursor);
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent*>(event);

        // Conceito 1: Revela o Floating HUD quando o cursor se aproxima do rodapé em tela cheia
        if (isFullScreen() && m_floatingHud) {
            const int mouseY = me->globalPosition().toPoint().y();
            const int screenBottom = screen() ? screen()->geometry().bottom() : height();
            if (mouseY >= (screenBottom - 130)) {
                if (m_pdfDoc && m_pdfDoc->pageCount() > 0) {
                    m_floatingHud->atualizarPagina(m_paginaAtual + 1, m_pdfDoc->pageCount());
                }
                if (m_pdfView) {
                    m_floatingHud->atualizarZoom(m_pdfView->zoomFactor());
                }
                m_floatingHud->adjustSize();
                const int hudW = m_floatingHud->width();
                const int hudH = m_floatingHud->height();
                m_floatingHud->move((width() - hudW) / 2, height() - hudH - 24);
                m_floatingHud->raise();
                m_floatingHud->mostrarTemporariamente();
            }
        }

        // Arrastar overlay de selecao
        if (m_modoCaptura && m_overlaySelecao && m_overlaySelecao->isAtivo()) {
            m_overlaySelecao->atualizar(me->pos());
            return true;
        }
        // Arrastar pan
        if (m_panModo && (me->buttons() & Qt::LeftButton)) {
            const QPoint delta = me->pos() - m_panOrigin;
            m_panOrigin = me->pos();
            auto *hsb = view->horizontalScrollBar();
            auto *vsb = view->verticalScrollBar();
            if (hsb) hsb->setValue(hsb->value() - delta.x());
            if (vsb) vsb->setValue(vsb->value() - delta.y());
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent*>(event);
        // Soltar selecao
        if (me->button() == Qt::LeftButton && m_modoCaptura && m_overlaySelecao && m_overlaySelecao->isAtivo()) {
            const QRect rect = m_overlaySelecao->boundingBox();
            const QPolygonF poligono = m_overlaySelecao->poligono();
            const bool isLivre = m_overlaySelecao->isLivre();
            m_overlaySelecao->finalizar();
            const auto tipoCaptura = m_tipoCaptura;
            onToggleModoCaptura(false);

            if (rect.width() > 10 && rect.height() > 10) {
                const QString textoExtraido = extrairTextoOcrDaRegiao(rect, poligono, isLivre);
                QByteArray bytesImg;
                if (textoExtraido.isEmpty() || tipoCaptura == ModoCapturaRubberBand::CircuitoChat) {
                    const QPixmap pix = view->viewport()->grab(rect);
                    QBuffer buf(&bytesImg);
                    buf.open(QIODevice::WriteOnly);
                    pix.save(&buf, "PNG");
                }

                if (m_origemCapturaTelaCheiaFlutuante) {
                    const QPoint posGlobal = QCursor::pos();
                    m_origemCapturaTelaCheiaFlutuante = false;

                    if (m_ultimoAtalhoTelaCheia == 1) { // Tutor de Pronúncia (Ctrl+U)
                        if (m_quickLookupPopover) {
                            m_quickLookupPopover->exibirTutor(textoExtraido, posGlobal);
                        }
                        appendLog("Tutor de Fala flutuante exibido com sucesso.", "success");
                    } else if (m_ultimoAtalhoTelaCheia == 2) { // Assistente IA (Ctrl+I)
                        const QString b64 = bytesImg.isEmpty() ? QString() : QString::fromLatin1(bytesImg.toBase64());
                        m_chatImagemBase64 = b64;
                        if (m_quickLookupPopover) {
                            m_quickLookupPopover->exibirAssistenteIA(textoExtraido, b64, posGlobal);
                        }
                        appendLog("Assistente IA flutuante exibido com sucesso.", "success");
                    } else if (m_ultimoAtalhoTelaCheia == 3) { // Bloco de Notas (Ctrl+N)
                        if (m_quickLookupPopover) {
                            m_quickLookupPopover->exibirAnotacoes(textoExtraido, posGlobal);
                        }
                        appendLog("Bloco de Notas flutuante exibido com sucesso.", "success");
                    } else { // Tradução Técnica (Ctrl+T)
                        if (!textoExtraido.isEmpty()) {
                            m_textoOcrAtual = textoExtraido;
                            if (m_tabTraducao && m_tabTraducao->btnTraduzir()) m_tabTraducao->btnTraduzir()->setEnabled(true);
                            appendLog(QString("Texto extraído instantaneamente do cache OCR (%1 palavras).").arg(textoExtraido.split(' ', Qt::SkipEmptyParts).size()), "success");
                        } else {
                            m_textoOcrAtual = "BASE64:" + bytesImg.toBase64();
                            if (m_tabTraducao && m_tabTraducao->btnTraduzir()) m_tabTraducao->btnTraduzir()->setEnabled(true);
                        }
                        m_origemCapturaTelaCheiaFlutuante = true; // Mantém ativo para onTraducaoDiretaResultado
                        onTraduzirDireto();
                    }
                } else {
                    // Modo tradicional (fora de tela cheia ou pelo botão Modo Seleção da barra lateral)
                    if (tipoCaptura == ModoCapturaRubberBand::CircuitoChat) {
                        m_chatImagemBase64 = QString::fromLatin1(bytesImg.toBase64());
                        if (m_tabChatIA && m_tabChatIA->promptInput()) {
                            m_tabChatIA->promptInput()->showImagePreview(m_chatImagemBase64, QString("Circuito capturado (%1x%2 px)").arg(rect.width()).arg(rect.height()));
                        }
                        abrirOuAtivarAba(2); // Aba do Chat IA
                        appendLog("Circuito capturado e anexado ao Assistente IA.", "success");
                    } else {
                        if (!textoExtraido.isEmpty()) {
                            m_textoOcrAtual = textoExtraido;
                            if (m_tabTraducao && m_tabTraducao->btnTraduzir()) m_tabTraducao->btnTraduzir()->setEnabled(true);
                            appendLog(QString("Texto extraído instantaneamente do cache OCR (%1 palavras).").arg(textoExtraido.split(' ', Qt::SkipEmptyParts).size()), "success");
                        } else {
                            m_textoOcrAtual = "BASE64:" + bytesImg.toBase64();
                            if (m_tabTraducao && m_tabTraducao->btnTraduzir()) m_tabTraducao->btnTraduzir()->setEnabled(true);
                        }

                        if (m_tabTraducao && m_tabTraducao->chkTraducaoAuto() && m_tabTraducao->chkTraducaoAuto()->isChecked()) {
                            onTraduzirDireto();
                        } else {
                            appendLog("Região capturada. Clique em 'Traduzir Seleção'.");
                        }
                    }
                }
            }

            m_tipoCaptura = ModoCapturaRubberBand::Nenhum;
            return true;
        }

        // Soltar pan
        if (me->button() == Qt::LeftButton && m_panModo) {
            m_panModo = false;
            view->viewport()->setCursor(Qt::OpenHandCursor);
            return true;
        }
        break;
    }
    case QEvent::Enter: {
        // Cursor de mao aberta quando nao esta capturando
        if (!m_modoCaptura)
            view->viewport()->setCursor(Qt::OpenHandCursor);
        break;
    }
    case QEvent::Leave: {
        if (!m_modoCaptura && !m_panModo)
            view->viewport()->setCursor(Qt::ArrowCursor);
        break;
    }
    case QEvent::Wheel: {
        auto *we = static_cast<QWheelEvent*>(event);
        if (we->modifiers() & Qt::ControlModifier) {
            const int delta = we->angleDelta().y();
            if (delta > 0) zoomDelta(0.12);
            else if (delta < 0) zoomDelta(-0.12);
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QMainWindow::eventFilter(watched, event);
}

// ═════════════════════════════════════════════════════════════════════════════
// Navegação de Páginas e Zoom
// ═════════════════════════════════════════════════════════════════════════════

int MainWindow::calcularScrollVerticalParaPagina(int pagina, qreal zoom) const
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return 0;
    pagina = qBound(0, pagina, m_pdfDoc->pageCount() - 1);

    qreal y = m_pdfView ? m_pdfView->documentMargins().top() : 0;
    const int spacing = m_pdfView ? m_pdfView->pageSpacing() : 4;

    for (int i = 0; i < pagina; ++i) {
        const QSizeF sz = m_pdfDoc->pagePointSize(i);
        y += (sz.height() * zoom) + spacing;
    }
    return qRound(y);
}

int MainWindow::obterPaginaVisivelNoModoContinuo() const
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0 || !m_pdfView) return 0;
    if (m_modoVis != ModoVisualizacao::Continuo) return m_paginaAtual;

    const auto *vsb = m_pdfView->verticalScrollBar();
    if (!vsb) return m_paginaAtual;

    const int scrollY = vsb->value();
    qreal yAcumulado = m_pdfView->documentMargins().top();
    const int spacing = m_pdfView->pageSpacing();
    const qreal zoom = m_pdfView->zoomFactor();
    const int total = m_pdfDoc->pageCount();

    for (int i = 0; i < total; ++i) {
        const QSizeF sz = m_pdfDoc->pagePointSize(i);
        const qreal h = (sz.height() * zoom) + spacing;
        if (scrollY < yAcumulado + (h * 0.7)) {
            return i;
        }
        yAcumulado += h;
    }
    return total - 1;
}

void MainWindow::forcarNavegacaoPagina(int pagina)
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;
    const int total = m_pdfDoc->pageCount();
    pagina = qBound(0, pagina, total - 1);

    m_bloquearSyncPagina = true;
    m_paginaAtual = pagina;

    if (m_modoVis == ModoVisualizacao::Continuo) {
        const int y = calcularScrollVerticalParaPagina(pagina, m_pdfView->zoomFactor());
        if (m_pdfView->verticalScrollBar()) {
            m_pdfView->verticalScrollBar()->setValue(y);
        }
    }
    m_pdfView->pageNavigator()->jump(pagina, {}, 0);

    if (m_modoVis == ModoVisualizacao::DuasPaginas) {
        if (pagina + 1 < total) {
            m_pdfView2->pageNavigator()->jump(pagina + 1, {}, 0);
            m_pdfView2->setVisible(true);
        } else {
            m_pdfView2->setVisible(false);
        }
    } else {
        m_pdfView2->setVisible(false);
    }

    atualizarInfoNavegacao();

    // Mantém o bloqueio ativo durante o ciclo de layout do Qt para evitar eventos espúrios
    QTimer::singleShot(150, this, [this, pagina]() {
        if (m_paginaAtual == pagina) {
            m_bloquearSyncPagina = false;
        }
    });
}

void MainWindow::irParaPagina(int pagina)
{
    forcarNavegacaoPagina(pagina);
}

void MainWindow::setModoVisualizacao(ModoVisualizacao modo)
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;

    // Obtém com máxima precisão a página ativa real
    int pagAlvo = m_paginaAtual;
    if (m_pdfView && m_pdfView->pageNavigator()) {
        const int pNav = m_pdfView->pageNavigator()->currentPage();
        if (pNav >= 0 && pNav < m_pdfDoc->pageCount()) {
            pagAlvo = pNav;
        }
    }
    pagAlvo = qBound(0, pagAlvo, m_pdfDoc->pageCount() - 1);

    m_modoVis = modo;
    m_bloquearSyncPagina = true;
    m_paginaAtual = pagAlvo;

    m_btnModo1Pag->setChecked(modo == ModoVisualizacao::UmaPagina);
    m_btnModo2Pag->setChecked(modo == ModoVisualizacao::DuasPaginas);
    m_btnModoCont->setChecked(modo == ModoVisualizacao::Continuo);

    switch (modo) {
    case ModoVisualizacao::UmaPagina:
        m_pdfView->setPageMode(QPdfView::PageMode::SinglePage);
        m_pdfView2->setVisible(false);
        break;
    case ModoVisualizacao::DuasPaginas:
        m_pdfView->setPageMode(QPdfView::PageMode::SinglePage);
        break;
    case ModoVisualizacao::Continuo:
        m_pdfView->setPageMode(QPdfView::PageMode::MultiPage);
        m_pdfView2->setVisible(false);
        break;
    }

    m_paginaAtual = pagAlvo;
    atualizarInfoNavegacao();

    auto executarAncoragem = [this, pagAlvo]() {
        if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;
        if (m_modoVis == ModoVisualizacao::Continuo) {
            const int y = calcularScrollVerticalParaPagina(pagAlvo, m_pdfView->zoomFactor());
            if (m_pdfView->verticalScrollBar()) {
                m_pdfView->verticalScrollBar()->setValue(y);
            }
        }
        m_pdfView->pageNavigator()->jump(pagAlvo, {}, 0);

        if (m_modoVis == ModoVisualizacao::DuasPaginas) {
            if (pagAlvo + 1 < m_pdfDoc->pageCount()) {
                m_pdfView2->pageNavigator()->jump(pagAlvo + 1, {}, 0);
                m_pdfView2->setVisible(true);
            } else {
                m_pdfView2->setVisible(false);
            }
        } else {
            m_pdfView2->setVisible(false);
        }
        m_paginaAtual = pagAlvo;
        atualizarInfoNavegacao();
    };

    // Ancoragem sequencial durante a reconstrução assíncrona do layout MultiPage pelo Qt
    executarAncoragem();
    QTimer::singleShot(60,  this, executarAncoragem);
    QTimer::singleShot(180, this, executarAncoragem);
    QTimer::singleShot(350, this, executarAncoragem);
    QTimer::singleShot(600, this, [this, executarAncoragem]() {
        executarAncoragem();
        m_bloquearSyncPagina = false;
    });
}

void MainWindow::onModo1PagClicado() { setModoVisualizacao(ModoVisualizacao::UmaPagina); }
void MainWindow::onModo2PagClicado() { setModoVisualizacao(ModoVisualizacao::DuasPaginas); }
void MainWindow::onModoContClicado() { setModoVisualizacao(ModoVisualizacao::Continuo); }

void MainWindow::onPaginaAnterior()
{
    const int passo = (m_modoVis == ModoVisualizacao::DuasPaginas) ? 2 : 1;
    irParaPagina(m_paginaAtual - passo);
}

void MainWindow::onPaginaProxima()
{
    const int passo = (m_modoVis == ModoVisualizacao::DuasPaginas) ? 2 : 1;
    irParaPagina(m_paginaAtual + passo);
}

void MainWindow::onPaginaSpinChanged(int pag)
{
    irParaPagina(pag - 1);
}

void MainWindow::atualizarInfoNavegacao()
{
    const int total = m_pdfDoc ? m_pdfDoc->pageCount() : 0;
    const bool temDoc = (total > 0);
    m_btnPagAnterior->setEnabled(temDoc && m_paginaAtual > 0);
    m_btnPagProxima->setEnabled(temDoc && m_paginaAtual < total - 1);
    m_spinPagina->setEnabled(temDoc);

    m_spinPagina->blockSignals(true);
    if (temDoc) {
        m_spinPagina->setRange(1, total);
        m_spinPagina->setValue(m_paginaAtual + 1);
        if (m_lblTotalPag) {
            m_lblTotalPag->setText(QString("/ %1").arg(total));
        }
    } else {
        m_spinPagina->setRange(0, 0);
        m_spinPagina->setValue(0);
        if (m_lblTotalPag) {
            m_lblTotalPag->setText("/ 0");
        }
    }
    m_spinPagina->blockSignals(false);

    // ── OCR Preemptivo em Segundo Plano com Debounce e Limpeza Temporal ──
    if (temDoc) {
        if (m_paginaAnteriorOcr != -1 && m_paginaAnteriorOcr != m_paginaAtual) {
            if (!m_timerLimpezaCacheOcr) {
                m_timerLimpezaCacheOcr = new QTimer(this);
                m_timerLimpezaCacheOcr->setSingleShot(true);
                connect(m_timerLimpezaCacheOcr, &QTimer::timeout, this, [this]() {
                    const QList<int> chaves = m_cacheOcrPaginas.keys();
                    for (int p : chaves) {
                        if (p != m_paginaAtual) {
                            m_cacheOcrPaginas.remove(p);
                        }
                    }
                });
            }
            m_timerLimpezaCacheOcr->start(8000); // Descarta páginas anteriores após 8 segundos
        }
        m_paginaAnteriorOcr = m_paginaAtual;

        if (!m_timerOcrPreemptivo) {
            m_timerOcrPreemptivo = new QTimer(this);
            m_timerOcrPreemptivo->setSingleShot(true);
            connect(m_timerOcrPreemptivo, &QTimer::timeout, this, &MainWindow::executarOcrPreemptivoPaginaAtual);
        }
        // Debounce de 1.8 segundos: se o usuário estiver rolando rápido, não processa repetidamente
        m_timerOcrPreemptivo->start(1800);
    }
}

void MainWindow::executarOcrPreemptivoPaginaAtual()
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;
    const int pag = m_paginaAtual;
    if (pag < 0 || pag >= m_pdfDoc->pageCount()) return;

    // Se já estiver no cache, não reprocessa
    if (m_cacheOcrPaginas.contains(pag)) {
        return;
    }

    const QSizeF ptSize = m_pdfDoc->pagePointSize(pag);
    if (ptSize.isEmpty()) return;

    // Renderiza em resolução nítida (escala 2.0x, ~150-200 DPI) para precisão de OCR
    const qreal scale = 2.0;
    const QSize imgSize = (ptSize * scale).toSize();
    const QImage img = m_pdfDoc->render(pag, imgSize);
    if (img.isNull()) return;

    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");

    m_net->executarOcrPaginaCompleta(pag, bytes.toBase64());
}

void MainWindow::onOcrPaginaCompletaResultado(int pagina, int largura, int altura, const QJsonArray &palavras, const QString &textoCompleto)
{
    CacheOcrPagina cache;
    cache.pagina = pagina;
    cache.largura = largura;
    cache.altura = altura;
    cache.textoCompleto = textoCompleto;
    cache.criadoEm = QDateTime::currentDateTime();

    for (const auto &val : palavras) {
        const QJsonObject obj = val.toObject();
        OcrPalavra p;
        p.texto = obj["text"].toString();
        p.box = QRect(obj["x"].toInt(), obj["y"].toInt(), obj["w"].toInt(), obj["h"].toInt());
        cache.palavras.append(p);
    }

    m_cacheOcrPaginas.insert(pagina, cache);
}

QString MainWindow::extrairTextoOcrDaRegiao(const QRect &rect, const QPolygonF &poligono, bool isLivre)
{
    int pagAlvo = m_paginaAtual;
    if (m_activePdfView == m_pdfView2) {
        pagAlvo = m_paginaAtual + 1;
    }

    if (!m_cacheOcrPaginas.contains(pagAlvo)) {
        return QString();
    }

    const CacheOcrPagina &cache = m_cacheOcrPaginas[pagAlvo];
    if (cache.largura <= 0 || cache.altura <= 0 || cache.palavras.isEmpty()) {
        return QString();
    }

    QPdfView *view = m_activePdfView ? m_activePdfView : m_pdfView;
    if (!view || !m_pdfDoc || pagAlvo >= m_pdfDoc->pageCount()) return QString();

    const QSizeF ptSize = m_pdfDoc->pagePointSize(pagAlvo);
    if (ptSize.isEmpty()) return QString();

    const qreal zoom = view->zoomFactor();
    const qreal pageWidthOnScreen = ptSize.width() * zoom;
    const qreal pageHeightOnScreen = ptSize.height() * zoom;

    qreal pageLeft = 0;
    qreal pageTop = 0;
    const int vpW = view->viewport()->width();
    const int vpH = view->viewport()->height();

    if (m_modoVis == ModoVisualizacao::Continuo) {
        const int scrollY = view->verticalScrollBar() ? view->verticalScrollBar()->value() : 0;
        const int scrollX = view->horizontalScrollBar() ? view->horizontalScrollBar()->value() : 0;
        pageTop = calcularScrollVerticalParaPagina(pagAlvo, zoom) - scrollY;
        pageLeft = (vpW > pageWidthOnScreen) ? ((vpW - pageWidthOnScreen) / 2.0) : -scrollX;
    } else {
        const int scrollY = view->verticalScrollBar() ? view->verticalScrollBar()->value() : 0;
        const int scrollX = view->horizontalScrollBar() ? view->horizontalScrollBar()->value() : 0;
        pageTop = (vpH > pageHeightOnScreen) ? ((vpH - pageHeightOnScreen) / 2.0) : -scrollY;
        pageLeft = (vpW > pageWidthOnScreen) ? ((vpW - pageWidthOnScreen) / 2.0) : -scrollX;
    }

    struct PalavraSelecionada {
        QString texto;
        int x;
        int y;
    };
    QList<PalavraSelecionada> selecionadas;

    for (const auto &p : cache.palavras) {
        if (p.texto.trimmed().isEmpty()) continue;

        const qreal normCx = (p.box.x() + (p.box.width() / 2.0)) / static_cast<qreal>(cache.largura);
        const qreal normCy = (p.box.y() + (p.box.height() / 2.0)) / static_cast<qreal>(cache.altura);

        const qreal sx = pageLeft + (normCx * pageWidthOnScreen);
        const qreal sy = pageTop + (normCy * pageHeightOnScreen);
        const QPointF ptTela(sx, sy);

        bool contido = false;
        if (isLivre && poligono.size() >= 3) {
            contido = poligono.containsPoint(ptTela, Qt::OddEvenFill);
        } else {
            contido = rect.contains(ptTela.toPoint());
        }

        if (contido) {
            selecionadas.append({ p.texto, p.box.x(), p.box.y() });
        }
    }

    if (selecionadas.isEmpty()) return QString();

    // Ordenação linear em ordem de leitura natural (linha por linha, esquerda para direita)
    std::sort(selecionadas.begin(), selecionadas.end(), [](const PalavraSelecionada &a, const PalavraSelecionada &b) {
        const int linhaA = a.y / 24;
        const int linhaB = b.y / 24;
        if (linhaA != linhaB) return linhaA < linhaB;
        return a.x < b.x;
    });

    QStringList resultado;
    for (const auto &p : selecionadas) {
        resultado.append(p.texto);
    }
    return resultado.join(' ');
}

void MainWindow::executarComandoAba(int index)
{
    const qint64 agora = QDateTime::currentMSecsSinceEpoch();

    if (isFullScreen()) {
        // Se a barra lateral já estiver aberta e exibindo a aba solicitada, pressionar novamente fecha para restaurar tela limpa
        if (m_tabWidget->isVisible() && m_tabWidget->currentIndex() == index) {
            onTogglePainelLateral(false);
            m_ultimoAtalhoTelaCheia = -1;
            m_contagemAtalhoTelaCheia = 0;
            return;
        }

        // Verifica se é o segundo toque consecutivo do mesmo atalho (dentro de 4s ou enquanto em modo de seleção)
        const bool segundoToque = (m_ultimoAtalhoTelaCheia == index &&
                                  (m_modoCaptura || (agora - m_tempoUltimoAtalhoTelaCheia < 4000)));

        if (segundoToque) {
            // 2º Toque em tela cheia: Abre a aba lateral inteira
            if (m_modoCaptura) {
                onToggleModoCaptura(false);
            }
            m_ultimoAtalhoTelaCheia = -1;
            m_contagemAtalhoTelaCheia = 0;
            m_origemCapturaTelaCheiaFlutuante = false;
            abrirOuAtivarAba(index);
            appendLog(QString("Painel lateral exibido em tela cheia (aba %1).").arg(index + 1), "info");
            return;
        }

        // 1º Toque em tela cheia:
        m_ultimoAtalhoTelaCheia = index;
        m_contagemAtalhoTelaCheia = 1;
        m_tempoUltimoAtalhoTelaCheia = agora;
        m_origemCapturaTelaCheiaFlutuante = true;

        if (index == 2) {
            m_tipoCaptura = ModoCapturaRubberBand::CircuitoChat;
        } else {
            m_tipoCaptura = ModoCapturaRubberBand::Traducao;
        }

        onToggleModoCaptura(true);

        const QString formatoStr = (m_overlaySelecao && m_overlaySelecao->isLivre()) ? "Formato Livre" : "Retângulo";
        appendLog(QString("Modo de seleção [%1] ativado. Selecione na tela (ou pressione novamente para abrir o painel lateral).").arg(formatoStr), "info");
    } else {
        // Modo janela normal: Abre ou alterna a aba lateral normalmente
        m_origemCapturaTelaCheiaFlutuante = false;
        abrirOuAtivarAba(index);
    }
}


void MainWindow::aplicarZoom(qreal fator)
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;

    // 1. Identifica a página onde o usuário está lendo no exato momento
    int pagFoco = m_paginaAtual;
    if (m_pdfView && m_pdfView->pageNavigator()) {
        const int pNav = m_pdfView->pageNavigator()->currentPage();
        if (pNav >= 0 && pNav < m_pdfDoc->pageCount()) {
            pagFoco = pNav;
        }
    }
    pagFoco = qBound(0, pagFoco, m_pdfDoc->pageCount() - 1);
    m_bloquearSyncPagina = true;

    fator = qBound(0.5, fator, 4.0);

    m_pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
    m_pdfView->setZoomFactor(fator);

    if (m_pdfView2->isVisible()) {
        m_pdfView2->setZoomMode(QPdfView::ZoomMode::Custom);
        m_pdfView2->setZoomFactor(fator);
    }
    atualizarLabelZoom();

    // 2. Trava e ancora diretamente e imediatamente EM CIMA da página atual
    if (m_modoVis == ModoVisualizacao::Continuo) {
        const int y = calcularScrollVerticalParaPagina(pagFoco, fator);
        if (m_pdfView->verticalScrollBar()) {
            m_pdfView->verticalScrollBar()->setValue(y);
        }
    }
    m_pdfView->pageNavigator()->jump(pagFoco, {}, 0);
    m_paginaAtual = pagFoco;
    atualizarInfoNavegacao();

    if (m_modoVis != ModoVisualizacao::Continuo) {
        m_bloquearSyncPagina = false;
        return;
    }

    // 3. Em modo contínuo, reinicia o temporizador de debounce único:
    // Mantém o bloqueio ativo durante múltiplos passos rápidos de zoom (botão ou roda do mouse)
    // e reancora na página ao finalizar a operação
    if (m_timerZoomDebounce) {
        m_timerZoomDebounce->stop();
        m_timerZoomDebounce->start(200);
    }
}

void MainWindow::zoomDelta(qreal delta)
{
    aplicarZoom(m_pdfView->zoomFactor() + delta);
}

void MainWindow::onZoomMenos() { zoomDelta(-0.15); }
void MainWindow::onZoomMais()  { zoomDelta(0.15); }
void MainWindow::onZoomReset() { aplicarZoom(1.0); }

void MainWindow::onAjustarLargura()
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;
    const int pagFoco = qBound(0, m_paginaAtual, m_pdfDoc->pageCount() - 1);

    const int viewW = m_pdfView->viewport()->width();
    const QSizeF tamPag = m_pdfDoc->pagePointSize(pagFoco);
    qreal fator = 1.0;
    if (tamPag.width() > 0) {
        // Desconta scrollbar vertical e margem lateral (~24px)
        const qreal larguraUtil = qMax(100.0, (qreal)viewW - 24.0);
        fator = qBound(0.5, larguraUtil / tamPag.width(), 4.0);
    }

    aplicarZoom(fator);
}

void MainWindow::onAjustarPagina()
{
    if (!m_pdfDoc || m_pdfDoc->pageCount() == 0) return;
    const int pagFoco = qBound(0, m_paginaAtual, m_pdfDoc->pageCount() - 1);

    const int viewW = m_pdfView->viewport()->width();
    const int viewH = m_pdfView->viewport()->height();
    const QSizeF tamPag = m_pdfDoc->pagePointSize(pagFoco);
    qreal fator = 1.0;
    if (tamPag.width() > 0 && tamPag.height() > 0) {
        const qreal fW = qMax(100.0, (qreal)viewW - 24.0) / tamPag.width();
        const qreal fH = qMax(100.0, (qreal)viewH - 24.0) / tamPag.height();
        fator = qBound(0.5, qMin(fW, fH), 4.0);
    }

    aplicarZoom(fator);
}

void MainWindow::atualizarLabelZoom()
{
    const int perc = qRound(m_pdfView->zoomFactor() * 100);
    m_btnZoomReset->setText(QString("%1%").arg(perc));
}

// ═════════════════════════════════════════════════════════════════════════════
// Respostas de Rede e Registro
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::iniciarProgresso(const QString &descricao, int duracaoEstimadaMs)
{
    m_operacaoAtual = descricao;
    m_tempoDecorridoMs = 0;
    m_duracaoEstimadaMs = qMax(600, duracaoEstimadaMs);
    m_progressoPercentual = 5;

    const QString texto = QString("%1: %2%").arg(m_operacaoAtual).arg(m_progressoPercentual);
    if (m_lblProgressoNumerico) {
        m_lblProgressoNumerico->setText(texto);
    }
    if (m_lblProgressoTopo) {
        m_lblProgressoTopo->setText(QString("%1%").arg(m_progressoPercentual));
        m_lblProgressoTopo->setVisible(true);
    }
    if (m_timerProgresso) {
        m_timerProgresso->start(40);
    }
}

void MainWindow::atualizarProgressoPasso()
{
    m_tempoDecorridoMs += 40;
    double t = double(m_tempoDecorridoMs) / double(m_duracaoEstimadaMs);
    if (t > 1.0) t = 1.0 + (t - 1.0) * 0.15;

    // Curva assintótica que desacelera perto de 96% aguardando a resposta real
    int pct = qBound(5, int((1.0 - std::exp(-2.5 * t)) * 105.0), 96);
    m_progressoPercentual = pct;

    const QString texto = QString("%1: %2%").arg(m_operacaoAtual).arg(m_progressoPercentual);
    if (m_lblProgressoNumerico) {
        m_lblProgressoNumerico->setText(texto);
    }
    if (m_lblProgressoTopo) {
        m_lblProgressoTopo->setText(QString("%1%").arg(m_progressoPercentual));
    }
}

void MainWindow::finalizarProgresso(bool sucesso)
{
    if (m_timerProgresso) {
        m_timerProgresso->stop();
    }
    m_progressoPercentual = sucesso ? 100 : m_progressoPercentual;

    if (m_lblProgressoNumerico) {
        if (sucesso) {
            m_lblProgressoNumerico->setText(QString("%1: 100%").arg(m_operacaoAtual));
        } else {
            m_lblProgressoNumerico->setText(QString("%1: Falha").arg(m_operacaoAtual));
        }
    }
    if (m_lblProgressoTopo) {
        m_lblProgressoTopo->setText(sucesso ? "100%" : "Erro");
    }

    QTimer::singleShot(sucesso ? 1200 : 2500, this, [this]() {
        if (m_timerProgresso && !m_timerProgresso->isActive()) {
            if (m_lblProgressoNumerico) m_lblProgressoNumerico->setText("");
            if (m_lblProgressoTopo) {
                m_lblProgressoTopo->setText("");
                m_lblProgressoTopo->setVisible(false);
            }
        }
    });
}

void MainWindow::onServidorOnline(bool online)
{
    if (online) {
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Motor de IA ativo");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Motor de IA ativo");
        emit backendPronto();
        m_net->verificarModelosStatus();
    } else {
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Iniciando motor de IA em segundo plano...");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Iniciando motor de IA em segundo plano...");
        verificarEIniciarBackend();
    }
}

void MainWindow::onModelosStatusRecebido(bool ollamaOnline, bool temPhi3, bool temLlama32, bool temLlama3)
{
    QComboBox *comboPerfil = m_tabConfiguracoes ? m_tabConfiguracoes->comboPerfil() : nullptr;
    if (!comboPerfil) return;
    auto *model = qobject_cast<QStandardItemModel*>(comboPerfil->model());
    if (!model) return;

    // Item 0: Nuvem (Gemini / OpenAI) - sempre habilitado
    if (auto *item = model->item(0)) {
        item->setEnabled(true);
        item->setText("Nuvem    — API Gemini/OpenAI  (sem Ollama, requer chave)");
    }

    // Item 1: Básico (phi3:mini)
    const bool hab1 = ollamaOnline && temPhi3;
    if (auto *item = model->item(1)) {
        item->setEnabled(hab1);
        if (!ollamaOnline) {
            item->setText("Básico   — phi3:mini (Ollama desligado)");
        } else if (!temPhi3) {
            item->setText("Básico   — phi3:mini (Não instalado no Ollama)");
        } else {
            item->setText("Básico   — phi3:mini + Whisper tiny.en  (~3 GB RAM)");
        }
    }

    // Item 2: Médio (llama3.2:3b)
    const bool hab2 = ollamaOnline && temLlama32;
    if (auto *item = model->item(2)) {
        item->setEnabled(hab2);
        if (!ollamaOnline) {
            item->setText("Médio    — llama3.2:3b (Ollama desligado)");
        } else if (!temLlama32) {
            item->setText("Médio    — llama3.2:3b (Não instalado no Ollama)");
        } else {
            item->setText("Médio    — llama3.2:3b + Whisper base.en (~6 GB RAM)");
        }
    }

    // Item 3: Avançado (llama3)
    const bool hab3 = ollamaOnline && temLlama3;
    if (auto *item = model->item(3)) {
        item->setEnabled(hab3);
        if (!ollamaOnline) {
            item->setText("Avançado — llama3 (Ollama desligado)");
        } else if (!temLlama3) {
            item->setText("Avançado — llama3 (Não instalado no Ollama)");
        } else {
            item->setText("Avançado — llama3 + Whisper base.en     (~12 GB RAM)");
        }
    }

    // Se o item atualmente selecionado estiver desabilitado, seleciona o melhor disponível
    const int idxAtual = comboPerfil->currentIndex();
    if (auto *itemAtual = model->item(idxAtual)) {
        if (!itemAtual->isEnabled()) {
            if (hab2) {
                comboPerfil->setCurrentIndex(2);
            } else if (hab1) {
                comboPerfil->setCurrentIndex(1);
            } else if (hab3) {
                comboPerfil->setCurrentIndex(3);
            } else {
                comboPerfil->setCurrentIndex(0); // Nuvem como fallback seguro
            }
        }
    }
    atualizarBadgeModeloAtivo();

    // Auto pull de modelos caso estejam faltando e o perfil exija
    if (ollamaOnline && m_iaProvedor == "ollama") {
        if (!temLlama32 && m_ollamaModelTier == "llama3.2:3b") {
            puxarModeloOllama("llama3.2:3b");
        } else if (!temPhi3 && m_ollamaModelTier == "phi3:mini") {
            puxarModeloOllama("phi3:mini");
        } else if (!temLlama3 && m_ollamaModelTier == "llama3") {
            puxarModeloOllama("llama3");
        }
    }
}

void MainWindow::puxarModeloOllama(const QString &modelo)
{
    if (m_processoOllamaPull && m_processoOllamaPull->state() == QProcess::Running) {
        return; // Já está baixando algo
    }
    
    if (!m_processoOllamaPull) {
        m_processoOllamaPull = new QProcess(this);
        connect(m_processoOllamaPull, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, modelo](int exitCode, QProcess::ExitStatus) {
            if (exitCode == 0) {
                if (m_net) m_net->verificarModelosStatus(); // Atualiza a UI para liberar o modelo
            }
        });
    }
    
#ifdef Q_OS_WIN
    QString ollamaPath = "ollama.exe";
    QString localAppPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/Programs/Ollama/ollama.exe";
    if (QFile::exists(localAppPath)) {
        ollamaPath = localAppPath;
    }
    m_processoOllamaPull->start(ollamaPath, QStringList() << "pull" << modelo);
#else
    m_processoOllamaPull->start("ollama", QStringList() << "pull" << modelo);
#endif
}

void MainWindow::onRequisicaoIniciada(const QString &endpoint)
{
    if (endpoint == "/traduzir") {
        if (m_tabTraducao) setButtonBusy(m_tabTraducao->btnTraduzir(), true);
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Traduzindo seleção técnica...");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Traduzindo...");
        const int tempoEst = (m_iaProvedor == "gemini") ? 3200 : 1800;
        iniciarProgresso("Tradução", tempoEst);
    } else if (endpoint == "/falar") {
        if (m_tabTutor) setButtonBusy(m_tabTutor->btnOuvir(), true);
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Gerando áudio da fala...");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Gerando áudio da fala...");
        iniciarProgresso("Voz", 1200);
    } else if (endpoint.startsWith("/parar_gravacao")) {
        if (m_tabTutor) setButtonBusy(m_tabTutor->btnGravar(), true);
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Transcrevendo com Whisper...");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Transcrevendo com Whisper...");
        iniciarProgresso("Transcrição", 1500);
    } else if (endpoint == "/avaliar_pronuncia") {
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Analisando pronúncia técnica...");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Analisando pronúncia...");
        iniciarProgresso("Avaliação", 1800);
    } else if (endpoint == "/chat_ia") {
        if (m_tabChatIA && m_tabChatIA->promptInput()) m_tabChatIA->promptInput()->setSendButtonBusy(true);
        if (m_lblStatusGeral) m_lblStatusGeral->setText("Consultando Assistente IA...");
        if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Consultando Assistente IA...");
        iniciarProgresso("Assistente", 2800);
    }
}

void MainWindow::onRequisicaoConcluida(const QString &endpoint)
{
    if (endpoint == "/traduzir" && m_tabTraducao) setButtonBusy(m_tabTraducao->btnTraduzir(), false);
    if (endpoint == "/falar" && m_tabTutor) setButtonBusy(m_tabTutor->btnOuvir(), false);
    if (endpoint.startsWith("/parar_gravacao") && m_tabTutor) setButtonBusy(m_tabTutor->btnGravar(), false);
    if (endpoint == "/chat_ia" && m_tabChatIA && m_tabChatIA->promptInput()) m_tabChatIA->promptInput()->setSendButtonBusy(false);

    if (m_lblStatusGeral) m_lblStatusGeral->setText("Pronto");
    if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Pronto");
    finalizarProgresso(true);
}

void MainWindow::onErroRequisicao(const QString &endpoint, const QString &mensagem)
{
    if (endpoint == "/traduzir" && m_tabTraducao) setButtonBusy(m_tabTraducao->btnTraduzir(), false);
    if (endpoint == "/falar" && m_tabTutor) setButtonBusy(m_tabTutor->btnOuvir(), false);
    if (endpoint.startsWith("/parar_gravacao") && m_tabTutor) setButtonBusy(m_tabTutor->btnGravar(), false);
    if (endpoint == "/chat_ia") {
        if (m_tabChatIA && m_tabChatIA->promptInput()) m_tabChatIA->promptInput()->setSendButtonBusy(false);
        if (m_tabChatIA) {
            m_tabChatIA->adicionarMensagemErro(mensagem);
        }
    }
    if (m_lblStatusGeral) m_lblStatusGeral->setText("Falha na operação");
    if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Falha na operação");
    appendLog("Erro em " + endpoint + ": " + mensagem, "error");
    finalizarProgresso(false);
}

void MainWindow::onLimparOcrResultado(const QString &texto)
{
    m_textoOriginalEn = texto;
    if (m_tabTutor) {
        if (m_tabTutor->promptTextoTutor()) m_tabTutor->promptTextoTutor()->setText(texto);
        if (m_tabTutor->btnOuvir()) m_tabTutor->btnOuvir()->setEnabled(true);
        if (m_tabTutor->btnGravar()) m_tabTutor->btnGravar()->setEnabled(true);
    }
    appendLog("Texto extraído com sucesso.");
}

void MainWindow::onGravacaoIniciada()
{
    if (m_quickLookupPopover && m_quickLookupPopover->isVisible()) {
        m_quickLookupPopover->setGravandoTutor(true);
    }
    appendLog("Gravação ativa no microfone.");
}


void MainWindow::appendLog(const QString &mensagem, const QString &tipo)
{
    const QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString cor = "#9ca3af";
    if (tipo == "error")   cor = "#f87171";
    if (tipo == "warning") cor = "#fbbf24";
    if (tipo == "success") cor = "#34d399";

    // Mostra na UI
    if (m_tabConfiguracoes) {
        m_tabConfiguracoes->appendLogEntry(timestamp, cor, mensagem);
    }
    
    // Grava no arquivo vellum.log
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logPath);
    QFile logFile(logPath + "/vellum.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << "[" << timestamp << "] [" << tipo.toUpper() << "] " << mensagem << "\n";
        
        // Rotaciona o log se for maior que 5MB (limita tamanho do arquivo)
        if (logFile.size() > 5 * 1024 * 1024) {
            logFile.close();
            QFile::remove(logPath + "/vellum_old.log");
            QFile::rename(logPath + "/vellum.log", logPath + "/vellum_old.log");
        }
    }
}

void MainWindow::setButtonBusy(QPushButton *btn, bool busy)
{
    if (!btn) return;
    btn->setEnabled(!busy);
}

// ═════════════════════════════════════════════════════════════════════════════
// Assistente Técnico / Chat IA Multimodal (Circuitos, Dúvidas e API)
// ═════════════════════════════════════════════════════════════════════════════

void MainWindow::onCapturarCircuitoChat()
{
    m_tipoCaptura = ModoCapturaRubberBand::CircuitoChat;
    m_modoCaptura = true;
    m_pdfView->viewport()->setCursor(Qt::CrossCursor);
    m_pdfView2->viewport()->setCursor(Qt::CrossCursor);
    appendLog("Modo circuito ativado: selecione o circuito ou esquema no leitor PDF com o mouse.", "info");
}

void MainWindow::onAnexarImagemChat()
{
    const QString arq = QFileDialog::getOpenFileName(
        this, "Selecionar Imagem do Circuito", QString(), "Imagens (*.png *.jpg *.jpeg *.webp *.bmp)");
    if (arq.isEmpty()) return;

    QFile file(arq);
    if (file.open(QIODevice::ReadOnly)) {
        const QByteArray bytes = file.readAll();
        m_chatImagemBase64 = QString::fromLatin1(bytes.toBase64());
        QPixmap pix;
        pix.loadFromData(bytes);
        if (m_tabChatIA && m_tabChatIA->promptInput()) {
            m_tabChatIA->promptInput()->showImagePreview(m_chatImagemBase64, QFileInfo(arq).fileName());
        }
        appendLog("Imagem anexada: " + QFileInfo(arq).fileName(), "success");
    }
}

void MainWindow::onRemoverImagemChat()
{
    m_chatImagemBase64.clear();
    if (m_tabChatIA && m_tabChatIA->promptInput()) {
        m_tabChatIA->promptInput()->hideImagePreview();
    }
}

void MainWindow::onColarTrechoChat()
{
    QString trecho = m_textoOriginalEn;
    if (trecho.isEmpty()) {
        trecho = m_textoTraduzidoPt;
    }
    if (trecho.isEmpty()) {
        appendLog("Nenhum trecho de texto capturado ainda.", "warning");
        return;
    }

    if (m_tabChatIA && m_tabChatIA->promptInput()) {
        QString atual = m_tabChatIA->promptInput()->text();
        if (!atual.isEmpty()) atual += "\n\n";
        atual += QString("Explicar este trecho: \"%1\"").arg(trecho);
        m_tabChatIA->promptInput()->setText(atual);
        m_tabChatIA->promptInput()->setFocus();
    }
}

void MainWindow::onLimparChat()
{
    m_chatHistoricoJson = QJsonArray();
    if (m_tabChatIA) {
        m_tabChatIA->resetarHistoricoVisual();
    }
    appendLog("Historico do Assistente IA limpo.", "info");
}

void MainWindow::onEnviarChat()
{
    PromptInputWidget *promptInput = m_tabChatIA ? m_tabChatIA->promptInput() : nullptr;
    const QString texto = promptInput ? promptInput->text().trimmed() : QString();
    if (texto.isEmpty() && m_chatImagemBase64.isEmpty()) {
        return;
    }

    if (m_tabChatIA) {
        m_tabChatIA->adicionarMensagemUsuario(texto, !m_chatImagemBase64.isEmpty());
    }

    // Salva no histórico JSON
    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = texto;
    m_chatHistoricoJson.append(userMsg);

    const QString imgB64 = m_chatImagemBase64;
    onRemoverImagemChat();
    if (promptInput) promptInput->clearInput();

    if (promptInput) promptInput->setSendButtonBusy(true);
    if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Consultando Assistente IA...");

    m_timerRequisicaoIA.restart();
    if (m_badgeMetricasIA) {
        m_badgeMetricasIA->setText(QString("IA (%1)...").arg(m_iaModelo));
    }

    m_net->enviarMensagemChat(texto, imgB64, m_chatHistoricoJson, m_iaProvedor, m_iaApiKey, m_iaModelo);
}

void MainWindow::onChatRespostaResultado(const QString &resposta, const QString &provedor, const QString &modelo)
{
    if (m_tabChatIA && m_tabChatIA->promptInput()) m_tabChatIA->promptInput()->setSendButtonBusy(false);
    if (m_tabConfiguracoes) m_tabConfiguracoes->setStatusText("Pronto");

    const qint64 elapsedMs = m_timerRequisicaoIA.isValid() ? m_timerRequisicaoIA.elapsed() : 0;
    if (m_badgeMetricasIA && elapsedMs > 0) {
        m_badgeMetricasIA->setText(QString("%1 • %2ms").arg(modelo).arg(elapsedMs));
    }

    // Salva no histórico JSON
    QJsonObject botMsg;
    botMsg["role"] = "assistant";
    botMsg["content"] = resposta;
    m_chatHistoricoJson.append(botMsg);

    if (m_tabChatIA) {
        m_tabChatIA->adicionarMensagemAssistente(resposta, provedor, modelo);
    }
    if (m_quickLookupPopover && m_quickLookupPopover->isVisible()) {
        m_quickLookupPopover->atualizarRespostaChat(resposta, modelo);
    }
    appendLog(QString("Resposta do Assistente IA recebida (%1).").arg(provedor), "success");
}

void MainWindow::onConfigurarIA()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Configurar API");
    dlg.setMinimumWidth(420);

    auto *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    // Modo: Online (Gemini) ou Offline (Ollama)
    auto *form = new QFormLayout();
    form->setSpacing(10);

    auto *comboModo = new QComboBox(&dlg);
    comboModo->addItem("Google Gemini (online, requer chave)", "gemini");
    comboModo->addItem("Ollama (offline, local)", "ollama");
    comboModo->setCurrentIndex(m_iaProvedor == "ollama" ? 1 : 0);
    form->addRow("Modo:", comboModo);

    // Chave Gemini
    auto *txtApiKey = new QLineEdit(&dlg);
    txtApiKey->setEchoMode(QLineEdit::Password);
    txtApiKey->setText(m_iaApiKey);
    txtApiKey->setPlaceholderText("Chave de API do Google Gemini");

    auto *chkMostrarKey = new QCheckBox("Mostrar chave", &dlg);
    connect(chkMostrarKey, &QCheckBox::toggled, &dlg, [txtApiKey](bool checked) {
        txtApiKey->setEchoMode(checked ? QLineEdit::Normal : QLineEdit::Password);
    });

    auto *layoutKey = new QVBoxLayout();
    layoutKey->addWidget(txtApiKey);
    layoutKey->addWidget(chkMostrarKey);

    auto *lblLinkKey = new QLabel(
        "<a href='https://aistudio.google.com/app/apikey' style='color:#2F80ED; text-decoration:none;'>"
        "Gerar chave gratuita em aistudio.google.com</a>", &dlg);
    lblLinkKey->setOpenExternalLinks(true);
    layoutKey->addWidget(lblLinkKey);

    form->addRow("Chave Gemini:", layoutKey);

    // Modelo Gemini
    auto *comboModelo = new QComboBox(&dlg);
    comboModelo->addItem("gemini-3.5-flash-lite (Ultra Rápido - Recomendado)", "gemini-3.5-flash-lite");
    comboModelo->addItem("gemini-3.5-flash (Balanceado / Alta Precisão)", "gemini-3.5-flash");
    comboModelo->addItem("gemini-3.8-flash (Experimental / Alta Demanda)", "gemini-3.8-flash");
    const int idxMod = comboModelo->findData(m_iaModelo);
    if (idxMod >= 0) comboModelo->setCurrentIndex(idxMod);
    form->addRow("Modelo Gemini:", comboModelo);

    // Modelo Ollama (offline)
    auto *txtOllamaModelo = new QLineEdit(&dlg);
    txtOllamaModelo->setText(m_iaProvedor == "ollama" ? m_iaModelo : "llama3");
    txtOllamaModelo->setPlaceholderText("ex: llama3, mistral");
    form->addRow("Modelo Ollama:", txtOllamaModelo);

    layout->addLayout(form);

    auto *btnBox = new QHBoxLayout();
    btnBox->addStretch();
    auto *btnCancelar = new QPushButton("Cancelar", &dlg);
    btnCancelar->setObjectName("btnSecundario");
    connect(btnCancelar, &QPushButton::clicked, &dlg, &QDialog::reject);
    btnBox->addWidget(btnCancelar);

    auto *btnSalvar = new QPushButton("Salvar", &dlg);
    btnSalvar->setObjectName("btnPrimario");
    connect(btnSalvar, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnBox->addWidget(btnSalvar);

    layout->addLayout(btnBox);

    if (dlg.exec() == QDialog::Accepted) {
        m_iaProvedor = comboModo->currentData().toString();
        m_iaApiKey   = txtApiKey->text().trimmed();
        if (m_iaProvedor == "gemini") {
            m_iaModelo = comboModelo->currentData().toString();
        } else {
            m_iaModelo = txtOllamaModelo->text().trimmed();
            if (m_iaModelo.isEmpty()) m_iaModelo = "llama3";
        }
        salvarConfiguracoes();

        if (m_tabChatIA && m_tabChatIA->lblModeloAtivoChat()) {
            m_tabChatIA->lblModeloAtivoChat()->setText(m_iaModelo);
        }
        if (m_tabChatIA && m_tabChatIA->comboChatIA()) {
            const int idx = m_tabChatIA->comboChatIA()->findData(m_iaProvedor);
            if (idx >= 0) m_tabChatIA->comboChatIA()->setCurrentIndex(idx);
        }
        atualizarBadgeModeloAtivo();
        appendLog("Configurações de IA salvas.", "success");
    }
}

void MainWindow::verificarEIniciarBackend()
{
    if (m_backendProcess && (m_backendProcess->state() == QProcess::Running || m_backendProcess->state() == QProcess::Starting)) {
        return;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidatosDirs;
    candidatosDirs << appDir;
    candidatosDirs << appDir + "/backend";
    candidatosDirs << appDir + "/backend_python";
    candidatosDirs << appDir + "/bin";
    candidatosDirs << QDir(appDir).filePath("..");
    candidatosDirs << QDir(appDir).filePath("../backend");
    candidatosDirs << QDir(appDir).filePath("../backend_python");
    candidatosDirs << QDir(appDir).filePath("../bin");
    candidatosDirs << QDir(appDir).filePath("../..");
    candidatosDirs << QDir(appDir).filePath("../../backend_python");
    candidatosDirs << QDir::currentPath();
    candidatosDirs << QDir::currentPath() + "/backend_python";
    candidatosDirs << QDir::homePath() + "/.local/share/vellum";
    candidatosDirs << QDir::homePath() + "/.local/share/vellum/backend_python";

    QString execPath;
    QStringList args;
    QString workingDir;

#ifdef Q_OS_WIN
    const QStringList binCandidates = {"vellum_backend.exe", "leitor_backend.exe"};
#else
    const QStringList binCandidates = {"vellum_backend", "leitor_backend"};
#endif

    for (const QString &dirPath : candidatosDirs) {
        QDir d(dirPath);
        const QString base = d.canonicalPath();
        if (base.isEmpty()) continue;

        // 1. Procura executável congelado (distribuição/instalador final)
        bool achouBin = false;
        for (const QString &binName : binCandidates) {
            if (QFile::exists(base + "/" + binName)) {
                execPath = base + "/" + binName;
                workingDir = base;
                achouBin = true;
                break;
            }
            if (QFile::exists(base + "/bin/" + binName)) {
                execPath = base + "/bin/" + binName;
                workingDir = base;
                achouBin = true;
                break;
            }
            if (QFile::exists(base + "/backend/" + binName)) {
                execPath = base + "/backend/" + binName;
                workingDir = base + "/backend";
                achouBin = true;
                break;
            }
        }
        if (achouBin) break;

        // 2. Procura ambiente com main.py (desenvolvimento / venv)
        const QString scriptMain = base + "/main.py";
        if (QFile::exists(scriptMain)) {
            workingDir = base;
            const QString venvLinux = base + "/venv/bin/python3";
            const QString venvParentLinux = QDir(base).filePath("../venv/bin/python3");
            const QString venvShare = QDir::homePath() + "/.local/share/vellum/venv/bin/python3";
            const QString venvWinW = base + "/venv/Scripts/pythonw.exe";
            const QString venvParentWinW = QDir(base).filePath("../venv/Scripts/pythonw.exe");
            const QString venvWin = base + "/venv/Scripts/python.exe";
            const QString venvParentWin = QDir(base).filePath("../venv/Scripts/python.exe");

            if (QFile::exists(venvLinux)) {
                execPath = venvLinux;
            } else if (QFile::exists(venvParentLinux)) {
                execPath = venvParentLinux;
            } else if (QFile::exists(venvShare)) {
                execPath = venvShare;
            } else if (QFile::exists(venvWinW)) {
                execPath = venvWinW; // Sem console preto no Windows
            } else if (QFile::exists(venvParentWinW)) {
                execPath = venvParentWinW;
            } else if (QFile::exists(venvWin)) {
                execPath = venvWin;
            } else if (QFile::exists(venvParentWin)) {
                execPath = venvParentWin;
            } else {
#ifdef Q_OS_WIN
                execPath = "pythonw";
#else
                execPath = "python3";
#endif
            }
            args << scriptMain;
            break;
        }
    }

    if (execPath.isEmpty()) {
        appendLog("Não foi possível localizar o executável ou script do backend.", "warning");
        return;
    }

    if (!m_backendProcess) {
        m_backendProcess = new QProcess(this);
        
        // Capturar logs do backend e direcioná-los para o appendLog
        connect(m_backendProcess, &QProcess::readyReadStandardOutput, this, [this]() {
            QByteArray out = m_backendProcess->readAllStandardOutput();
            // Evitar linhas vazias indesejadas
            for (const QByteArray &linha : out.split('\n')) {
                if (!linha.trimmed().isEmpty()) {
                    appendLog(QString::fromUtf8(linha).trimmed(), "info");
                }
            }
        });
        connect(m_backendProcess, &QProcess::readyReadStandardError, this, [this]() {
            QByteArray err = m_backendProcess->readAllStandardError();
            for (const QByteArray &linha : err.split('\n')) {
                if (!linha.trimmed().isEmpty()) {
                    appendLog(QString::fromUtf8(linha).trimmed(), "warning");
                }
            }
        });
        
        // Alerta caso o processo do backend morra ou falhe
        connect(m_backendProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (!m_isFechando) {
                QString msg = QString("O motor de IA encerrou inesperadamente (Código: %1, Status: %2).")
                                .arg(exitCode).arg(exitStatus == QProcess::CrashExit ? "Crash" : "Normal");
                appendLog(msg, "error");
                
                // Não exibir QMessageBox bloqueante para evitar loop infinito na inicialização
                if (m_lblStatusGeral) m_lblStatusGeral->setText("Erro no Motor IA - Tentando reiniciar...");
                
                // Reinicia em 3s, mas sem bloquear a UI
                QTimer::singleShot(3000, this, &MainWindow::verificarEIniciarBackend);
            }
        });
        
        connect(m_backendProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (!m_isFechando) {
                appendLog(QString("Erro no processo do motor de IA: %1").arg(error), "error");
            }
        });
    }

    m_backendProcess->setWorkingDirectory(workingDir);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!m_whisperModelTier.isEmpty()) {
        env.insert("WHISPER_MODEL", m_whisperModelTier);
    }
    if (!m_ollamaModelTier.isEmpty()) {
        env.insert("OLLAMA_MODEL", m_ollamaModelTier);
    }
    m_backendProcess->setProcessEnvironment(env);

    appendLog(QString("Iniciando motor de IA na porta %1...").arg(m_backendPort), "info");
    args << "--port" << QString::number(m_backendPort)
         << "--ppid" << QString::number(QCoreApplication::applicationPid());
    
    // Configurar o QProcess para interceptar stdout e stderr antes de iniciar
    m_backendProcess->setProcessChannelMode(QProcess::SeparateChannels);
    
    m_backendProcess->start(execPath, args);

    // Tenta pingar a conexão gradualmente
    for (int ms : {1200, 2500, 4500, 7000}) {
        QTimer::singleShot(ms, this, [this]() {
            if (m_net) m_net->verificarConexao();
        });
    }
}

void MainWindow::pararBackend()
{
    if (m_backendProcess && m_backendProcess->state() == QProcess::Running) {
        m_backendProcess->terminate();
        if (!m_backendProcess->waitForFinished(2000)) {
            m_backendProcess->kill();
        }
        m_backendProcess->deleteLater();
        m_backendProcess = nullptr;
    }
}

void MainWindow::registrarLancadorLinux()
{
#ifdef Q_OS_LINUX
    QString appsDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (appsDir.isEmpty()) {
        appsDir = QDir::homePath() + "/.local/share/applications";
    }
    QDir().mkpath(appsDir);
    QString desktopPath = appsDir + "/vellum.desktop";

    QString appPath = QCoreApplication::applicationFilePath();
    const QString appDir = QCoreApplication::applicationDirPath();
    QString iconPath = QDir::homePath() + "/.local/share/icons/hicolor/512x512/apps/vellum.png";
    if (!QFile::exists(iconPath)) {
        const QString cand1 = QDir(appDir).filePath("../frontend_qt/resources/app_icon.png");
        const QString cand2 = QDir::currentPath() + "/frontend_qt/resources/app_icon.png";
        if (QFile::exists(cand1)) {
            iconPath = QFileInfo(cand1).canonicalFilePath();
        } else if (QFile::exists(cand2)) {
            iconPath = QFileInfo(cand2).canonicalFilePath();
        } else {
            iconPath = "vellum";
        }
    }

    QFile file(desktopPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Version=1.0\n";
        out << "Type=Application\n";
        out << "Name=Vellum\n";
        out << "GenericName=Leitor Técnico & Tutor IA\n";
        out << "Comment=Leitor Técnico e Tutor de Inglês com Inteligência Artificial\n";
        out << "Exec=" << appPath << " %f\n";
        out << "Icon=" << iconPath << "\n";
        out << "Terminal=false\n";
        out << "Categories=Office;Viewer;Education;Science;\n";
        out << "MimeType=application/pdf;\n";
        out << "StartupWMClass=Vellum\n";
        out << "StartupNotify=true\n";
        file.close();

        QFile::setPermissions(desktopPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
                                          QFile::ReadGroup | QFile::ExeGroup |
                                          QFile::ReadOther | QFile::ExeOther);
        QProcess::startDetached("update-desktop-database", QStringList() << appsDir);
    }
#endif
}

