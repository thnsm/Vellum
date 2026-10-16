#include "SpeechTutorWidget.h"
#include "PromptInputWidget.h"
#include <QStyle>

SpeechTutorWidget::SpeechTutorWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void SpeechTutorWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(8);

    auto *layoutVozVel = new QHBoxLayout();
    layoutVozVel->setSpacing(6);

    auto *boxVoz = new QVBoxLayout();
    auto *lblVoz = new QLabel("Voz do Tutor:", this);
    lblVoz->setObjectName("lblSecao");
    m_comboVoz = new QComboBox(this);
    m_comboVoz->setObjectName("comboVoz");
    m_comboVoz->addItem("US Jenny (Feminina)", "en-US-JennyNeural");
    m_comboVoz->addItem("US Guy (Masculina)", "en-US-GuyNeural");
    m_comboVoz->addItem("UK Sonia (Feminina)", "en-GB-SoniaNeural");
    m_comboVoz->addItem("UK Ryan (Masculina)", "en-GB-RyanNeural");
    m_comboVoz->addItem("AU Natasha (Feminina)", "en-AU-NatashaNeural");
    boxVoz->addWidget(lblVoz);
    boxVoz->addWidget(m_comboVoz);

    auto *boxVel = new QVBoxLayout();
    auto *lblVel = new QLabel("Velocidade:", this);
    lblVel->setObjectName("lblSecao");
    m_comboVelocidade = new QComboBox(this);
    m_comboVelocidade->setObjectName("comboVelocidade");
    m_comboVelocidade->addItem("0.75x (Lenta)", "-25%");
    m_comboVelocidade->addItem("1.0x (Normal)", "+0%");
    m_comboVelocidade->addItem("1.25x (Rápida)", "+25%");
    m_comboVelocidade->addItem("1.5x (Avançada)", "+50%");
    m_comboVelocidade->setCurrentIndex(1);
    boxVel->addWidget(lblVel);
    boxVel->addWidget(m_comboVelocidade);

    layoutVozVel->addLayout(boxVoz, 65);
    layoutVozVel->addLayout(boxVel, 35);
    layout->addLayout(layoutVozVel);

    auto *lblNivel = new QLabel("Nível de Exigência do Tutor:", this);
    lblNivel->setObjectName("lblSecao");
    layout->addWidget(lblNivel);

    m_comboNivelTutor = new QComboBox(this);
    m_comboNivelTutor->setObjectName("comboNivelTutor");
    m_comboNivelTutor->addItem("Iniciante (Flexível & Encorajador)", "iniciante");
    m_comboNivelTutor->addItem("Intermediário (Equilibrado)", "intermediario");
    m_comboNivelTutor->addItem("Avançado (Exigente / Rigor Técnico)", "avancado");
    m_comboNivelTutor->setCurrentIndex(1);
    layout->addWidget(m_comboNivelTutor);

    auto *lblTextoPratica = new QLabel("Texto para Prática:", this);
    lblTextoPratica->setObjectName("lblSecao");
    layout->addWidget(lblTextoPratica);

    m_promptTextoTutor = new PromptInputWidget(this);
    m_promptTextoTutor->setToolbarVisible(false);
    m_promptTextoTutor->setObjectName("promptTextoTutor");
    m_promptTextoTutor->setPlaceholderText("O texto selecionado no PDF aparecerá aqui. Você também pode digitar ou colar o texto em inglês que deseja praticar.");
    layout->addWidget(m_promptTextoTutor, 1);

    auto *layoutBotoes = new QHBoxLayout();
    layoutBotoes->setSpacing(6);

    m_btnOuvir = new QPushButton("Ouvir Pronúncia", this);
    m_btnOuvir->setObjectName("btnSecundario");
    m_btnOuvir->setEnabled(false);
    layoutBotoes->addWidget(m_btnOuvir);

    m_btnPararAudio = new QPushButton("Parar Fala", this);
    m_btnPararAudio->setObjectName("btnSecundario");
    layoutBotoes->addWidget(m_btnPararAudio);

    m_btnGravar = new QPushButton("Gravar Minha Voz", this);
    m_btnGravar->setObjectName("btnGravar");
    m_btnGravar->setEnabled(false);
    layoutBotoes->addWidget(m_btnGravar);

    layout->addLayout(layoutBotoes);

    auto *lblAcuraciaTitulo = new QLabel("Acurácia da Pronúncia:", this);
    lblAcuraciaTitulo->setObjectName("lblSecao");
    layout->addWidget(lblAcuraciaTitulo);

    m_barAcuracia = new QProgressBar(this);
    m_barAcuracia->setObjectName("barAcuracia");
    m_barAcuracia->setRange(0, 100);
    m_barAcuracia->setValue(0);
    m_barAcuracia->setTextVisible(true);
    m_barAcuracia->setFormat("%p%");
    layout->addWidget(m_barAcuracia);

    auto *lblFeedback = new QLabel("Análise Fonética & Dica do Tutor:", this);
    lblFeedback->setObjectName("lblSecao");
    layout->addWidget(lblFeedback);

    m_txtFeedbackTutor = new QTextEdit(this);
    m_txtFeedbackTutor->setObjectName("txtFeedback");
    m_txtFeedbackTutor->setReadOnly(true);
    m_txtFeedbackTutor->setPlaceholderText("Grave sua leitura do texto selecionado para receber o mapa colorido palavra por palavra (Verde/Amarelo/Vermelho) e dicas fonéticas do tutor.");
    layout->addWidget(m_txtFeedbackTutor, 1);

    connect(m_btnOuvir,      &QPushButton::clicked, this, &SpeechTutorWidget::ouvirPronunciaRequested);
    connect(m_btnPararAudio, &QPushButton::clicked, this, &SpeechTutorWidget::pararAudioRequested);
    connect(m_btnGravar,     &QPushButton::clicked, this, &SpeechTutorWidget::gravarVozRequested);
}

void SpeechTutorWidget::setRecordingState(bool gravando)
{
    if (!m_btnGravar) return;
    m_btnGravar->setText(gravando ? "Parar e Avaliar" : "Gravar Minha Voz");
    m_btnGravar->setProperty("recording", gravando);
    m_btnGravar->style()->unpolish(m_btnGravar);
    m_btnGravar->style()->polish(m_btnGravar);
}

void SpeechTutorWidget::exibirAvaliacao(int nota,
                                        const QString &feedback,
                                        const QString &textoFalado,
                                        const QStringList &palavrasAusentes,
                                        const QString &htmlDestacado)
{
    if (m_barAcuracia) {
        m_barAcuracia->setValue(nota);
    }

    const QString corNota = (nota >= 80) ? "#10b981" : (nota >= 60) ? "#f59e0b" : "#ef4444";

    QString html = QString(
        "<div style='margin-bottom: 8px;'>"
        "<span style='color: %1; font-weight: 700; font-size: 13px;'>Nota de Pronúncia: %2%</span>"
        "</div>"
    ).arg(corNota).arg(nota);

    if (!htmlDestacado.isEmpty()) {
        html += QString(
            "<div style='background-color: #161b26; border: 1px solid #283142; padding: 8px 10px; border-radius: 6px; margin-bottom: 8px;'>"
            "<div style='color: #9ca3af; font-size: 10px; font-weight: 600; text-transform: uppercase; margin-bottom: 4px;'>"
            "Mapa Fonético Palavra por Palavra "
            "<span style='color:#10b981;'>● Correta</span> &nbsp;"
            "<span style='color:#f59e0b;'>● Quase lá (sotaque)</span> &nbsp;"
            "<span style='color:#ef4444;'>● Faltou/Rever</span>"
            "</div>"
            "<div style='font-size: 13px; line-height: 1.7;'>%1</div>"
            "</div>"
        ).arg(htmlDestacado);
    }

    html += QString(
        "<div style='background-color: #1e222a; border-left: 3px solid #3b82f6; padding: 6px 8px; border-radius: 4px; margin-bottom: 8px;'>"
        "<span style='color: #9ca3af; font-size: 10px; font-weight: 600; text-transform: uppercase;'>O que o Whisper ouviu literalmente:</span><br>"
        "<span style='color: #e5e7eb; font-size: 12px; font-weight: 500;'>\"%1\"</span>"
        "</div>"
        "<div style='margin-bottom: 8px; color: #d1d5db; font-size: 12px; line-height: 1.45;'>"
        "<b>Dica do Tutor (Pronúncia Figurada):</b><br>%2"
        "</div>"
    ).arg(textoFalado.toHtmlEscaped(), feedback.toHtmlEscaped().replace("\n", "<br>"));

    if (!palavrasAusentes.isEmpty()) {
        html += "<div style='background-color: #26201a; border-left: 3px solid #f59e0b; padding: 5px 8px; border-radius: 4px; color: #fbbf24; font-size: 11px; margin-top: 6px;'>"
                "<b>Foque nestas palavras na próxima leitura:</b><br>" + palavrasAusentes.join(", ").toHtmlEscaped() +
                "</div>";
    }

    if (m_txtFeedbackTutor) {
        m_txtFeedbackTutor->setHtml(html);
    }
}
