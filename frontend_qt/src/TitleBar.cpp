#include "TitleBar.h"
#include <QApplication>
#include <QStyle>

#include <QPixmap>
#include <QFile>

TitleBar::TitleBar(QMainWindow *parent)
    : QWidget(parent), m_parentWindow(parent), m_isDragging(false)
{
    setObjectName("titleBar");
    setFixedHeight(32);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 0, 0);
    layout->setSpacing(8);

    m_logoLabel = new QLabel(this);
    m_logoLabel->setObjectName("titleBarLogo");
    m_logoLabel->setAlignment(Qt::AlignCenter);

    QPixmap logo(":/logo.png");
    if (logo.isNull()) logo.load(":/logo_transparent.png");
    if (logo.isNull()) logo.load(":/LOGO.png");
    if (logo.isNull()) logo.load(":/app_icon.png");
    if (logo.isNull() && QFile::exists("logo.png")) logo.load("logo.png");
    if (logo.isNull() && QFile::exists("LOGO.png")) logo.load("LOGO.png");

    if (!logo.isNull()) {
        m_logoLabel->setPixmap(logo.scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    m_logoLabel->setFixedSize(20, 20);

    m_titleLabel = new QLabel("Vellum", this);
    m_titleLabel->setObjectName("titleBarTitle");
    
    m_btnMinimize = new QPushButton("-", this);
    m_btnMinimize->setObjectName("btnTitleMin");

    m_btnMaximize = new QPushButton("☐", this);
    m_btnMaximize->setObjectName("btnTitleMax");

    m_btnClose = new QPushButton("✕", this);
    m_btnClose->setObjectName("btnTitleClose");

    layout->addWidget(m_logoLabel);
    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_btnMinimize);
    layout->addWidget(m_btnMaximize);
    layout->addWidget(m_btnClose);

    connect(m_btnMinimize, &QPushButton::clicked, this, &TitleBar::onMinimize);
    connect(m_btnMaximize, &QPushButton::clicked, this, &TitleBar::onMaximizeRestore);
    connect(m_btnClose, &QPushButton::clicked, this, &TitleBar::onClose);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_parentWindow) {
        m_isDragging = true;
        const QPoint globalPt = event->globalPosition().toPoint();
        m_dragPosition = globalPt - m_parentWindow->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) && m_isDragging && m_parentWindow) {
        const QPoint globalPt = event->globalPosition().toPoint();
        if (m_parentWindow->isMaximized()) {
            const double ratioX = double(event->position().x()) / double(qMax(1, width()));
            m_parentWindow->showNormal();
            m_btnMaximize->setText("☐");
            m_dragPosition = QPoint(int(m_parentWindow->width() * ratioX), int(event->position().y()));
        }
        m_parentWindow->move(globalPt - m_dragPosition);
        event->accept();
    }
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_isDragging = false;
    event->accept();
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        onMaximizeRestore();
        event->accept();
    }
}

void TitleBar::onMinimize()
{
    if (m_parentWindow)
        m_parentWindow->showMinimized();
}

void TitleBar::onMaximizeRestore()
{
    if (m_parentWindow) {
        if (m_parentWindow->isMaximized()) {
            m_parentWindow->showNormal();
            m_btnMaximize->setText("☐");
        } else {
            m_parentWindow->showMaximized();
            m_btnMaximize->setText("❐");
        }
    }
}

void TitleBar::onClose()
{
    if (m_parentWindow)
        m_parentWindow->close();
}
