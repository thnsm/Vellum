#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>

class PromptInputWidget;

/**
 * SpeechTutorWidget
 * ─────────────────
 * Aba modular do Tutor de Pronúncia & Voz com realce fonético colorido palavra por palavra
 * (Verde = correto, Amarelo = aproximado/sotaque, Vermelho = ausente/incorreto).
 */
class SpeechTutorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpeechTutorWidget(QWidget *parent = nullptr);

    QComboBox* comboVoz() const { return m_comboVoz; }
    QComboBox* comboVelocidade() const { return m_comboVelocidade; }
    QComboBox* comboNivelTutor() const { return m_comboNivelTutor; }
    PromptInputWidget* promptTextoTutor() const { return m_promptTextoTutor; }
    QPushButton* btnOuvir() const { return m_btnOuvir; }
    QPushButton* btnPararAudio() const { return m_btnPararAudio; }
    QPushButton* btnGravar() const { return m_btnGravar; }
    QProgressBar* barAcuracia() const { return m_barAcuracia; }
    QTextEdit* txtFeedbackTutor() const { return m_txtFeedbackTutor; }

    void setRecordingState(bool gravando);
    void exibirAvaliacao(int nota,
                         const QString &feedback,
                         const QString &textoFalado,
                         const QStringList &palavrasAusentes,
                         const QString &htmlDestacado = QString());

signals:
    void ouvirPronunciaRequested();
    void pararAudioRequested();
    void gravarVozRequested();

private:
    void setupUi();

    QComboBox         *m_comboVoz          = nullptr;
    QComboBox         *m_comboVelocidade   = nullptr;
    QComboBox         *m_comboNivelTutor   = nullptr;
    PromptInputWidget *m_promptTextoTutor  = nullptr;
    QPushButton       *m_btnOuvir          = nullptr;
    QPushButton       *m_btnPararAudio     = nullptr;
    QPushButton       *m_btnGravar         = nullptr;
    QProgressBar      *m_barAcuracia       = nullptr;
    QTextEdit         *m_txtFeedbackTutor  = nullptr;
};
