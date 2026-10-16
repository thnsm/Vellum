#include "TranslationWidget.h"
#include "PromptInputWidget.h"
#include "SelectionOverlayWidget.h"

TranslationWidget::TranslationWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void TranslationWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(8);

    auto *layoutTop = new QHBoxLayout();
    layoutTop->setSpacing(6);

    m_btnModoCaptura = new QPushButton("Modo Seleção", this);
    m_btnModoCaptura->setObjectName("btnCaptura");
    m_btnModoCaptura->setCheckable(true);
    layoutTop->addWidget(m_btnModoCaptura);

    m_comboFormatoSelecao = new QComboBox(this);
    m_comboFormatoSelecao->setObjectName("comboFormatoSelecao");
    m_comboFormatoSelecao->addItem("Retângulo", static_cast<int>(SelectionOverlayWidget::Formato::Retangular));
    m_comboFormatoSelecao->addItem("Formato Livre", static_cast<int>(SelectionOverlayWidget::Formato::Livre));
    m_comboFormatoSelecao->setToolTip("Formato de captura: Retângulo padrão ou Desenho Livre com o mouse");
    layoutTop->addWidget(m_comboFormatoSelecao);

    m_btnTraduzir = new QPushButton("Traduzir Seleção", this);
    m_btnTraduzir->setObjectName("btnTraduzir");
    m_btnTraduzir->setEnabled(false);
    layoutTop->addWidget(m_btnTraduzir);

    layout->addLayout(layoutTop);

    m_chkTraducaoAuto = new QCheckBox("Traduzir automaticamente ao selecionar na tela", this);
    m_chkTraducaoAuto->setObjectName("chkAuto");
    m_chkTraducaoAuto->setChecked(true);
    layout->addWidget(m_chkTraducaoAuto);

    auto *lblOrig = new QLabel("Texto Original em Inglês (Editável)", this);
    lblOrig->setObjectName("lblSecao");
    layout->addWidget(lblOrig);

    m_txtTraducaoOriginalEn = new QTextEdit(this);
    m_txtTraducaoOriginalEn->setObjectName("txtTraducaoOriginalEn");
    m_txtTraducaoOriginalEn->setPlaceholderText("Selecione um parágrafo no documento ou digite/edite o texto em inglês aqui...");
    m_txtTraducaoOriginalEn->setFixedHeight(80);
    layout->addWidget(m_txtTraducaoOriginalEn);

    auto *layoutAcoesTrad = new QHBoxLayout();
    m_btnRetraduzirTexto = new QPushButton("Traduzir Texto Modificado", this);
    m_btnRetraduzirTexto->setObjectName("btnTraduzir");
    m_btnRetraduzirTexto->setCursor(Qt::PointingHandCursor);
    layoutAcoesTrad->addWidget(m_btnRetraduzirTexto);
    layoutAcoesTrad->addStretch();
    layout->addLayout(layoutAcoesTrad);

    m_timerDebounceTraducao = new QTimer(this);
    m_timerDebounceTraducao->setSingleShot(true);
    m_timerDebounceTraducao->setInterval(700);

    connect(m_txtTraducaoOriginalEn, &QTextEdit::textChanged, this, [this]() {
        if (m_timerDebounceTraducao) {
            m_timerDebounceTraducao->start(700);
        }
    });

    auto dispararRetraducao = [this]() {
        if (!m_txtTraducaoOriginalEn) return;
        const QString txt = m_txtTraducaoOriginalEn->toPlainText().trimmed();
        if (!txt.isEmpty() && txt != m_ultimoTextoSolicitado) {
            m_ultimoTextoSolicitado = txt;
            emit retraduzirTextoRequested(txt);
        }
    };

    connect(m_timerDebounceTraducao, &QTimer::timeout, this, dispararRetraducao);
    connect(m_btnRetraduzirTexto, &QPushButton::clicked, this, [this]() {
        if (!m_txtTraducaoOriginalEn) return;
        const QString txt = m_txtTraducaoOriginalEn->toPlainText().trimmed();
        if (!txt.isEmpty()) {
            m_ultimoTextoSolicitado = txt;
            emit retraduzirTextoRequested(txt);
        }
    });

    auto *lblTrad = new QLabel("Tradução Técnica (PT-BR)", this);
    lblTrad->setObjectName("lblSecao");
    layout->addWidget(lblTrad);

    m_promptTraducao = new PromptInputWidget(this);
    m_promptTraducao->setToolbarVisible(false);
    m_promptTraducao->setReadOnly(true);
    m_promptTraducao->setObjectName("promptTraducao");
    m_promptTraducao->setPlaceholderText("A tradução técnica em português aparecerá aqui...");
    layout->addWidget(m_promptTraducao, 1);

    auto *layoutBottom = new QHBoxLayout();
    m_btnCopiarTraducao = new QPushButton("Copiar Tradução", this);
    m_btnCopiarTraducao->setObjectName("btnSecundario");
    layoutBottom->addWidget(m_btnCopiarTraducao);
    layoutBottom->addStretch();

    layout->addLayout(layoutBottom);

    connect(m_btnModoCaptura, &QPushButton::toggled, this, &TranslationWidget::toggleModoCapturaRequested);
    connect(m_comboFormatoSelecao, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TranslationWidget::formatoSelecaoChanged);
    connect(m_btnTraduzir, &QPushButton::clicked, this, &TranslationWidget::traduzirSelecaoRequested);
    connect(m_btnCopiarTraducao, &QPushButton::clicked, this, &TranslationWidget::copiarTraducaoRequested);
}

void TranslationWidget::setOriginalTextSilent(const QString &textoIngles)
{
    if (!m_txtTraducaoOriginalEn) return;
    if (m_timerDebounceTraducao) {
        m_timerDebounceTraducao->stop();
    }
    m_ultimoTextoSolicitado = textoIngles.trimmed();
    m_txtTraducaoOriginalEn->blockSignals(true);
    m_txtTraducaoOriginalEn->setPlainText(textoIngles);
    m_txtTraducaoOriginalEn->blockSignals(false);
}

void TranslationWidget::exibirResultado(const QString &textoIngles, const QString &traducaoPortugues, const QString &modeloUsado)
{
    if (m_txtTraducaoOriginalEn && !m_txtTraducaoOriginalEn->hasFocus()) {
        setOriginalTextSilent(textoIngles);
    }

    const QString badgeTag = !modeloUsado.isEmpty()
        ? QString(" <span style='color: #10b981; font-size: 10px; font-weight: 600; background: #064e3b; padding: 2px 6px; border-radius: 4px;'>[%1]</span>")
            .arg(modeloUsado.toHtmlEscaped())
        : QString();

    if (m_promptTraducao) {
        m_promptTraducao->setHtml(
            "<div>"
            "<span style='color: #60a5fa; font-size: 11px; font-weight: 600; text-transform: uppercase;'>Tradução Técnica (PT-BR)</span>" + badgeTag + "<br><br>"
            "<span style='color: #f3f4f6; font-size: 13px; font-weight: 500; line-height: 1.5;'>" + traducaoPortugues.toHtmlEscaped() + "</span>"
            "</div>"
        );
    }
}
