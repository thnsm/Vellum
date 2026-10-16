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
        QVERIFY(popover.maximumWidth() <= 920);
        QVERIFY(popover.maximumHeight() <= 740);

        popover.exibirTraducao("Test", "Teste", "IA", QPoint(300, 300));
        const QRect initialGeom = popover.geometry();

        // Simula arrasto na borda esquerda tentando expandir exageradamente (-2000px)
        QMouseEvent pressLeft(QEvent::MouseButtonPress, QPoint(2, initialGeom.height() / 2),
                              popover.mapToGlobal(QPoint(2, initialGeom.height() / 2)),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&popover, &pressLeft);

        QMouseEvent moveLeft(QEvent::MouseMove, QPoint(-2000, initialGeom.height() / 2),
                             popover.mapToGlobal(QPoint(2, initialGeom.height() / 2)) - QPoint(2000, 0),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&popover, &moveLeft);

        QMouseEvent releaseLeft(QEvent::MouseButtonRelease, QPoint(2, initialGeom.height() / 2),
                                popover.mapToGlobal(QPoint(2, initialGeom.height() / 2)),
                                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(&popover, &releaseLeft);

        // Garante que a largura respeitou o limite máximo e não expandiu infinitamente
        QVERIFY(popover.width() <= popover.maximumWidth());
        QVERIFY(popover.width() >= popover.minimumWidth());
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

    void testMarkdownELatexNoChatEPararAudio() {
        QuickLookupPopover popover;
        popover.exibirAssistenteIA("Análise de malha", QString(), QPoint(200, 200));
        popover.atualizarRespostaChat("A impedância equivalente é **Z_eq** com $\\Omega$ e $\\alpha$.", "Gemini");

        auto *txtChat = popover.findChild<QTextBrowser*>("txtPopoverChat");
        QVERIFY(txtChat != nullptr);
        const QString plain = txtChat->toPlainText();
        QVERIFY(plain.contains("Ω"));
        QVERIFY(plain.contains("α"));

        QSignalSpy spyParar(&popover, &QuickLookupPopover::pararAudioClicado);
        popover.exibirTutor("Feedback topology in multi-stage amplifiers.", QPoint(200, 200));
        popover.atualizarResultadoTutor(88, "Excelente pronúncia das consoantes plosivas.", "Feedback topology in multi-stage amplifiers.", {});
        QCOMPARE(spyParar.count(), 0);
    }
};

QTEST_MAIN(TestQuickLookupPopover)
#include "tst_QuickLookupPopover.moc"
