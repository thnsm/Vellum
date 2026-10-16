#pragma once

#include <QWidget>
#include <QPolygonF>
#include <QPoint>
#include <QRect>
#include <QPainter>

class SelectionOverlayWidget : public QWidget {
    Q_OBJECT
public:
    enum class Formato { Retangular, Livre };

    explicit SelectionOverlayWidget(QWidget *parent = nullptr);

    void setFormato(Formato f);
    Formato formato() const { return m_formato; }
    bool isLivre() const { return m_formato == Formato::Livre; }
    bool isAtivo() const { return m_ativo; }

    void iniciar(const QPoint &pt);
    void atualizar(const QPoint &pt);
    void finalizar();
    void cancelar();

    QRect boundingBox() const;
    QPolygonF poligono() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Formato   m_formato = Formato::Retangular;
    bool      m_ativo   = false;
    QPoint    m_ptOrigem;
    QPoint    m_ptAtual;
    QPolygonF m_pontos;
};
