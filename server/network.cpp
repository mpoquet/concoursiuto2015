#include "network.hpp"

#include <iostream>

#include <QStringList>

#include "protocole.h"

using namespace std;

Network::Network(quint16 port, int maxPlayerCount, int maxDisplayCount, QObject *parent) :
    QObject(parent),
    _server(new QTcpServer(this)),
    _port(port),
    _maxPlayerCount(maxPlayerCount),
    _maxDisplayCount(maxDisplayCount)
{
    if (_maxPlayerCount < 0)
        _maxPlayerCount = 0;

    if (_maxDisplayCount < 0)
        _maxDisplayCount = 0;

    connect(_server, &QTcpServer::newConnection, this, &Network::onNewConnection);

    _regexMessage.setPattern(QString("([%1%2%3])(.*)").arg(LOGIN_PLAYER).arg(LOGIN_DISPLAY).arg(MOVE_PLAYER));
    _regexLoginPlayer.setPattern("[a-zA-Z0-9_\\-]{1,10}");
    _regexMovePlayer.setPattern("(.*)\\t(.*)\\t(.*)");
    _regexScans.setPattern("(\\d+)(?: \\d+)*");
    _regexBuilds.setPattern("(\\d+)(?: \\d+ \\d+)*");
    _regexMoves.setPattern("(\\d+)(?: \\d+ \\d+ \\d+)*");

    Q_ASSERT(_regexMessage.isValid());
    Q_ASSERT(_regexLoginPlayer.isValid());
    Q_ASSERT(_regexMovePlayer.isValid());
    Q_ASSERT(_regexScans.isValid());
    Q_ASSERT(_regexBuilds.isValid());
    Q_ASSERT(_regexMoves.isValid());
}

void Network::run()
{
    if (!_server->listen(QHostAddress::Any, _port))
        cerr << "Cannot listen port " << _port << endl;
    else
        cout << "Listening port " << _port << endl;
}

int Network::playerCount() const
{
    int count = 0;

    QMapIterator<QTcpSocket *, Client> it(_clients);

    while (it.hasNext())
    {
        it.next();

        if (it.value().type == Client::PLAYER)
            ++count;
    }

    return count;
}

int Network::displayCount() const
{
    int count = 0;

    QMapIterator<QTcpSocket *, Client> it(_clients);

    while (it.hasNext())
    {
        it.next();

        if (it.value().type == Client::DISPLAY)
            ++count;
    }

    return count;
}

void Network::sendLoginPlayerACK(QTcpSocket *socket, char value)
{
    QByteArray message(2, '\n');
    message[0] = value;
    socket->write(message);
}

void Network::sendLoginDisplayACK(QTcpSocket *socket, char value)
{
    QByteArray message(2, '\n');
    message[0] = value;
    socket->write(message);
}

void Network::sendInitPlayer(QTcpSocket *socket,
                             int planetCount,
                             QVector<QVector<int> > distanceMatrix,
                             int roundCount,
                             int scanLimit)
{
}

void Network::sendFinished(QTcpSocket *socket, bool youWon)
{
}

void Network::onNewConnection()
{
    QTcpSocket * socket = _server->nextPendingConnection();

    cout << QString("New connection from %1").arg(socket->peerAddress().toString()).toStdString() << endl;

    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onError(QAbstractSocket::SocketError)));
    connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(onMessageReceived()));

    _clients[socket] = Client();
}

void Network::onMessageReceived()
{
    QTcpSocket * socket = (QTcpSocket *) sender();

    _clients[socket].buffer += socket->readAll();

    int index;

    do
    {
        index = _clients[socket].buffer.indexOf('\n');

        if (index != -1)
        {
            QString message = _clients[socket].buffer.left(index);
            _clients[socket].buffer.remove(0, index+1);

            if (_regexMessage.exactMatch(message))
            {
                char type = _regexMessage.cap(1).at(0).toLatin1();
                QString content = _regexMessage.cap(2);

                switch (type)
                {
                case LOGIN_PLAYER:
                {
                    if (_clients[socket].type == Client::UNKNOWN)
                    {
                        if (playerCount() < _maxPlayerCount)
                        {
                            if (_regexLoginPlayer.exactMatch(content))
                            {
                                _clients[socket].type = Client::PLAYER;

                                emit loginPlayer(socket, content);
                                sendLoginPlayerACK(socket, OK);
                            }
                            else
                                sendLoginPlayerACK(socket, INVALID_NICKNAME);
                        }
                        else
                            sendLoginPlayerACK(socket, NO_MORE_ROOM);
                    }
                    else
                        sendLoginPlayerACK(socket, ALREADY_LOGGED);

                } break;

                case LOGIN_DISPLAY:
                {
                    if (_clients[socket].type == Client::UNKNOWN)
                    {
                        if (displayCount() < _maxDisplayCount)
                        {
                            _clients[socket].type = Client::DISPLAY;

                            emit loginDisplay(socket);
                            sendLoginPlayerACK(socket, OK);
                        }
                        else
                            sendLoginPlayerACK(socket, NO_MORE_ROOM);
                    }
                    else
                        sendLoginPlayerACK(socket, ALREADY_LOGGED);
                } break;

                case MOVE_PLAYER:
                {
                    if (_clients[socket].type == Client::PLAYER)
                    {
                        if (_regexMovePlayer.exactMatch(content))
                        {
                            bool validMove = true;

                            QString scans = _regexMovePlayer.cap(1);
                            QString builds = _regexMovePlayer.cap(2);
                            QString moves = _regexMovePlayer.cap(3);

                            QVector<int> planetsToScan;
                            QVector<BuildOrder> shipsToBuild;
                            QVector<ShipMove> shipsToMove;

                            if (_regexScans.exactMatch(scans))
                            {
                                int scanCount = _regexScans.cap(1).toInt();

                                if (scanCount < 0)
                                    scanCount = 0;

                                QStringList qsl = scans.split(" ");
                                int realScanCount = qsl.size() - 1;

                                if (scanCount == realScanCount)
                                {
                                    planetsToScan.resize(scanCount);

                                    for (int i = 0; i < scanCount; ++i)
                                        planetsToScan[i] = qsl[i+1].toInt();

                                    if (_regexBuilds.exactMatch(builds))
                                    {
                                        int buildCount = _regexBuilds.cap(1).toInt();

                                        if (buildCount < 0)
                                            buildCount = 0;

                                        qsl = builds.split(" ");
                                        int realBuildCount = (qsl.size() - 1) / 2;
                                        int remainder = (qsl.size() - 1) % 2;

                                        if (buildCount == realBuildCount && remainder == 0)
                                        {
                                            shipsToBuild.resize(buildCount);

                                            for (int i = 0; i < buildCount; ++i)
                                            {
                                                shipsToBuild[i].planet = qsl[1 + 2*i].toInt();
                                                shipsToBuild[i].shipCount = qsl[1 + 2*i + 1].toInt();
                                            }

                                            if (_regexMoves.exactMatch(moves))
                                            {
                                                int moveCount = _regexMoves.cap(1).toInt();

                                                if (moveCount < 0)
                                                    moveCount = 0;

                                                qsl = moves.split(" ");
                                                int realMoveCount = (qsl.size() - 1) / 3;
                                                remainder = (qsl.size() - 1) % 3;

                                                if (moveCount == realMoveCount && remainder == 0)
                                                {
                                                    shipsToMove.resize(moveCount);

                                                    for (int i = 0; i < moveCount; ++i)
                                                    {
                                                        shipsToMove[i].srcPlanet = qsl[1 + 3*i].toInt();
                                                        shipsToMove[i].destPlanet = qsl[1 + 3*i + 1].toInt();
                                                        shipsToMove[i].shipCount = qsl[1 + 3*i + 2].toInt();
                                                    }

                                                    emit movePlayer(socket, planetsToScan, shipsToBuild, shipsToMove);
                                                }
                                                else
                                                    cerr << QString("Invalid message : moves count mismatch in move (%1)").arg(moves).toStdString();
                                            }
                                            else
                                                cerr << QString("Invalid message : invalid moves in move (%1)").arg(moves).toStdString();
                                        }
                                        else
                                            cerr << QString("Invalid message : build count mismatch in move (%1)").arg(builds).toStdString();
                                    }
                                    else
                                        cerr << QString("Invalid message : invalid build in move (%1)").arg(builds).toStdString();
                                }
                                else
                                    cerr << QString("Invalid message : scan count mismatch in move (%1)").arg(scans).toStdString();
                            }
                            else
                                cerr << QString("Invalid message : invalid scan in move (%1)").arg(scans).toStdString();
                        }
                        else
                            cerr << QString("Invalid message : invalid move (%1)").arg(content).toStdString();
                    }
                    else
                        cerr << "Invalid message : move from a not-player client";
                } break;
                }
            }
            else
                qDebug() << "Invalid message received" << message;
        }

    } while (index != -1);
}

void Network::onError(QAbstractSocket::SocketError error)
{
    if (error != QAbstractSocket::RemoteHostClosedError)
        qDebug() << error;
}

void Network::onDisconnected()
{
    QTcpSocket * socket = (QTcpSocket *) sender();

    qDebug() << "Client disconnected";

    _clients.remove(socket);
    socket->deleteLater();
}