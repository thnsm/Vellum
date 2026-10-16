#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QMainWindow>

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QMainWindow *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onMinimize();
    void onMaximizeRestore();
    void onClose();

private:
    QMainWindow *m_parentWindow;
    bool m_isDragging;
    QPoint m_dragPosition;
    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QPushButton *m_btnMinimize;
    QPushButton *m_btnMaximize;
    QPushButton *m_btnClose;
};

#endif // TITLEBAR_H
