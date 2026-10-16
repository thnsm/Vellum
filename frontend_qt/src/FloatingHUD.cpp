#include "FloatingHUD.h"
#include <QFrame>

FloatingHUD::FloatingHUD(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("floatingHUD");
    setFixedHeight(46);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 4, 14, 4);
    layout->setSpacing(8);

    m_btnAnt = new QPushButton("◀", this);
    m_btnAnt->setObjectName("btnHudNav");
    m_btnAnt->setToolTip("Página Anterior");
    m_btnAnt->setCursor(Qt::PointingHandCursor);
    connect(m_btnAnt, &QPushButton::clicked, this, &FloatingHUD::paginaAnteriorClicada);

    m_lblPagina = new QLabel("1 / 1", this);
    m_lblPagina->setObjectName("lblHudPagina");

    m_btnProx = new QPushButton("▶", this);
    m_btnProx->setObjectName("btnHudNav");
    m_btnProx->setToolTip("Próxima Página");
    m_btnProx->setCursor(Qt::PointingHandCursor);
    connect(m_btnProx, &QPushButton::clicked, this, &FloatingHUD::paginaProximaClicada);

    auto *sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setObjectName("vseparador");

    m_btnZoomMenos = new QPushButton("−", this);
    m_btnZoomMenos->setObjectName("btnHudNav");
    m_btnZoomMenos->setToolTip("Diminuir Zoom");
    m_btnZoomMenos->setCursor(Qt::PointingHandCursor);
    connect(m_btnZoomMenos, &QPushButton::clicked, this, &FloatingHUD::zoomMenosClicado);

    m_lblZoom = new QLabel("100%", this);
    m_lblZoom->setObjectName("lblHudZoom");

    m_btnZoomMais = new QPushButton("+", this);
    m_btnZoomMais->setObjectName("btnHudNav");
    m_btnZoomMais->setToolTip("Aumentar Zoom");
    m_btnZoomMais->setCursor(Qt::PointingHandCursor);
    connect(m_btnZoomMais, &QPushButton::clicked, this, &FloatingHUD::zoomMaisClicado);

    auto *sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setObjectName("vseparador");

    m_btnSair = new QPushButton("Sair", this);
    m_btnSair->setObjectName("btnHudSair");
    m_btnSair->setToolTip("Sair da Tela Cheia (Esc)");
    m_btnSair->setCursor(Qt::PointingHandCursor);
    connect(m_btnSair, &QPushButton::clicked, this, &FloatingHUD::sairTelaCheiaClicado);

    layout->addWidget(m_btnAnt);
    layout->addWidget(m_lblPagina);
    layout->addWidget(m_btnProx);
    layout->addWidget(sep1);
    layout->addWidget(m_btnZoomMenos);
    layout->addWidget(m_lblZoom);
    layout->addWidget(m_btnZoomMais);
    layout->addWidget(sep2);
    layout->addWidget(m_btnSair);

    m_timerOcultar = new QTimer(this);
    m_timerOcultar->setSingleShot(true);
    connect(m_timerOcultar, &QTimer::timeout, this, [this]() {
        hide();
    });

    hide();
}

void FloatingHUD::atualizarPagina(int atual, int total)
{
    m_lblPagina->setText(QString("%1 / %2").arg(atual).arg(total));
}

void FloatingHUD::atualizarZoom(qreal zoom)
{
    m_lblZoom->setText(QString("%1%").arg(qRound(zoom * 100)));
}

void FloatingHUD::mostrarTemporariamente()
{
    show();
    raise();
    m_timerOcultar->start(2500);
}

void FloatingHUD::enterEvent(QEnterEvent *event)
{
    m_timerOcultar->stop();
    QWidget::enterEvent(event);
}

void FloatingHUD::leaveEvent(QEvent *event)
{
    m_timerOcultar->start(1500);
    QWidget::leaveEvent(event);
}
