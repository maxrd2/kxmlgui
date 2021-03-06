/* This file is part of the KDE libraries
    Copyright (c) 2006 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <QTest>
#include "kmainwindow_unittest.h"
#include <QEventLoopLocker>
#include <kmainwindow.h>
#include <QStatusBar>
#include <QResizeEvent>
#include <ktoolbar.h>
#include <ksharedconfig.h>
#include <kconfiggroup.h>

QTEST_MAIN(KMainWindow_UnitTest)

void KMainWindow_UnitTest::initTestCase()
{
    QStandardPaths::enableTestMode(true);
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + KSharedConfig::openConfig()->name());
}

void KMainWindow_UnitTest::cleanupTestCase()
{
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + KSharedConfig::openConfig()->name());
}

void KMainWindow_UnitTest::testDefaultName()
{
    KMainWindow mw;
    mw.show();
    mw.ensurePolished();
    QCOMPARE(mw.objectName(), QString::fromLatin1("MainWindow#1"));
    KMainWindow mw2;
    mw2.show();
    mw2.ensurePolished();
    QCOMPARE(mw2.objectName(), QString::fromLatin1("MainWindow#2"));
}

void KMainWindow_UnitTest::testFixedName()
{
    KMainWindow mw;
    mw.setObjectName(QStringLiteral("mymainwindow"));
    mw.show();
    mw.ensurePolished();
    QCOMPARE(mw.objectName(), QString::fromLatin1("mymainwindow"));
    KMainWindow mw2;
    mw2.setObjectName(QStringLiteral("mymainwindow"));
    mw2.show();
    mw2.ensurePolished();
    QCOMPARE(mw2.objectName(), QString::fromLatin1("mymainwindow2"));
}

void KMainWindow_UnitTest::testNameWithHash()
{
    KMainWindow mw;
    mw.setObjectName(QStringLiteral("composer#"));
    mw.show();
    mw.ensurePolished();
    QCOMPARE(mw.objectName(), QString::fromLatin1("composer#1"));
    KMainWindow mw2;
    mw2.setObjectName(QStringLiteral("composer#"));
    mw2.show();
    mw2.ensurePolished();
    QCOMPARE(mw2.objectName(), QString::fromLatin1("composer#2"));
    KMainWindow mw4;
    mw4.setObjectName(QStringLiteral("composer#4"));
    mw4.show();
    mw4.ensurePolished();
    QCOMPARE(mw4.objectName(), QString::fromLatin1("composer#4"));
}

void KMainWindow_UnitTest::testNameWithSpecialChars()
{
    KMainWindow mw;
    mw.setObjectName(QStringLiteral("a#@_test/"));
    mw.show();
    mw.ensurePolished();
    QCOMPARE(mw.dbusName(), QString::fromLatin1("/kmainwindow_unittest/a___test_"));
    KMainWindow mw2;
    mw2.setObjectName(QStringLiteral("a#@_test/"));
    mw2.show();
    mw2.ensurePolished();
    QCOMPARE(mw2.dbusName(), QString::fromLatin1("/kmainwindow_unittest/a___test_2"));
}

static bool s_mainWindowDeleted;
class MyMainWindow : public KMainWindow
{
public:
    MyMainWindow() : KMainWindow(),
        m_queryClosedCalled(false)
    {
    }
    bool queryClose() Q_DECL_OVERRIDE
    {
        m_queryClosedCalled = true;
        return true;
    }
    ~MyMainWindow()
    {
        s_mainWindowDeleted = true;
    }
    bool m_queryClosedCalled;

    void reallyResize(int width, int height)
    {
        const QSize oldSize = size();

        resize(width, height);

        // Send the pending resize event (resize() only sets Qt::WA_PendingResizeEvent)
        QResizeEvent e(size(), oldSize);
        QApplication::sendEvent(this, &e);

        QCOMPARE(this->width(), width);
        QCOMPARE(this->height(), height);
    }
};

// Here we test
// - that queryClose is called
// - that autodeletion happens
void KMainWindow_UnitTest::testDeleteOnClose()
{
    QEventLoopLocker locker; // don't let the deref in KMainWindow quit the app.
    s_mainWindowDeleted = false;
    MyMainWindow *mw = new MyMainWindow;
    QVERIFY(mw->testAttribute(Qt::WA_DeleteOnClose));
    mw->close();
    QVERIFY(mw->m_queryClosedCalled);
    qApp->sendPostedEvents(mw, QEvent::DeferredDelete);
    QVERIFY(s_mainWindowDeleted);
}

void KMainWindow_UnitTest::testSaveWindowSize()
{
    QCOMPARE(KSharedConfig::openConfig()->name(), QString::fromLatin1("kmainwindow_unittestrc"));
    KConfigGroup cfg(KSharedConfig::openConfig(), "TestWindowSize");

    {
        MyMainWindow mw;
        mw.show();
        KToolBar *tb = new KToolBar(&mw); // we need a toolbar to trigger an old bug in saveMainWindowSettings
        tb->setObjectName(QStringLiteral("testtb"));
        mw.reallyResize(800, 600);
        QTRY_COMPARE(mw.size(), QSize(800, 600));
        QTRY_COMPARE(mw.windowHandle()->size(), QSize(800, 600));
        mw.saveMainWindowSettings(cfg);
        mw.close();
    }

    KMainWindow mw2;
    mw2.show();
    KToolBar *tb = new KToolBar(&mw2);
    tb->setObjectName(QStringLiteral("testtb"));
    mw2.resize(500, 500);
    mw2.applyMainWindowSettings(cfg);

    QTRY_COMPARE(mw2.size(), QSize(800, 600));
}

void KMainWindow_UnitTest::testAutoSaveSettings()
{
    const QString group(QStringLiteral("AutoSaveTestGroup"));

    {
        MyMainWindow mw;
        mw.show();
        KToolBar *tb = new KToolBar(&mw); // we need a toolbar to trigger an old bug in saveMainWindowSettings
        tb->setObjectName(QStringLiteral("testtb"));
        mw.setAutoSaveSettings(group);
        mw.reallyResize(800, 600);
        mw.close();
    }

    KMainWindow mw2;
    mw2.show();
    KToolBar *tb = new KToolBar(&mw2);
    tb->setObjectName(QStringLiteral("testtb"));
    mw2.setAutoSaveSettings(group);
    QTRY_COMPARE(mw2.size(), QSize(800, 600));
}

void KMainWindow_UnitTest::testNoAutoSave()
{
    const QString group(QStringLiteral("AutoSaveTestGroup"));

    {
        // A mainwindow with autosaving, but not of the window size.
        MyMainWindow mw;
        mw.show();
        mw.setAutoSaveSettings(group, false);
        mw.reallyResize(750, 550);
        mw.close();
    }

    KMainWindow mw2;
    mw2.show();
    mw2.setAutoSaveSettings(group, false);
    // NOT 750, 550! (the 800,600 comes from testAutoSaveSettings)
    QTRY_COMPARE(mw2.size(), QSize(800, 600));
}

void KMainWindow_UnitTest::testWidgetWithStatusBar()
{
    // KMainWindow::statusBar() should not find any indirect QStatusBar child
    // (e.g. in a case like konqueror, with one statusbar per frame)
    MyMainWindow mw;
    QWidget *frame1 = new QWidget(&mw);
    QStatusBar *frameStatusBar = new QStatusBar(frame1);
    QVERIFY(mw.statusBar() != frameStatusBar);
}
