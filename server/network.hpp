#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QByteArray>
#include <QRegExp>
#include <QMutex>

#include "struct.hpp"

class Game;

class Network : public QObject
{
	struct Client
	{
		enum ClientType
		{
            DISCONNECTED,
			UNKNOWN,
			PLAYER,
			DISPLAY
		};

        int receivedMessageCount;
		ClientType type;
		QByteArray buffer;

        Client() : receivedMessageCount(0), type(UNKNOWN) {}
	};

	Q_OBJECT
public:
    explicit Network(quint16 port, int maxPlayerCount, int maxDisplayCount, Game * game = 0, QObject *parent = 0);

	void run();

	int playerCount() const;
	int displayCount() const;
	int clientCount() const;
	bool isListening() const { return _server->isListening(); }
    QVector<QTcpSocket *> clients() const;
    QVector<QTcpSocket *> players() const;
    QVector<QTcpSocket *> displays() const;

signals:
	void loginPlayer(QTcpSocket * socket, QString nickname);
	void loginDisplay(QTcpSocket * socket);

	void movePlayer(QTcpSocket * socket,
					QVector<int> planetsToScan,
					QVector<BuildOrder> shipsToBuild,
					QVector<ShipMove> shipsToMove);

	void playerDisconnected(QTcpSocket * socket);
	void displayDisconnected(QTcpSocket * socket);
	void unloggedClientDisconnected(QTcpSocket * socket);

	void clientConnected(QTcpSocket * socket);

public slots:
	void sendInitPlayer(QTcpSocket * socket,
						int planetCount,
						QVector<QVector<int> > distanceMatrix,
						int roundCount,
						int scanLimit,
						int shipCost,
						int nbPlayers,
						int idPlayer);

    void sendInitDisplay(QTcpSocket * socket,
                         int planetCount,
                         QVector<QVector<int> > distanceMatrix,
                         QVector<InitDisplayPlanet> planets,
                         QVector<QString> playerNicks,
                         int roundCount);

	void sendTurnPlayer(QTcpSocket * socket,
				  int currentRound,
				  int resources,
				  QVector<OurShipsOnPlanets> ourShipsOnPlanet,
				  QVector<ScanResult> scanResults,
				  QVector<OurMovingShips> ourMovingShips,
				  QVector<IncomingEnnemyShips> incomingEnnemies,
				  QVector<FightReport> fightReports);

    void sendTurnDisplay(QTcpSocket * socket,
                         QVector<int> scores,
                         QVector<TurnDisplayPlanet> planets,
                         QVector<ShipMovement> movements);

    void sendFinishedPlayer(QTcpSocket * socket, bool youWon);

private:
	void sendLoginPlayerACK(QTcpSocket * socket, char value);
	void sendLoginDisplayACK(QTcpSocket * socket, char value);

	void debugDisplayMove(QVector<int> planetsToScan,
						  QVector<BuildOrder> shipsToBuild,
						  QVector<ShipMove> shipsToMove);

    Client::ClientType typeOf(QTcpSocket * socket) const;

private slots:
	void onNewConnection();
	void onMessageReceived();
	void onError(QAbstractSocket::SocketError error);
	void onDisconnected();

private:
	QTcpServer * _server;
	quint16 _port;
	QMap<QTcpSocket*, Client> _clients;

	QRegExp _regexMessage;
	QRegExp _regexLoginPlayer;
	QRegExp _regexMovePlayer;
	QRegExp _regexScans;
	QRegExp _regexBuilds;
	QRegExp _regexMoves;

    QMutex _mutexDisconnected;
    bool _isListening;

	int _maxPlayerCount;
	int _maxDisplayCount;

    Game * _game;
};

#endif // NETWORK_HPP
