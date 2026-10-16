#include <QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QTextEdit>
#include "../src/QuickLookupPopover.h"

class TestQuickLookupPopover : public QObject {
    Q_OBJECT

private slots:
    void testModosETransicoes() {
        QuickLookupPopover popover;
        QCOMPARE(popover.modoAtual(), QuickLookupPopover::Modo::Traducao);

        popover.exibirTutor("Maxwell's equations are fundamental.", QPoint(100, 100));
        QCOMPARE(popover.modoAtual(), QuickLookupPopover::Modo::Tutor);

        popover.exibirAssistenteIA("Calculando a impedância da linha", QString(), QPoint(100, 100));
        QCOMPARE(popover.modoAtual(), QuickLookupPopover::Modo::AssistenteIA);

        popover.exibirAnotacoes("Lembrar de revisar o Smith Chart.", QPoint(100, 100));
        QCOMPARE(popover.modoAtual(), QuickLookupPopover::Modo::BlocoNotas);

        popover.exibirTraducao("Impedance matching", "Casamento de impedância", "phi3", QPoint(100, 100));
        QCOMPARE(popover.modoAtual(), QuickLookupPopover::Modo::Traducao);
    }

    void testSizeGripEMovimento() {
        QuickLookupPopover popover;
        auto *grip = popover.findChild<QSizeGrip*>("popoverSizeGrip");
        QVERIFY(grip != nullptr);
        QVERIFY(popover.minimumWidth() >= 360);
        QVERIFY(popover.minimumHeight() >= 220);
    }

    void testTraducaoEditavelERetraducao() {
        QuickLookupPopover popover;
        popover.exibirTraducao("Circuit design", "Projeto de circuito", "gemini", QPoint(100, 100));

        auto *txtOriginal = popover.findChild<QTextEdit*>("txtPopoverOriginal");
        QVERIFY(txtOriginal != nullptr);
        QCOMPARE(txtOriginal->toPlainText(), QString("Circuit design"));

        QSignalSpy spyRetraducao(&popover, &QuickLookupPopover::solicitarRetraducao);

        // Usuário adiciona palavras
        txtOriginal->setPlainText("Advanced circuit design");
        QTest::qWait(700); // Aguarda o debounce de 600ms

        QVERIFY(spyRetraducao.count() >= 1);
        QCOMPARE(spyRetraducao.takeFirst().at(0).toString(), QString("Advanced circuit design"));
    }

    void testCapturarImagensModos() {
        QFile qssFile(":/themes/dark.qss");
        QString qss;
        if (qssFile.open(QIODevice::ReadOnly)) {
            qss = QString::fromUtf8(qssFile.readAll());
            qssFile.close();
        }

        QuickLookupPopover popover;
        if (!qss.isEmpty()) popover.setStyleSheet(qss);
        popover.resize(480, 320);

        // 1. Tradução
        popover.exibirTraducao("Negative impedance converter using operational amplifier.",
                               "Conversor de impedância negativa utilizando amplificador operacional.",
                               "Gemini", QPoint(200, 200));
        QTest::qWait(100);
        popover.grab().save("/home/thnsm/.gemini/antigravity-ide/brain/652f3ada-fd30-406a-a8f3-a13a01182de0/popover_traducao_editavel.png");

        // 2. Tutor
        popover.exibirTutor("Feedback topology in multi-stage amplifiers.", QPoint(200, 200));
        popover.atualizarResultadoTutor(88, "Excelente pronúncia das consoantes plosivas.", "Feedback topology in multi-stage amplifiers.", {});
        QTest::qWait(100);
        popover.grab().save("/home/thnsm/.gemini/antigravity-ide/brain/652f3ada-fd30-406a-a8f3-a13a01182de0/popover_tutor_voz.png");
    }
};

QTEST_MAIN(TestQuickLookupPopover)
#include "tst_QuickLookupPopover.moc"
