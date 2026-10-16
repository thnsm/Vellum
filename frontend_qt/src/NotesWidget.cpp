#include "NotesWidget.h"
#include <QFrame>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QMessageBox>
#include <QGuiApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QRegularExpression>
#include <algorithm>

NotesWidget::NotesWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void NotesWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(6);

    auto *layoutAcoes = new QHBoxLayout();
    layoutAcoes->setSpacing(6);

    m_btnNovoNota = new QPushButton("Novo", this);
    m_btnNovoNota->setObjectName("btnSecundario");
    m_btnNovoNota->setToolTip("Iniciar um novo bloco de anotações em branco");
    layoutAcoes->addWidget(m_btnNovoNota);

    m_btnAbrirNota = new QPushButton("Abrir...", this);
    m_btnAbrirNota->setObjectName("btnSecundario");
    m_btnAbrirNota->setToolTip("Abrir anotações salvas anteriormente (.html ou .txt)");
    layoutAcoes->addWidget(m_btnAbrirNota);

    m_btnSalvarNota = new QPushButton("Salvar...", this);
    m_btnSalvarNota->setObjectName("btnPrimario");
    m_btnSalvarNota->setToolTip("Salvar anotações atuais em disco");
    layoutAcoes->addWidget(m_btnSalvarNota);

    m_btnCopiarNota = new QPushButton("Copiar Tudo", this);
    m_btnCopiarNota->setObjectName("btnSecundario");
    m_btnCopiarNota->setToolTip("Copiar todas as anotações para a área de transferência");
    layoutAcoes->addWidget(m_btnCopiarNota);

    layoutAcoes->addStretch();

    m_lblArquivoNota = new QLabel("Novo Documento", this);
    m_lblArquivoNota->setObjectName("lblArquivoNota");
    layoutAcoes->addWidget(m_lblArquivoNota);

    layout->addLayout(layoutAcoes);

    auto *layoutFormatacao = new QHBoxLayout();
    layoutFormatacao->setSpacing(5);

    m_comboFonteNota = new QComboBox(this);
    m_comboFonteNota->setObjectName("comboFonteNota");
    m_comboFonteNota->addItem("Segoe UI", "Segoe UI");
    m_comboFonteNota->addItem("Arial", "Arial");
    m_comboFonteNota->addItem("Roboto", "Roboto");
    m_comboFonteNota->addItem("Times New Roman", "Times New Roman");
    m_comboFonteNota->addItem("Consolas", "Consolas");
    m_comboFonteNota->addItem("JetBrains Mono", "JetBrains Mono");
    layoutFormatacao->addWidget(m_comboFonteNota);

    m_spinTamanhoNota = new QSpinBox(this);
    m_spinTamanhoNota->setObjectName("spinTamanhoNota");
    m_spinTamanhoNota->setRange(9, 36);
    m_spinTamanhoNota->setValue(13);
    m_spinTamanhoNota->setSuffix(" pt");
    layoutFormatacao->addWidget(m_spinTamanhoNota);

    m_btnFonteMenosNota = new QPushButton("-", this);
    m_btnFonteMenosNota->setObjectName("btnFormatoNota");
    m_btnFonteMenosNota->setFixedSize(28, 26);
    m_btnFonteMenosNota->setToolTip("Diminuir tamanho da fonte (-)");
    layoutFormatacao->addWidget(m_btnFonteMenosNota);

    m_btnFonteMaisNota = new QPushButton("+", this);
    m_btnFonteMaisNota->setObjectName("btnFormatoNota");
    m_btnFonteMaisNota->setFixedSize(28, 26);
    m_btnFonteMaisNota->setToolTip("Aumentar tamanho da fonte (+)");
    layoutFormatacao->addWidget(m_btnFonteMaisNota);

    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    layoutFormatacao->addWidget(sep);

    m_btnBoldNota = new QPushButton("N", this);
    m_btnBoldNota->setObjectName("btnBoldNota");
    m_btnBoldNota->setCheckable(true);
    m_btnBoldNota->setFixedSize(28, 26);
    m_btnBoldNota->setToolTip("Negrito (N)");
    layoutFormatacao->addWidget(m_btnBoldNota);

    m_btnItalicNota = new QPushButton("I", this);
    m_btnItalicNota->setObjectName("btnItalicNota");
    m_btnItalicNota->setCheckable(true);
    m_btnItalicNota->setFixedSize(28, 26);
    m_btnItalicNota->setToolTip("Itálico (I)");
    layoutFormatacao->addWidget(m_btnItalicNota);

    m_btnUnderlineNota = new QPushButton("S", this);
    m_btnUnderlineNota->setObjectName("btnUnderlineNota");
    m_btnUnderlineNota->setCheckable(true);
    m_btnUnderlineNota->setFixedSize(28, 26);
    m_btnUnderlineNota->setToolTip("Sublinhado (S)");
    layoutFormatacao->addWidget(m_btnUnderlineNota);

    m_btnCorNota = new QPushButton("Cor...", this);
    m_btnCorNota->setObjectName("btnCorNota");
    m_btnCorNota->setFixedHeight(26);
    m_btnCorNota->setToolTip("Alterar cor do texto");
    layoutFormatacao->addWidget(m_btnCorNota);

    layoutFormatacao->addStretch();
    layout->addLayout(layoutFormatacao);

    m_txtAnotacoes = new QTextEdit(this);
    m_txtAnotacoes->setObjectName("txtAnotacoes");
    m_txtAnotacoes->setPlaceholderText("Escreva aqui suas anotações de estudo, resumos e fórmulas do livro...\n\nVocê pode salvar em disco a qualquer momento para retomar seus estudos mais tarde.");
    layout->addWidget(m_txtAnotacoes, 1);

    auto *layoutStatus = new QHBoxLayout();
    m_lblStatusNota = new QLabel("0 palavras | 0 caracteres", this);
    m_lblStatusNota->setObjectName("lblStatusNota");
    layoutStatus->addWidget(m_lblStatusNota);
    layoutStatus->addStretch();

    layout->addLayout(layoutStatus);

    connect(m_btnNovoNota,       &QPushButton::clicked, this, &NotesWidget::onLimparAnotacoes);
    connect(m_btnAbrirNota,      &QPushButton::clicked, this, &NotesWidget::onAbrirAnotacoes);
    connect(m_btnSalvarNota,     &QPushButton::clicked, this, &NotesWidget::onSalvarAnotacoes);
    connect(m_btnCopiarNota,     &QPushButton::clicked, this, &NotesWidget::onCopiarAnotacoes);
    connect(m_comboFonteNota,    &QComboBox::currentTextChanged, this, &NotesWidget::onMudarFonteAnotacoes);
    connect(m_spinTamanhoNota,   QOverload<int>::of(&QSpinBox::valueChanged), this, &NotesWidget::onMudarTamanhoAnotacoes);
    connect(m_btnFonteMaisNota,  &QPushButton::clicked, this, &NotesWidget::onAumentarFonteAnotacoes);
    connect(m_btnFonteMenosNota, &QPushButton::clicked, this, &NotesWidget::onDiminuirFonteAnotacoes);
    connect(m_btnBoldNota,       &QPushButton::clicked, this, &NotesWidget::onToggleBoldAnotacoes);
    connect(m_btnItalicNota,     &QPushButton::clicked, this, &NotesWidget::onToggleItalicAnotacoes);
    connect(m_btnUnderlineNota,  &QPushButton::clicked, this, &NotesWidget::onToggleUnderlineAnotacoes);
    connect(m_btnCorNota,        &QPushButton::clicked, this, &NotesWidget::onEscolherCorAnotacoes);
    connect(m_txtAnotacoes,      &QTextEdit::textChanged, this, &NotesWidget::onAtualizarContadorNotas);
}

void NotesWidget::setText(const QString &texto)
{
    if (m_txtAnotacoes) {
        m_txtAnotacoes->setText(texto);
    }
}

void NotesWidget::appendQuickNote(const QString &texto)
{
    if (m_txtAnotacoes && !texto.isEmpty()) {
        m_txtAnotacoes->append("\n--- Anotação Rápida ---\n" + texto);
    }
}

void NotesWidget::onSalvarAnotacoes()
{
    if (!m_txtAnotacoes) return;

    QString caminho = m_caminhoArquivoNota;
    if (caminho.isEmpty()) {
        caminho = QFileDialog::getSaveFileName(
            this,
            "Salvar Anotações",
            QDir::homePath() + "/MinhasAnotacoes.html",
            "Documento HTML com formatação (*.html);;Texto Puro (*.txt);;Todos os Arquivos (*.*)"
        );
    }
    if (caminho.isEmpty()) return;

    QFile file(caminho);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erro ao Salvar", "Não foi possível gravar no arquivo:\n" + file.errorString());
        return;
    }

    QTextStream out(&file);
    if (caminho.endsWith(".txt", Qt::CaseInsensitive)) {
        out << m_txtAnotacoes->toPlainText();
    } else {
        out << m_txtAnotacoes->toHtml();
    }
    file.close();

    m_caminhoArquivoNota = caminho;
    if (m_lblArquivoNota) {
        m_lblArquivoNota->setText(QFileInfo(caminho).fileName());
    }
    emit logMessage(QString("Anotações salvas com sucesso em %1").arg(QFileInfo(caminho).fileName()), "success");
}

void NotesWidget::onAbrirAnotacoes()
{
    if (!m_txtAnotacoes) return;

    const QString caminho = QFileDialog::getOpenFileName(
        this,
        "Abrir Anotações",
        QDir::homePath(),
        "Arquivos Suportados (*.html *.htm *.txt *.md);;Documentos HTML (*.html *.htm);;Texto Puro (*.txt *.md);;Todos os Arquivos (*.*)"
    );
    if (caminho.isEmpty()) return;

    QFile file(caminho);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Erro ao Abrir", "Não foi possível abrir o arquivo:\n" + file.errorString());
        return;
    }

    const QString conteudo = QString::fromUtf8(file.readAll());
    file.close();

    if (caminho.endsWith(".html", Qt::CaseInsensitive) || caminho.endsWith(".htm", Qt::CaseInsensitive)) {
        m_txtAnotacoes->setHtml(conteudo);
    } else {
        m_txtAnotacoes->setPlainText(conteudo);
    }

    m_caminhoArquivoNota = caminho;
    if (m_lblArquivoNota) {
        m_lblArquivoNota->setText(QFileInfo(caminho).fileName());
    }
    emit logMessage(QString("Anotações carregadas: %1").arg(QFileInfo(caminho).fileName()), "info");
}

void NotesWidget::onLimparAnotacoes()
{
    if (!m_txtAnotacoes) return;
    if (!m_txtAnotacoes->toPlainText().trimmed().isEmpty()) {
        const auto resp = QMessageBox::question(
            this,
            "Novo Bloco de Notas",
            "Deseja limpar as anotações atuais e começar uma folha em branco?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );
        if (resp != QMessageBox::Yes) return;
    }
    m_txtAnotacoes->clear();
    m_caminhoArquivoNota.clear();
    if (m_lblArquivoNota) {
        m_lblArquivoNota->setText("Novo Documento");
    }
}

void NotesWidget::onCopiarAnotacoes()
{
    if (!m_txtAnotacoes) return;
    QClipboard *clip = QGuiApplication::clipboard();
    if (clip) {
        clip->setText(m_txtAnotacoes->toPlainText());
        emit logMessage("Anotações copiadas para a área de transferência.", "info");
    }
}

void NotesWidget::onMudarFonteAnotacoes(const QString &familia)
{
    if (!m_txtAnotacoes) return;
    m_txtAnotacoes->setFontFamily(familia);
}

void NotesWidget::onMudarTamanhoAnotacoes(int pt)
{
    if (!m_txtAnotacoes) return;
    m_txtAnotacoes->setFontPointSize(pt);
}

void NotesWidget::onAumentarFonteAnotacoes()
{
    if (!m_spinTamanhoNota) return;
    m_spinTamanhoNota->setValue(std::min(m_spinTamanhoNota->value() + 1, 36));
}

void NotesWidget::onDiminuirFonteAnotacoes()
{
    if (!m_spinTamanhoNota) return;
    m_spinTamanhoNota->setValue(std::max(m_spinTamanhoNota->value() - 1, 9));
}

void NotesWidget::onToggleBoldAnotacoes()
{
    if (!m_txtAnotacoes || !m_btnBoldNota) return;
    m_txtAnotacoes->setFontWeight(m_btnBoldNota->isChecked() ? QFont::Bold : QFont::Normal);
}

void NotesWidget::onToggleItalicAnotacoes()
{
    if (!m_txtAnotacoes || !m_btnItalicNota) return;
    m_txtAnotacoes->setFontItalic(m_btnItalicNota->isChecked());
}

void NotesWidget::onToggleUnderlineAnotacoes()
{
    if (!m_txtAnotacoes || !m_btnUnderlineNota) return;
    m_txtAnotacoes->setFontUnderline(m_btnUnderlineNota->isChecked());
}

void NotesWidget::onEscolherCorAnotacoes()
{
    if (!m_txtAnotacoes) return;
    const QColor inicial = m_txtAnotacoes->textColor().isValid() ? m_txtAnotacoes->textColor() : QColor("#f1f5f9");
    const QColor escolhida = QColorDialog::getColor(inicial, this, "Selecionar Cor do Texto");
    if (escolhida.isValid()) {
        m_txtAnotacoes->setTextColor(escolhida);
        if (m_btnCorNota) {
            m_btnCorNota->setStyleSheet(QString(
                "QPushButton { background-color: %1; color: %2; font-weight: 600; border: 1px solid %1; border-radius: 4px; padding: 3px 10px; font-size: 11px; }"
                "QPushButton:hover { opacity: 0.9; }"
            ).arg(escolhida.name(), escolhida.lightness() > 140 ? "#000000" : "#ffffff"));
        }
    }
}

void NotesWidget::onAtualizarContadorNotas()
{
    if (!m_txtAnotacoes || !m_lblStatusNota) return;
    const QString texto = m_txtAnotacoes->toPlainText().trimmed();
    const int chars = texto.length();
    const int palavras = texto.isEmpty() ? 0 : texto.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    m_lblStatusNota->setText(QString("%1 palavras | %2 caracteres").arg(palavras).arg(chars));
}
