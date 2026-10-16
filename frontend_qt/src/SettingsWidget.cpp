#include "SettingsWidget.h"
#include <QScrollBar>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SettingsWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(10);

    auto *lblPerfil = new QLabel("Perfil de Hardware:", this);
    lblPerfil->setObjectName("lblSecao");
    layout->addWidget(lblPerfil);

    m_comboPerfil = new QComboBox(this);
    m_comboPerfil->setObjectName("comboPerfil");
    m_comboPerfil->addItem("Nuvem    — API Gemini/OpenAI  (sem Ollama, requer chave)",  "nuvem");
    m_comboPerfil->addItem("Basico   — phi3:mini + Whisper tiny.en  (~3 GB RAM)",        "basico");
    m_comboPerfil->addItem("Medio    — llama3.2:3b + Whisper base.en (~6 GB RAM)",       "medio");
    m_comboPerfil->addItem("Avancado — llama3 + Whisper base.en     (~12 GB RAM)",       "avancado");
    layout->addWidget(m_comboPerfil);

    auto *lblTierInfo = new QLabel(
        "<span style='color:#64748b; font-size:10px;'>"
        "A troca de perfil é aplicada em tempo real (sem reiniciar)."
        "</span>", this);
    layout->addWidget(lblTierInfo);

    auto *lblLayout = new QLabel("Gerenciamento de Layout:", this);
    lblLayout->setObjectName("lblSecao");
    layout->addWidget(lblLayout);

    auto *layoutBtnsLayout = new QHBoxLayout();
    m_btnSalvarLayout = new QPushButton("Salvar Layout", this);
    m_btnSalvarLayout->setObjectName("btnSecundario");
    layoutBtnsLayout->addWidget(m_btnSalvarLayout);

    m_btnRestaurarLayout = new QPushButton("Restaurar Padrao", this);
    m_btnRestaurarLayout->setObjectName("btnSecundario");
    layoutBtnsLayout->addWidget(m_btnRestaurarLayout);
    layout->addLayout(layoutBtnsLayout);

    auto *layoutProps = new QHBoxLayout();
    m_btnProporcao100 = new QPushButton("100% Leitor", this);
    m_btnProporcao100->setObjectName("btnSecundario");
    layoutProps->addWidget(m_btnProporcao100);

    m_btnProporcao70 = new QPushButton("70% / 30%", this);
    m_btnProporcao70->setObjectName("btnSecundario");
    layoutProps->addWidget(m_btnProporcao70);

    m_btnProporcao50 = new QPushButton("50% / 50%", this);
    m_btnProporcao50->setObjectName("btnSecundario");
    layoutProps->addWidget(m_btnProporcao50);
    layout->addLayout(layoutProps);

    auto *lblLog = new QLabel("Registro de Atividades:", this);
    lblLog->setObjectName("lblSecao");
    layout->addWidget(lblLog);

    m_logBox = new QTextEdit(this);
    m_logBox->setObjectName("logBox");
    m_logBox->setReadOnly(true);
    m_logBox->setMaximumHeight(110);
    layout->addWidget(m_logBox);

    m_lblStatus = new QLabel("Pronto", this);
    m_lblStatus->setObjectName("lblStatus");
    layout->addWidget(m_lblStatus);

    connect(m_comboPerfil, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWidget::perfilHardwareChanged);
    connect(m_btnSalvarLayout,    &QPushButton::clicked, this, &SettingsWidget::salvarLayoutRequested);
    connect(m_btnRestaurarLayout, &QPushButton::clicked, this, &SettingsWidget::restaurarLayoutRequested);
    connect(m_btnProporcao100,    &QPushButton::clicked, this, [this]() { emit aplicarProporcaoRequested(100); });
    connect(m_btnProporcao70,     &QPushButton::clicked, this, [this]() { emit aplicarProporcaoRequested(70); });
    connect(m_btnProporcao50,     &QPushButton::clicked, this, [this]() { emit aplicarProporcaoRequested(50); });
}

void SettingsWidget::setStatusText(const QString &texto)
{
    if (m_lblStatus) {
        m_lblStatus->setText(texto);
    }
}

void SettingsWidget::appendLogEntry(const QString &timestamp, const QString &corHex, const QString &mensagem)
{
    if (!m_logBox) return;
    m_logBox->append(QString("<span style='color: #4b5563;'>[%1]</span> <span style='color: %2;'>%3</span>")
                     .arg(timestamp, corHex, mensagem.toHtmlEscaped()));
    if (m_logBox->verticalScrollBar()) {
        m_logBox->verticalScrollBar()->setValue(m_logBox->verticalScrollBar()->maximum());
    }
}
