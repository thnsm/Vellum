#include "AiAssistantWidget.h"
#include "PromptInputWidget.h"
#include <QRegularExpression>
#include <QScrollBar>

AiAssistantWidget::AiAssistantWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void AiAssistantWidget::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 8, 4, 4);
    layout->setSpacing(6);

    auto *layoutHeader = new QHBoxLayout();
    layoutHeader->setSpacing(6);

    m_comboChatIA = new QComboBox(this);
    m_comboChatIA->setObjectName("comboChatIA");
    m_comboChatIA->setToolTip("Selecionar IA para responder");
    m_comboChatIA->addItem("Gemini",  "gemini");
    m_comboChatIA->addItem("Ollama",  "ollama");
    layoutHeader->addWidget(m_comboChatIA);

    m_lblModeloAtivoChat = new QLabel(QStringLiteral("—"), this);
    m_lblModeloAtivoChat->setObjectName("lblModeloAtivoChat");
    layoutHeader->addWidget(m_lblModeloAtivoChat, 1);

    m_btnConfigurarIA = new QPushButton("Configurar API", this);
    m_btnConfigurarIA->setObjectName("btnSecundario");
    m_btnConfigurarIA->setToolTip("Configurar chave da API Gemini ou ativar modo offline");
    layoutHeader->addWidget(m_btnConfigurarIA);

    m_btnLimparChat = new QPushButton("Limpar", this);
    m_btnLimparChat->setObjectName("btnSecundario");
    m_btnLimparChat->setToolTip("Limpar historico de conversa");
    layoutHeader->addWidget(m_btnLimparChat);

    layout->addLayout(layoutHeader);

    m_chatHistorico = new QTextBrowser(this);
    m_chatHistorico->setObjectName("chatHistorico");
    m_chatHistorico->setOpenExternalLinks(true);
    m_chatHistorico->setHtml(
        "<div style='color: #94a3b8; font-size: 13px; line-height: 1.5;'>"
        "<b style='color: #60a5fa;'>Assistente Tecnico</b><br>"
        "Faca perguntas conceituais ou anexe circuitos para analise da IA.<br><br>"
        "<b>Instrucoes:</b><br>"
        "• <b>Capturar Circuito:</b> Selecione um esquema diretamente no PDF com o mouse.<br>"
        "• <b>Colar Trecho:</b> Insere o texto selecionado na pergunta.<br>"
        "• <b>Configurar API:</b> Insira sua chave Gemini ou ative o modo offline (Ollama)."
        "</div>"
    );
    layout->addWidget(m_chatHistorico, 1);

    m_promptInput = new PromptInputWidget(this);
    layout->addWidget(m_promptInput);

    connect(m_comboChatIA, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AiAssistantWidget::provedorIaChanged);
    connect(m_btnConfigurarIA, &QPushButton::clicked, this, &AiAssistantWidget::configurarIaRequested);
    connect(m_btnLimparChat,   &QPushButton::clicked, this, &AiAssistantWidget::limparChatRequested);

    if (m_promptInput) {
        connect(m_promptInput, &PromptInputWidget::sendRequested, this, [this](const QString&) {
            emit enviarChatRequested();
        });
        connect(m_promptInput, &PromptInputWidget::captureCircuitRequested, this, &AiAssistantWidget::capturarCircuitoRequested);
        connect(m_promptInput, &PromptInputWidget::attachImageRequested,    this, &AiAssistantWidget::anexarImagemRequested);
        connect(m_promptInput, &PromptInputWidget::removeImageRequested,    this, &AiAssistantWidget::removerImagemRequested);
        connect(m_promptInput, &PromptInputWidget::pasteSnippetRequested,   this, &AiAssistantWidget::colarTrechoRequested);
    }
}

void AiAssistantWidget::resetarHistoricoVisual()
{
    if (!m_chatHistorico) return;
    m_chatHistorico->setHtml(
        "<div style='color: #94a3b8; font-size: 13px; line-height: 1.5;'>"
        "<b style='color: #60a5fa;'>Conversa reiniciada</b><br>"
        "Historico limpo. Faca uma nova pergunta ou anexe um circuito."
        "</div>"
    );
}

void AiAssistantWidget::adicionarMensagemUsuario(const QString &texto, bool temImagem)
{
    if (!m_chatHistorico) return;
    QString userHtml = "<div style='margin-bottom: 12px; margin-top: 6px;'>"
                       "<div style='font-size: 11px; font-weight: 600; color: #38bdf8; margin-bottom: 2px;'>Você</div>"
                       "<div style='background-color: #1e293b; border: 1px solid #334155; border-radius: 8px; padding: 8px 10px; color: #f1f5f9; font-size: 13px; line-height: 1.4;'>";

    if (temImagem) {
        userHtml += "<div style='color: #93c5fd; font-size: 11px; margin-bottom: 4px;'>[Imagem anexada]</div>";
    }
    if (!texto.isEmpty()) {
        userHtml += texto.toHtmlEscaped().replace("\n", "<br>");
    }
    userHtml += "</div></div>";

    m_chatHistorico->append(userHtml);
}

void AiAssistantWidget::adicionarMensagemAssistente(const QString &resposta, const QString &provedor, const QString &modelo)
{
    if (!m_chatHistorico) return;
    const QString formatted = renderizarMarkdownELatex(resposta);

    QString botHtml = QString(
        "<div style='margin-bottom: 16px; margin-top: 6px;'>"
        "<div style='font-size: 11px; font-weight: 600; color: #a78bfa; margin-bottom: 2px;'>Assistente Técnico (%1: %2)</div>"
        "<div style='background-color: #171b24; border: 1px solid #282f3d; border-left: 3px solid #8b5cf6; border-radius: 8px; padding: 10px 12px; color: #e2e8f0; font-size: 13px; line-height: 1.5;'>%3</div>"
        "</div>"
    ).arg(provedor.toUpper(), modelo, formatted);

    m_chatHistorico->append(botHtml);
    if (m_chatHistorico->verticalScrollBar()) {
        m_chatHistorico->verticalScrollBar()->setValue(m_chatHistorico->verticalScrollBar()->maximum());
    }
}

void AiAssistantWidget::adicionarMensagemErro(const QString &mensagemErro)
{
    if (!m_chatHistorico) return;
    m_chatHistorico->append(
        QString("<div style='margin-bottom: 12px; margin-top: 6px; background-color: rgba(239, 68, 68, 0.12); "
                "border: 1px solid rgba(239, 68, 68, 0.35); border-left: 3px solid #ef4444; border-radius: 8px; padding: 10px 12px;'>"
                "<div style='font-size: 11px; font-weight: 700; color: #f87171; margin-bottom: 4px;'>Falha na comunicacao com a IA</div>"
                "<div style='color: #fca5a5; font-size: 13px; line-height: 1.4;'>%1</div>"
                "<div style='margin-top: 6px; font-size: 11px; color: #94a3b8;'>"
                "Verifique sua chave de API em <b>Configurar API</b>.</div>"
                "</div>").arg(mensagemErro.toHtmlEscaped())
    );
    if (m_chatHistorico->verticalScrollBar()) {
        m_chatHistorico->verticalScrollBar()->setValue(m_chatHistorico->verticalScrollBar()->maximum());
    }
}

QString AiAssistantWidget::converterFormulaLatex(QString formula, bool isBlock)
{
    formula = formula.trimmed();
    if (formula.startsWith("$$") && formula.endsWith("$$")) formula = formula.mid(2, formula.length() - 4).trimmed();
    else if (formula.startsWith("$") && formula.endsWith("$")) formula = formula.mid(1, formula.length() - 2).trimmed();
    else if (formula.startsWith("\\[") && formula.endsWith("\\]")) formula = formula.mid(2, formula.length() - 4).trimmed();
    else if (formula.startsWith("\\(") && formula.endsWith("\\)")) formula = formula.mid(2, formula.length() - 4).trimmed();

    formula = formula.toHtmlEscaped();

    static const QRegularExpression reText(R"(\\text\{([^}]+)\})");
    formula.replace(reText, "<span style='font-style:normal;font-family:sans-serif;'>\\1</span>");
    static const QRegularExpression reMathrm(R"(\\mathrm\{([^}]+)\})");
    formula.replace(reMathrm, "<span style='font-style:normal;font-family:sans-serif;'>\\1</span>");

    static const QRegularExpression reFrac(R"(\\frac\{([^}]+)\}\{([^}]+)\})");
    if (isBlock) {
        formula.replace(reFrac,
            "<table style='display:inline-table;vertical-align:middle;border-collapse:collapse;margin:0 4px;font-size:95%;'>"
            "<tr><td style='border-bottom:1px solid #94a3b8;text-align:center;padding:0 4px;'>\\1</td></tr>"
            "<tr><td style='text-align:center;padding:0 4px;'>\\2</td></tr>"
            "</table>");
    } else {
        formula.replace(reFrac, "(\\1 / \\2)");
    }

    formula.replace("\\alpha", "&alpha;");
    formula.replace("\\beta", "&beta;");
    formula.replace("\\gamma", "&gamma;");
    formula.replace("\\Gamma", "&Gamma;");
    formula.replace("\\delta", "&delta;");
    formula.replace("\\Delta", "&Delta;");
    formula.replace("\\epsilon", "&epsilon;");
    formula.replace("\\theta", "&theta;");
    formula.replace("\\lambda", "&lambda;");
    formula.replace("\\Lambda", "&Lambda;");
    formula.replace("\\mu", "&mu;");
    formula.replace("\\pi", "&pi;");
    formula.replace("\\Pi", "&Pi;");
    formula.replace("\\sigma", "&sigma;");
    formula.replace("\\Sigma", "&Sigma;");
    formula.replace("\\tau", "&tau;");
    formula.replace("\\phi", "&phi;");
    formula.replace("\\Phi", "&Phi;");
    formula.replace("\\omega", "&omega;");
    formula.replace("\\Omega", "&Omega;");

    static const QRegularExpression reLatexImplies(R"([/\\]+(?:implies|Rightarrow)\b)", QRegularExpression::CaseInsensitiveOption);
    formula.replace(reLatexImplies, "&rArr;");

    static const QRegularExpression reLatexIff(R"([/\\]+(?:iff|Leftrightarrow)\b)", QRegularExpression::CaseInsensitiveOption);
    formula.replace(reLatexIff, "&hArr;");

    static const QRegularExpression reLatexLeftArrow(R"([/\\]+Leftarrow\b)", QRegularExpression::CaseInsensitiveOption);
    formula.replace(reLatexLeftArrow, "&lArr;");

    static const QRegularExpression reLatexTo(R"([/\\]+(?:to|rightarrow)\b)", QRegularExpression::CaseInsensitiveOption);
    formula.replace(reLatexTo, "&rarr;");

    static const QRegularExpression reLatexLeft(R"([/\\]+leftarrow\b)", QRegularExpression::CaseInsensitiveOption);
    formula.replace(reLatexLeft, "&larr;");

    formula.replace("\\equiv", "&equiv;");
    formula.replace("\\therefore", "&there4;");
    formula.replace("\\sum", "<span style='font-size:120%;font-weight:bold;'>&sum;</span>");
    formula.replace("\\int", "<span style='font-size:120%;font-weight:bold;'>&int;</span>");
    formula.replace("\\approx", "&asymp;");
    formula.replace("\\leq", "&le;");
    formula.replace("\\le", "&le;");
    formula.replace("\\geq", "&ge;");
    formula.replace("\\ge", "&ge;");
    formula.replace("\\neq", "&ne;");
    formula.replace("\\ne", "&ne;");
    formula.replace("\\pm", "&plusmn;");
    formula.replace("\\times", "&times;");
    formula.replace("\\cdot", "&sdot;");
    formula.replace("\\infty", "&infin;");
    formula.replace("\\partial", "&part;");
    formula.replace("\\sqrt", "&radic;");

    static const QRegularExpression reSubBrace(R"(_\{([^}]+)\})");
    formula.replace(reSubBrace, "<sub>\\1</sub>");
    static const QRegularExpression reSupBrace(R"(\^\{([^}]+)\})");
    formula.replace(reSupBrace, "<sup>\\1</sup>");

    static const QRegularExpression reSubSimple(R"(_([a-zA-Z0-9]))");
    formula.replace(reSubSimple, "<sub>\\1</sub>");
    static const QRegularExpression reSupSimple(R"(\^([a-zA-Z0-9]))");
    formula.replace(reSupSimple, "<sup>\\1</sup>");

    if (isBlock) {
        return QString(
            "<div style='margin: 8px 0; padding: 10px 14px; background-color: #161b26; "
            "border: 1px solid #2d3748; border-left: 3px solid #38bdf8; border-radius: 6px; "
            "text-align: center; font-family: \"DejaVu Serif\", \"Cambria Math\", \"Times New Roman\", serif; "
            "font-size: 15px; color: #38bdf8; font-weight: 500;'>"
            "%1"
            "</div>"
        ).arg(formula);
    } else {
        return QString(
            "<span style='font-family: \"DejaVu Serif\", \"Cambria Math\", \"Times New Roman\", serif; "
            "font-style: italic; color: #38bdf8; background-color: #17202f; padding: 1px 5px; "
            "border-radius: 4px; border: 1px solid #253347; font-size: 13px;'>"
            "%1"
            "</span>"
        ).arg(formula);
    }
}

QString AiAssistantWidget::converterTabelaMarkdown(const QString &tableBlock)
{
    QStringList lines = tableBlock.trimmed().split('\n');
    if (lines.size() < 2) return tableBlock;

    QString html = "<table style='width: 100%; border-collapse: collapse; margin: 10px 0; font-size: 12px; background-color: #141822; border: 1px solid #2d3748; border-radius: 6px;'>";

    QString line0 = lines[0].trimmed();
    if (line0.startsWith('|')) line0 = line0.mid(1);
    if (line0.endsWith('|')) line0.chop(1);
    QStringList headers = line0.split('|');
    html += "<tr style='background-color: #1e2638;'>";
    for (const QString &h : headers) {
        html += QString("<th style='border: 1px solid #2d3748; padding: 7px 10px; color: #93c5fd; text-align: left; font-weight: 600;'>%1</th>").arg(h.trimmed().toHtmlEscaped());
    }
    html += "</tr>";

    int startRow = 1;
    if (lines.size() > 1 && lines[1].contains("---")) {
        startRow = 2;
    }

    for (int r = startRow; r < lines.size(); ++r) {
        QString rowLine = lines[r].trimmed();
        if (rowLine.isEmpty()) continue;
        if (rowLine.startsWith('|')) rowLine = rowLine.mid(1);
        if (rowLine.endsWith('|')) rowLine.chop(1);
        QStringList cells = rowLine.split('|');
        html += "<tr style='border-bottom: 1px solid #242c3d;'>";
        for (const QString &c : cells) {
            html += QString("<td style='border: 1px solid #242c3d; padding: 6px 10px; color: #e2e8f0;'>%1</td>").arg(c.trimmed().toHtmlEscaped());
        }
        html += "</tr>";
    }

    html += "</table>";
    return html;
}

QString AiAssistantWidget::renderizarMarkdownELatex(const QString &texto)
{
    QString result = texto;

    QStringList codeBlocks;
    static const QRegularExpression reCodeBlock(R"(```([a-zA-Z0-9_-]*)\n([\s\S]*?)```)");
    auto matchCode = reCodeBlock.match(result);
    while (matchCode.hasMatch()) {
        const QString code = matchCode.captured(2).toHtmlEscaped();
        const QString htmlCode = QString(
            "<pre style='background-color: #151821; border: 1px solid #282f3f; border-radius: 6px; padding: 10px; font-family: monospace; font-size: 12px; color: #38bdf8; overflow-x: auto;'><code>%1</code></pre>"
        ).arg(code);
        const QString token = QString("@@CODE_BLOCK_%1@@").arg(codeBlocks.size());
        codeBlocks.append(htmlCode);
        result.replace(matchCode.capturedStart(), matchCode.capturedLength(), token);
        matchCode = reCodeBlock.match(result);
    }

    QStringList mathBlocks;
    static const QRegularExpression reBlockMath(R"(\$\$([^$]+?)\$\$|\\\[([\s\S]*?)\\\])");
    auto matchBlockMath = reBlockMath.match(result);
    while (matchBlockMath.hasMatch()) {
        QString formula = matchBlockMath.captured(1);
        if (formula.isEmpty()) formula = matchBlockMath.captured(2);
        const QString htmlMath = converterFormulaLatex(formula, true);
        const QString token = QString("@@MATH_BLOCK_%1@@").arg(mathBlocks.size());
        mathBlocks.append(htmlMath);
        result.replace(matchBlockMath.capturedStart(), matchBlockMath.capturedLength(), token);
        matchBlockMath = reBlockMath.match(result);
    }

    QStringList mathInlines;
    static const QRegularExpression reInlineMath(R"((?<!\$)\$([^$\n]+?)\$(?!\$)|\\\(([\s\S]*?)\\\))");
    auto matchInlineMath = reInlineMath.match(result);
    while (matchInlineMath.hasMatch()) {
        QString formula = matchInlineMath.captured(1);
        if (formula.isEmpty()) formula = matchInlineMath.captured(2);
        const QString htmlMath = converterFormulaLatex(formula, false);
        const QString token = QString("@@MATH_INLINE_%1@@").arg(mathInlines.size());
        mathInlines.append(htmlMath);
        result.replace(matchInlineMath.capturedStart(), matchInlineMath.capturedLength(), token);
        matchInlineMath = reInlineMath.match(result);
    }

    QStringList tableBlocks;
    static const QRegularExpression reTable(R"((?:^[ \t]*\|.+?\|[ \t]*\n?){2,})", QRegularExpression::MultilineOption);
    auto matchTable = reTable.match(result);
    while (matchTable.hasMatch()) {
        const QString tblRaw = matchTable.captured(0);
        const QString htmlTbl = converterTabelaMarkdown(tblRaw);
        const QString token = QString("@@TABLE_BLOCK_%1@@").arg(tableBlocks.size());
        tableBlocks.append(htmlTbl);
        result.replace(matchTable.capturedStart(), matchTable.capturedLength(), token);
        matchTable = reTable.match(result);
    }

    result = result.toHtmlEscaped();

    static const QRegularExpression reH3(R"(^###[ \t]+([^\n]+))", QRegularExpression::MultilineOption);
    result.replace(reH3, "<div style='font-size: 14px; font-weight: 700; color: #c084fc; margin-top: 10px; margin-bottom: 4px;'>\\1</div>");
    static const QRegularExpression reH2(R"(^##[ \t]+([^\n]+))", QRegularExpression::MultilineOption);
    result.replace(reH2, "<div style='font-size: 15px; font-weight: 700; color: #a78bfa; margin-top: 12px; margin-bottom: 6px;'>\\1</div>");
    static const QRegularExpression reH1(R"(^#[ \t]+([^\n]+))", QRegularExpression::MultilineOption);
    result.replace(reH1, "<div style='font-size: 16px; font-weight: 700; color: #f1f5f9; margin-top: 14px; margin-bottom: 6px;'>\\1</div>");

    static const QRegularExpression reHr(R"(^---[ \t]*$|^\*\*\*[ \t]*$)", QRegularExpression::MultilineOption);
    result.replace(reHr, "<hr style='border: 0; border-top: 1px solid #282f3d; margin: 10px 0;'>");

    static const QRegularExpression reBold(R"(\*\*([^*]+)\*\*)");
    result.replace(reBold, "<strong style='color: #f8fafc;'>\\1</strong>");
    static const QRegularExpression reItalic(R"((?<!\*)\*([^*\n]+)\*(?!\*))");
    result.replace(reItalic, "<em>\\1</em>");

    static const QRegularExpression reInlineCode(R"(`([^`]+)`)");
    result.replace(reInlineCode, "<code style='background-color: #272a33; color: #38bdf8; padding: 2px 5px; border-radius: 4px; font-family: monospace;'>\\1</code>");

    static const QRegularExpression reBullet(R"(^[ \t]*[\*\-][ \t]+([^\n]+))", QRegularExpression::MultilineOption);
    result.replace(reBullet, "<div style='margin-left: 12px; margin-bottom: 3px;'>• \\1</div>");

    static const QRegularExpression reNumList(R"(^[ \t]*([0-9]+)\.[ \t]+([^\n]+))", QRegularExpression::MultilineOption);
    result.replace(reNumList, "<div style='margin-left: 12px; margin-bottom: 3px;'><b style='color: #a78bfa;'>\\1.</b> \\2</div>");

    result.replace("\n\n", "<br><br>");
    result.replace("\n", "<br>");

    static const QRegularExpression reImpliesText(R"([/\\]+(?:implies|Rightarrow)\b)", QRegularExpression::CaseInsensitiveOption);
    result.replace(reImpliesText, "<span style='font-size:115%; color:#38bdf8;'>&nbsp;&rArr;&nbsp;</span>");

    static const QRegularExpression reIffText(R"([/\\]+(?:iff|Leftrightarrow)\b)", QRegularExpression::CaseInsensitiveOption);
    result.replace(reIffText, "<span style='font-size:115%; color:#38bdf8;'>&nbsp;&hArr;&nbsp;</span>");

    static const QRegularExpression reToText(R"([/\\]+(?:to|rightarrow)\b)", QRegularExpression::CaseInsensitiveOption);
    result.replace(reToText, "<span style='font-size:115%; color:#38bdf8;'>&nbsp;&rarr;&nbsp;</span>");

    static const QRegularExpression reArrowDbl(R"((?<=\s)==&gt;(?=\s)|(?<=\s)=&gt;(?=\s))");
    result.replace(reArrowDbl, "<span style='font-size:115%; color:#38bdf8;'>&nbsp;&rArr;&nbsp;</span>");

    static const QRegularExpression reArrowSgl(R"((?<=\s)--&gt;(?=\s)|(?<=\s)-&gt;(?=\s))");
    result.replace(reArrowSgl, "<span style='font-size:115%; color:#38bdf8;'>&nbsp;&rarr;&nbsp;</span>");

    for (int i = 0; i < tableBlocks.size(); ++i) {
        result.replace(QString("@@TABLE_BLOCK_%1@@").arg(i), tableBlocks[i]);
    }
    for (int i = 0; i < mathBlocks.size(); ++i) {
        result.replace(QString("@@MATH_BLOCK_%1@@").arg(i), mathBlocks[i]);
    }
    for (int i = 0; i < mathInlines.size(); ++i) {
        result.replace(QString("@@MATH_INLINE_%1@@").arg(i), mathInlines[i]);
    }
    for (int i = 0; i < codeBlocks.size(); ++i) {
        result.replace(QString("@@CODE_BLOCK_%1@@").arg(i), codeBlocks[i]);
    }

    return result;
}
