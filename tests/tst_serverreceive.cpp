#include <QString>
#include <QtTest>
#include <QTcpSocket>
#include <QDebug>

#include "../server/network.hpp"
#include "../server/protocole.h"

class ServerReceive : public QObject
{
	Q_OBJECT
public:
	ServerReceive();

private Q_SLOTS:
	void initTestCase();

    void testConnection();
	void testLogin();
    void testFinished();
    void testTurnPlayer();

private:
	Network n;
    int delay;
};

ServerReceive::ServerReceive() :
    n(4242, 2, 1),
    delay(100)
{
}

void ServerReceive::initTestCase()
{
	n.run();
	QVERIFY(n.isListening());
}

void ServerReceive::testConnection()
{
	QCOMPARE(n.clientCount(), 0);

	QTcpSocket sock;
	QCOMPARE(sock.state(), QAbstractSocket::UnconnectedState);

    qRegisterMetaType<QTcpSocket*>("QTcpSocket*");
    QSignalSpy spyConnected(&n, SIGNAL(clientConnected(QTcpSocket*)));
    QSignalSpy spyDisconnected(&n, SIGNAL(unloggedClientDisconnected(QTcpSocket*)));

    sock.connectToHost("127.0.0.1", 4242);

    while (spyConnected.count() == 0)
        QTest::qWait(delay);

    QCOMPARE(spyConnected.count(), 1);
	QCOMPARE(sock.state(), QAbstractSocket::ConnectedState);
	QCOMPARE(n.clientCount(), 1);

    sock.disconnectFromHost();

    while (spyDisconnected.count() == 0)
        QTest::qWait(delay);

    QCOMPARE(spyDisconnected.count(), 1);
}

void ServerReceive::testLogin()
{
	QByteArray response, expected, send;
    QTcpSocket sockP, sockP2, sockP3, sockD, sockD2;

    QSignalSpy spyConnected(&n, SIGNAL(clientConnected(QTcpSocket*)));
    QSignalSpy spyPlayerDisconnected(&n, SIGNAL(playerDisconnected(QTcpSocket*)));
    QSignalSpy spyDisplayDisconnected(&n, SIGNAL(displayDisconnected(QTcpSocket*)));

    sockP.connectToHost("127.0.0.1", 4242);
    sockP2.connectToHost("127.0.0.1", 4242);
    sockP3.connectToHost("127.0.0.1", 4242);
    sockD.connectToHost("127.0.0.1", 4242);
    sockD2.connectToHost("127.0.0.1", 4242);

    while(spyConnected.count() != 5)
        QTest::qWait(delay);

    // Simple login player
    send = QString("%1bouh\n").arg(LOGIN_PLAYER).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_PLAYER_ACK).arg(OK).toLatin1();
    sockP.write(send);

    QTest::qWait(delay);
    response = sockP.readAll();
    QCOMPARE(response, expected);

    // A player wants to login again as a player
    send = QString("%1bouh\n").arg(LOGIN_PLAYER).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_PLAYER_ACK).arg(ALREADY_LOGGED).toLatin1();
    sockP.write(send);

    QTest::qWait(delay);
    response = sockP.readAll();
    QCOMPARE(response, expected);

    // A player wants to login as a display
    send = QString("%1bouh\n").arg(LOGIN_DISPLAY).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_DISPLAY_ACK).arg(ALREADY_LOGGED).toLatin1();
    sockP.write(send);

    QTest::qWait(delay);
    response = sockP.readAll();
    QCOMPARE(response, expected);

    // Simple login display
    send = QString("%1bouh\n").arg(LOGIN_DISPLAY).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_DISPLAY_ACK).arg(OK).toLatin1();
    sockD.write(send);

    QTest::qWait(delay);
    response = sockD.readAll();
    QCOMPARE(response, expected);

    // A display wants to login again as a display
    send = QString("%1bouh\n").arg(LOGIN_DISPLAY).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_DISPLAY_ACK).arg(ALREADY_LOGGED).toLatin1();
    sockD.write(send);

    QTest::qWait(delay);
    response = sockD.readAll();
    QCOMPARE(response, expected);

    // A display wants to login as a player
    send = QString("%1bouh\n").arg(LOGIN_PLAYER).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_PLAYER_ACK).arg(ALREADY_LOGGED).toLatin1();
    sockD.write(send);

    QTest::qWait(delay);
    response = sockD.readAll();
    QCOMPARE(response, expected);

    // A second display wants to connect, but the max display count is set to 1
    send = QString("%1bouh\n").arg(LOGIN_DISPLAY).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_DISPLAY_ACK).arg(NO_MORE_ROOM).toLatin1();
    sockD2.write(send);

    QTest::qWait(delay);
    response = sockD2.readAll();
    QCOMPARE(response, expected);

    // The first display disconnects. The second one might be able to login as a display
    sockD.disconnectFromHost();
    QTest::qWait(delay);

    QCOMPARE(sockD.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(spyDisplayDisconnected.count(), 1);

    send = QString("%1bouh\n").arg(LOGIN_DISPLAY).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_DISPLAY_ACK).arg(OK).toLatin1();
    sockD2.write(send);

    QTest::qWait(delay);
    response = sockD2.readAll();
    QCOMPARE(response, expected);

    // A second player logs in, might be OK
    send = QString("%1bouh\n").arg(LOGIN_PLAYER).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_PLAYER_ACK).arg(OK).toLatin1();
    sockP2.write(send);

    QTest::qWait(delay);
    response = sockP2.readAll();
    QCOMPARE(response, expected);

    // A third player wants to log in, but the limit is 2
    send = QString("%1bouh\n").arg(LOGIN_PLAYER).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_PLAYER_ACK).arg(NO_MORE_ROOM).toLatin1();
    sockP3.write(send);

    QTest::qWait(delay);
    response = sockP3.readAll();
    QCOMPARE(response, expected);

    // The second player disconnects. The third player might now be able to log in
    sockP2.disconnectFromHost();
    QTest::qWait(delay);

    QCOMPARE(sockP2.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(spyPlayerDisconnected.count(), 1);

    send = QString("%1bouh\n").arg(LOGIN_PLAYER).toLatin1();
    expected = QString("%1%2\n").arg(LOGIN_PLAYER_ACK).arg(OK).toLatin1();
    sockP3.write(send);

    QTest::qWait(delay);
    response = sockP3.readAll();
    QCOMPARE(response, expected);

    // End of login test
    sockP.close();
    sockP2.close();
    sockP3.close();
    sockD.close();
    sockD2.close();

    QTest::qWait(delay);
    QCOMPARE(sockP.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(sockP2.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(sockP3.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(sockD.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(sockD2.state(), QAbstractSocket::UnconnectedState);
    QCOMPARE(n.clientCount(), 0);
}

void ServerReceive::testFinished()
{
    QTcpSocket sockc;
    QTcpSocket * socks;
    QByteArray response, expected;

    sockc.connectToHost("127.0.0.1", 4242);
    QTest::qWait(delay);

    QCOMPARE(sockc.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(n.clientCount(), 1);

    socks = n.clients().first();

    // Win send
    n.sendFinished(socks, true);
    QTest::qWait(delay);

    expected = QString("%1%2\n").arg(END_OF_GAME).arg('1').toLatin1();
    response = sockc.readAll();
    QCOMPARE(response, expected);

    // Lose send
    n.sendFinished(socks, false);
    QTest::qWait(delay);

    expected = QString("%1%2\n").arg(END_OF_GAME).arg('0').toLatin1();
    response = sockc.readAll();
    QCOMPARE(response, expected);
}

void ServerReceive::testTurnPlayer()
{
    QTcpSocket sockc;
    QTcpSocket * socks;
    QByteArray response, expected;

    sockc.connectToHost("127.0.0.1", 4242);
    QTest::qWait(delay);

    QCOMPARE(sockc.state(), QAbstractSocket::ConnectedState);
    QCOMPARE(n.clientCount(), 1);

    socks = n.clients().first();

    // Data declaration
    int resources;
    QVector<OurShipsOnPlanets> ourShips;
    QVector<ScanResult> scanResults;
    QVector<OurMovingShips> ourMovingShips;
    QVector<IncomingEnnemyShips> incomingEnnemies;
    QVector<FightReport> fightReports;

    // Let's do several tests
    for (int nbTests = 0; nbTests < 50; ++nbTests)
    {
        // Let's fill this data with random stuff
        resources = rand() % 500;
        ourShips.resize(rand() % 50);
        scanResults.resize(rand() % 50);
        ourMovingShips.resize(rand() % 50);
        incomingEnnemies.resize(rand() % 50);
        fightReports.resize(rand() % 50);

        for (int i = 0; i < ourShips.size(); ++i)
        {
            ourShips[i].planet = rand() % 50;
            ourShips[i].resourcePerRound = rand() % 50;
            ourShips[i].maxBuildPerRound = rand() % 50;
            ourShips[i].shipCount = rand() % 50;
        }

        for (int i = 0; i < scanResults.size(); ++i)
        {
            scanResults[i].planet = rand() % 50;
            scanResults[i].player = rand() % 50;
            scanResults[i].shipCount = rand() % 50;
            scanResults[i].resourcePerRound = rand() % 50;
            scanResults[i].maxBuildPerRound = rand() % 50;
        }

        for (int i = 0; i < ourMovingShips.size(); ++i)
        {
            ourMovingShips[i].srcPlanet = rand() % 50;
            ourMovingShips[i].destPlanet = rand() % 50;
            ourMovingShips[i].shipCount = rand() % 50;
            ourMovingShips[i].remainingTurns = rand() % 50;
        }

        for (int i = 0; i < incomingEnnemies.size(); ++i)
        {
            incomingEnnemies[i].srcPlanet = rand() % 50;
            incomingEnnemies[i].destPlanet = rand() % 50;
            incomingEnnemies[i].shipCount = rand() % 50;
            incomingEnnemies[i].remainingTurns = rand() % 50;
        }

        for (int i = 0; i < fightReports.size(); ++i)
        {
            fightReports[i].planet = rand() % 50;
            fightReports[i].winner = rand() % 50;
            fightReports[i].playerCount = rand() % 50;
            fightReports[i].aliveShipCount = rand() % 50;
        }

        // Let's prepare the according buffer
        expected.clear();
        expected += TURN_PLAYER;
        expected += QString("%1%2").arg(resources).arg(SEP).toLatin1();
        expected += QString("%1").arg(ourShips.size()).toLatin1();

        for (int i = 0; i < ourShips.size(); ++i)
            expected += QString("%1%2%1%3%1%4%1%5").arg(SSEP).arg(
                        ourShips[i].planet).arg(ourShips[i].resourcePerRound).arg(
                        ourShips[i].maxBuildPerRound).arg(ourShips[i].shipCount).toLatin1();

        expected += QString("%1%2").arg(SEP).arg(scanResults.size()).toLatin1();

        for (int i = 0; i < scanResults.size(); ++i)
            expected += QString("%1%2%1%3%1%4%1%5%1%6").arg(SSEP).arg(
                        scanResults[i].planet).arg(scanResults[i].player).arg(
                        scanResults[i].resourcePerRound).arg(scanResults[i].maxBuildPerRound).arg(
                        scanResults[i].shipCount).toLatin1();

        expected += QString("%1%2").arg(SEP).arg(ourMovingShips.size()).toLatin1();

        for (int i = 0; i < ourMovingShips.size(); ++i)
            expected += QString("%1%2%1%3%1%4%1%5").arg(SSEP).arg(ourMovingShips[i].srcPlanet).arg(
                        ourMovingShips[i].destPlanet).arg(ourMovingShips[i].shipCount).arg(
                        ourMovingShips[i].remainingTurns).toLatin1();

        expected += QString("%1%2").arg(SEP).arg(incomingEnnemies.size()).toLatin1();

        for (int i = 0; i < incomingEnnemies.size(); ++i)
            expected += QString("%1%2%1%3%1%4%1%5").arg(SSEP).arg(incomingEnnemies[i].srcPlanet).arg(
                        incomingEnnemies[i].destPlanet).arg(incomingEnnemies[i].shipCount).arg(
                        incomingEnnemies[i].remainingTurns).toLatin1();

        expected += QString("%1%2").arg(SEP).arg(fightReports.size()).toLatin1();

        for (int i = 0; i < fightReports.size(); ++i)
            expected += QString("%1%2%1%3%1%4%1%5").arg(SSEP).arg(fightReports[i].planet).arg(
                        fightReports[i].winner).arg(fightReports[i].playerCount).arg(
                        fightReports[i].aliveShipCount).toLatin1();

        expected += '\n';

        n.sendTurnPlayer(socks, resources, ourShips,
                         scanResults, ourMovingShips,
                         incomingEnnemies, fightReports);

        QTest::qWait(delay);

        response = sockc.readAll();
        QCOMPARE(response, expected);
    }
}

QTEST_GUILESS_MAIN(ServerReceive)


#include "tst_serverreceive.moc"
