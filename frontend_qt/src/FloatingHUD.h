#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>

class FloatingHUD : public QWidget {
    Q_OBJECT
public:
    explicit FloatingHUD(QWidget *parent = nullptr);

    void atualizarPagina(int atual, int total);
    void atualizarZoom(qreal zoom);
    void mostrarTemporariamente();

signals:
    void paginaAnteriorClicada();
    void paginaProximaClicada();
    void zoomMenosClicado();
    void zoomMaisClicado();
    void sairTelaCheiaClicado();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QPushButton *m_btnAnt;
    QLabel      *m_lblPagina;
    QPushButton *m_btnProx;
    QPushButton *m_btnZoomMenos;
    QLabel      *m_lblZoom;
    QPushButton *m_btnZoomMais;
    QPushButton *m_btnSair;

    QTimer      *m_timerOcultar;
};
