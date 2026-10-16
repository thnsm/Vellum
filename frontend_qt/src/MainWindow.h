#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QRubberBand>
#include <QPdfDocument>
#include <QPdfView>
#include <QPoint>
#include <QString>
#include <QByteArray>
#include <QScrollBar>
#include <QSpinBox>
#include <QCheckBox>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QTabWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QJsonArray>
#include <QTextBrowser>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QProcess>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QElapsedTimer>

class NetworkManager;
class PromptInputWidget;
class ThemeManager;
class TitleBar;
class FloatingHUD;
class QuickLookupPopover;
class SelectionOverlayWidget;
class TranslationWidget;
class SpeechTutorWidget;
class AiAssistantWidget;
class NotesWidget;
class SettingsWidget;

enum class ModoVisualizacao {
    UmaPagina,     // 1 página por vez
    DuasPaginas,   // 2 páginas lado a lado (modo livro)
    Continuo       // Rolagem contínua vertical
};

/**
 * MainWindow
 * ──────────
 * Leitor e Tradutor Técnico de Documentos PDF com Tutor de Pronúncia Integrado.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    void carregarArquivoPdf(const QString &caminho);
    void abrirOuAtivarAba(int index);
    void onFecharAbaPainel(int index);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

signals:
    void backendPronto();

private slots:
    // ── Arquivo e Seleção ─────────────────────────────────────────────────
    void onAbrirPdf();
    void onFecharPdf();
    void onToggleModoCaptura(bool ativo);
    void onTraduzirDireto();
    void onCopiarTraducao();

    // ── Painel e Layout ───────────────────────────────────────────────────
    void onTogglePainelLateral(bool visivel);
    void onMudarAbaPainel(int index);
    void executarComandoAba(int index);
    void executarOcrPreemptivoPaginaAtual();
    void onOcrPaginaCompletaResultado(int pagina, int largura, int altura, const QJsonArray &palavras, const QString &textoCompleto);
    QString extrairTextoOcrDaRegiao(const QRect &rect, const QPolygonF &poligono, bool isLivre);
    void onSalvarLayout();
    void onRestaurarLayoutPadrao();
    void onAplicarProporcao(int leitorPercent);

    // ── Tutor de Pronúncia & Voz ──────────────────────────────────────────
    void onOuvirPronuncia();
    void onPararAudio();
    void onGravarVozTutor();
    void onFalaIniciada(const QString &voz);
    void onAvaliacaoPronunciaResultado(int nota, const QString &feedback, const QString &textoFalado, const QStringList &palavrasAusentes, const QString &htmlDestacado = QString());

    // ── Assistente IA & Chat ──────────────────────────────────────────────
    void onEnviarChat();
    void onLimparChat();
    void onCapturarCircuitoChat();
    void onAnexarImagemChat();
    void onRemoverImagemChat();
    void onColarTrechoChat();
    void onConfigurarIA();
    void onChatRespostaResultado(const QString &resposta, const QString &provedor, const QString &modelo);

    // ── Navegação e Modos de Visualização ─────────────────────────────────
    void onPaginaAnterior();
    void onPaginaProxima();
    void onPaginaSpinChanged(int pag);
    void onModo1PagClicado();
    void onModo2PagClicado();
    void onModoContClicado();

    // ── Controles de Zoom e Janela ────────────────────────────────────────
    void onZoomMenos();
    void onZoomMais();
    void onAjustarLargura();
    void onAjustarPagina();
    void onZoomReset();
    void onToggleTelaCheia();

    // ── Respostas de Rede ─────────────────────────────────────────────────
    void onTraducaoDiretaResultado(const QString &textoIngles, const QString &traducaoPortugues, const QString &modeloUsado = QString());
    void onLimparOcrResultado(const QString &texto);
    void onTranscricaoResultado(const QString &texto);
    void onServidorOnline(bool online);
    void onModelosStatusRecebido(bool ollamaOnline, bool temPhi3, bool temLlama32, bool temLlama3);
    void onRequisicaoIniciada(const QString &endpoint);
    void onRequisicaoConcluida(const QString &endpoint);
    void onErroRequisicao(const QString &endpoint, const QString &mensagem);
    void onGravacaoIniciada();

    void puxarModeloOllama(const QString &modelo);

    // ── Configurações e Desempenho ────────────────────────────────────────
    void onPerfilAlterado(int index);

private:
    QMenuBar* setupMenuBar();
    void setupUi();
    void registrarLancadorLinux();
    QWidget* criarBarraVisualizacao();
    void setupStyleSheet();
    void connectSignals();

    void carregarConfiguracoes();
    void salvarConfiguracoes();

    void irParaPagina(int pagina);
    void forcarNavegacaoPagina(int pagina);
    void setModoVisualizacao(ModoVisualizacao modo);
    void aplicarZoom(qreal fator);
    void zoomDelta(qreal delta);
    void atualizarInfoNavegacao();
    void atualizarLabelZoom();
    int  calcularScrollVerticalParaPagina(int pagina, qreal zoom) const;
    int  obterPaginaVisivelNoModoContinuo() const;

    void capturarRegiaoRubberBand();
    void dispararTraducaoTexto(const QString &textoEn, const QByteArray &imagemB64 = QByteArray());

    void appendLog(const QString &mensagem, const QString &tipo = "info");
    void setButtonBusy(QPushButton *btn, bool busy);

    // ── Estado interno ────────────────────────────────────────────────────
    enum class ModoCapturaRubberBand { Nenhum, Traducao, CircuitoChat };
    ModoCapturaRubberBand m_tipoCaptura = ModoCapturaRubberBand::Nenhum;

    bool             m_modoCaptura        = false;
    bool             m_isFechando         = false;
    bool             m_gravando           = false;
    bool             m_panAtivo           = false;
    bool             m_bloquearSyncPagina = false;
    ModoVisualizacao m_modoVis            = ModoVisualizacao::UmaPagina;
    int              m_paginaAtual        = 0;

    QString          m_textoOcrAtual;
    QString          m_textoOriginalEn;
    QString          m_textoTraduzidoPt;

    QString          m_chatImagemBase64;
    QJsonArray       m_chatHistoricoJson;
    QString          m_iaProvedor         = "gemini";
    QString          m_iaApiKey;
    QString          m_iaModelo           = "gemini-3.5-flash-lite";
    QString          m_ollamaModelTier    = "llama3";    // modelo Ollama do tier selecionado
    QString          m_whisperModelTier   = "base.en";   // modelo Whisper do tier selecionado
    bool             m_panModo            = false;       // mao de navegacao no PDF

    QPoint           m_rbOrigin;
    QPoint           m_panOrigin;

    bool             m_barraVisPreviaVisivel = true;
    int              m_paginaSalvaResize     = 0;
    int              m_scrollOffsetNaPagina  = 0;
    QTimer          *m_timerDebounceResize   = nullptr;
    QTimer          *m_timerZoomDebounce     = nullptr;

    // ── Layout e Divisor ──────────────────────────────────────────────────
    QSplitter       *m_splitter       = nullptr;
    QWidget         *m_painelEsq      = nullptr;
    QTabWidget      *m_tabWidget      = nullptr;
    QWidget         *m_barraVis       = nullptr;
    TitleBar        *m_titleBar       = nullptr;
    QMenuBar        *m_customMenuBar  = nullptr;

    TranslationWidget *m_tabTraducao      = nullptr;
    SpeechTutorWidget *m_tabTutor         = nullptr;
    AiAssistantWidget *m_tabChatIA        = nullptr;
    NotesWidget       *m_tabAnotacoes     = nullptr;
    SettingsWidget    *m_tabConfiguracoes = nullptr;
    ThemeManager      *m_themeManager     = nullptr;

    FloatingHUD         *m_floatingHud        = nullptr;
    QuickLookupPopover  *m_quickLookupPopover = nullptr;
    QLabel              *m_badgeMetricasIA    = nullptr;
    QElapsedTimer        m_timerRequisicaoIA;

    // ── Estruturas e Cache para OCR Preemptivo em Segundo Plano ───────────
    struct OcrPalavra {
        QString texto;
        QRect box;
    };
    struct CacheOcrPagina {
        int pagina = -1;
        int largura = 0;
        int altura = 0;
        QList<OcrPalavra> palavras;
        QString textoCompleto;
        QDateTime criadoEm;
    };

    QMap<int, CacheOcrPagina> m_cacheOcrPaginas;
    QTimer                 *m_timerOcrPreemptivo = nullptr;
    QTimer                 *m_timerLimpezaCacheOcr = nullptr;
    int                     m_paginaAnteriorOcr = -1;

    // ── Seleção Livre e Retangular ─────────────────────────────────────────
    SelectionOverlayWidget *m_overlaySelecao = nullptr;

    // ── Controle de Atalhos em Tela Cheia (1º toque: seleção/popover; 2º toque: painel)
    int                     m_ultimoAtalhoTelaCheia = -1;
    int                     m_contagemAtalhoTelaCheia = 0;
    qint64                  m_tempoUltimoAtalhoTelaCheia = 0;
    bool                    m_origemCapturaTelaCheiaFlutuante = false;

    // ── PDF ───────────────────────────────────────────────────────────────
    QPdfDocument    *m_pdfDoc         = nullptr;
    QPdfView        *m_pdfView        = nullptr;
    QPdfView        *m_pdfView2       = nullptr;
    QPdfView        *m_activePdfView  = nullptr;
    QRubberBand     *m_rubberBand     = nullptr;

    // ── Rede ──────────────────────────────────────────────────────────────
    NetworkManager  *m_net            = nullptr;

    // ── Ações do Menu ─────────────────────────────────────────────────────
    QAction         *m_actTogglePainel = nullptr;
    QAction         *m_actBarraLeitor  = nullptr;
    QAction         *m_actTelaCheia    = nullptr;

    // ── Barra Superior de Navegação ───────────────────────────────────────
    QPushButton     *m_btnPagAnterior = nullptr;
    QPushButton     *m_btnPagProxima  = nullptr;
    QSpinBox        *m_spinPagina     = nullptr;
    QLabel          *m_lblTotalPag    = nullptr;

    QPushButton     *m_btnModo1Pag    = nullptr;
    QPushButton     *m_btnModo2Pag    = nullptr;
    QPushButton     *m_btnModoCont    = nullptr;

    QPushButton     *m_btnZoomMenos   = nullptr;
    QPushButton     *m_btnZoomMais    = nullptr;
    QPushButton     *m_btnZoomReset   = nullptr;
    QPushButton     *m_btnAjustarLarg = nullptr;
    QPushButton     *m_btnAjustarPag  = nullptr;
    QPushButton     *m_btnTelaCheia   = nullptr;

    // ── Badge de Modelo e Progresso Numérico Minimalista ─────────────────
    void atualizarBadgeModeloAtivo();
    void iniciarProgresso(const QString &descricao, int duracaoEstimadaMs = 2000);
    void atualizarProgressoPasso();
    void finalizarProgresso(bool sucesso = true);

    QLabel          *m_badgeModeloAtivo     = nullptr;
    QLabel          *m_lblProgressoTopo     = nullptr;
    QLabel          *m_lblProgressoNumerico = nullptr;
    QLabel          *m_lblStatusGeral       = nullptr;
    QTimer          *m_timerProgresso       = nullptr;
    int              m_progressoPercentual  = 0;
    int              m_duracaoEstimadaMs    = 2000;
    int              m_tempoDecorridoMs     = 0;
    QString          m_operacaoAtual;

    // ── Empty State & Gerenciamento do Motor Backend ──────────────────────
    QWidget         *m_emptyStateWidget     = nullptr;
    QProcess        *m_backendProcess       = nullptr;
    quint16          m_backendPort          = 8000;
    void atualizarVisibilidadeEmptyState();
    void verificarEIniciarBackend();
    void pararBackend();
    
    QProcess        *m_processoOllamaPull   = nullptr;
};

