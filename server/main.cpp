#include <iostream>

#include <QApplication>
#include <QStringList>

#include <QTime>

#include "network.hpp"
#include "game.hpp"
#include "basicgamemodel.hpp"
#include "servermanagerwidget.hpp"

using namespace std;

int main(int argc, char ** argv)
{	
    try
    {
        QApplication app(argc, argv);

        bool ok;
        quint16 port = 4242;
        QString mapname = "../map/example.map";

        if (argc > 1)
        {
            int p = app.arguments().at(1).toInt(&ok);

            if (!ok || p <= 1024 || p > 65535)
            {
                cout << "Invalid port. Must be in ]1024;65535]" << endl;
                return 0;
            }

            port = p;
        }

        if (argc > 2)
            mapname = argv[2];

        qsrand(QTime::currentTime().msec());

        Game * g = new Game(mapname, 500, 100, new BasicGameModel());
        Network * n = new Network(port, 16, 1, g);

        // Connexions entre le jeu et les joueurs
        QObject::connect(n, SIGNAL(playerDisconnected(QTcpSocket*)), g, SLOT(playerDisconnected(QTcpSocket*)));
        QObject::connect(n, SIGNAL(movePlayer(QTcpSocket*,QVector<int>,QVector<BuildOrder>,QVector<ShipMove>)),
                         g, SLOT(playerOrder(QTcpSocket*,QVector<int>,QVector<BuildOrder>,QVector<ShipMove>)));
        QObject::connect(g, SIGNAL(finishedSignal(QTcpSocket*,bool)), n, SLOT(sendFinishedPlayer(QTcpSocket*,bool)));
        QObject::connect(g, SIGNAL(initPlayerSignal(QTcpSocket*,int,QVector<QVector<int> >,int,int,int,int,int)),
                         n, SLOT(sendInitPlayer(QTcpSocket*,int,QVector<QVector<int> >,int,int,int,int,int)));
        QObject::connect(g, SIGNAL(turnPlayerSignal(QTcpSocket*,int,int,QVector<OurShipsOnPlanets>,QVector<ScanResult>,QVector<OurMovingShips>,QVector<IncomingEnnemyShips>,QVector<FightReport>)),
                         n, SLOT(sendTurnPlayer(QTcpSocket*,int,int,QVector<OurShipsOnPlanets>,QVector<ScanResult>,QVector<OurMovingShips>,QVector<IncomingEnnemyShips>,QVector<FightReport>)));

        // Connexions entre le jeu et les observateurs
        QObject::connect(n, SIGNAL(displayDisconnected(QTcpSocket*)), g, SLOT(displayDisconnected(QTcpSocket*)));
        QObject::connect(g, SIGNAL(initDisplaySignal(QTcpSocket*,int,QVector<QVector<int> >,QVector<InitDisplayPlanet>,QVector<QString>,int)),
                         n, SLOT(sendInitDisplay(QTcpSocket*,int,QVector<QVector<int> >,QVector<InitDisplayPlanet>,QVector<QString>,int)));
        QObject::connect(g, SIGNAL(turnDisplaySignal(QTcpSocket*,QVector<int>,QVector<TurnDisplayPlanet>,QVector<ShipMovement>)),
                         n, SLOT(sendTurnDisplay(QTcpSocket*,QVector<int>,QVector<TurnDisplayPlanet>,QVector<ShipMovement>)));

        ServerManagerWidget * w = new ServerManagerWidget(g, n);

        n->run();
        w->show();

        return app.exec();
    }
    catch(...)
    {
        return 0;
    }
}
