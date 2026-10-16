#include "SelectionOverlayWidget.h"
#include <QPainterPath>

SelectionOverlayWidget::SelectionOverlayWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("selectionOverlayWidget");
    setAttribute(Qt::WA_TransparentForMouseEvents, true); // Permite que cliques passem se necessário, controlados via eventFilter
    setAttribute(Qt::WA_NoSystemBackground, true);
    hide();
}

void SelectionOverlayWidget::setFormato(Formato f)
{
    m_formato = f;
}

void SelectionOverlayWidget::iniciar(const QPoint &pt)
{
    m_ativo = true;
    m_ptOrigem = pt;
    m_ptAtual = pt;
    m_pontos.clear();
    m_pontos.append(pt);

    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }
    show();
    raise();
    update();
}

void SelectionOverlayWidget::atualizar(const QPoint &pt)
{
    if (!m_ativo) return;

    m_ptAtual = pt;
    if (m_formato == Formato::Livre) {
        // Evita adicionar pontos idênticos consecutivos
        if (m_pontos.isEmpty() || (m_pontos.last() - pt).manhattanLength() > 2) {
            m_pontos.append(pt);
        }
    }
    update();
}

void SelectionOverlayWidget::finalizar()
{
    m_ativo = false;
    hide();
    update();
}

void SelectionOverlayWidget::cancelar()
{
    m_ativo = false;
    m_pontos.clear();
    hide();
    update();
}

QRect SelectionOverlayWidget::boundingBox() const
{
    if (m_formato == Formato::Retangular) {
        return QRect(m_ptOrigem, m_ptAtual).normalized();
    }
    if (m_pontos.size() >= 2) {
        return m_pontos.boundingRect().toRect();
    }
    return QRect();
}

QPolygonF SelectionOverlayWidget::poligono() const
{
    if (m_formato == Formato::Retangular) {
        const QRect r = QRect(m_ptOrigem, m_ptAtual).normalized();
        return QPolygonF(QRectF(r));
    }
    return m_pontos;
}

void SelectionOverlayWidget::paintEvent(QPaintEvent *)
{
    if (!m_ativo) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_formato == Formato::Retangular) {
        const QRect r = QRect(m_ptOrigem, m_ptAtual).normalized();
        if (r.width() < 2 || r.height() < 2) return;

        // Fundo azul translúcido e borda sólida azul Vellum (#1A62CD)
        painter.setBrush(QColor(26, 98, 205, 50));
        painter.setPen(QPen(QColor(26, 98, 205, 230), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRect(r);
    } else {
        if (m_pontos.size() < 2) return;

        // Desenho livre: preenchimento suave e linha tracejada antisserrilhada
        painter.setBrush(QColor(26, 98, 205, 50));
        painter.setPen(QPen(QColor(26, 98, 205, 240), 2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPolygon(m_pontos);
    }
}
