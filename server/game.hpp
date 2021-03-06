#ifndef GAME_HPP
#define GAME_HPP

#include "struct.hpp"
#include "planet.hpp"
#include "player.hpp"
#include "galaxy.hpp"
#include "abstractgamemodel.hpp"
#include "network.hpp"

#include <QObject>
#include <QVector>
#include <QTcpSocket>
#include <QString>
#include <QTimer>

class Game : public QObject
{
	Q_OBJECT
	public:
		Game(QString mapFilename, int delayBetweenRound, int roundCount, AbstractGameModel * gameModel);

        int currentRound() const { return m_currentRound; }
        void setPlayers(const QMap<QTcpSocket *, QString> players);
        void setDisplays(const QMap<QTcpSocket *, QString> displays);

	signals:
		void initPlayerSignal(QTcpSocket * socket,
							int planetCount,
							QVector<QVector<int> > distanceMatrix,
							int roundCount,
							int scanLimit,
							int shipCost,
							int nbPlayers,
							int idPlayer);

        void initDisplaySignal(QTcpSocket * socket,
                               int planetCount,
                               QVector<QVector<int> > distanceMatrix,
                               QVector<InitDisplayPlanet> planets,
                               QVector<QString> playerNicks,
                               int roundCount);

        void turnPlayerSignal(QTcpSocket * socket,
					  int currentRound,
					  int resources,
					  QVector<OurShipsOnPlanets> ourShipsOnPlanet,
					  QVector<ScanResult> scanResults,
					  QVector<OurMovingShips> ourMovingShips,
					  QVector<IncomingEnnemyShips> incomingEnnemies,
					  QVector<FightReport> fightReports);

        void turnDisplaySignal(QTcpSocket * socket,
                               QVector<int> scores,
                               QVector<TurnDisplayPlanet> planets,
                               QVector<ShipMovement> movements);

		void finishedSignal(QTcpSocket * socket, bool youWon);

		void gameStarted();
		void roundStarted(int nbRound);

	public slots:
		void playerOrder(QTcpSocket * socket,
						 QVector<int> planetsToScan,
						 QVector<BuildOrder> shipsToBuild,
						 QVector<ShipMove> shipsToMove);

		void playerDisconnected(QTcpSocket * socket);
        void displayDisconnected(QTcpSocket * socket);

		void start();
		void iteration();

		void setDelayBetweenRound(int ms);
		void setNumberOfRound(int round);

	protected:
		Planet * getPlanet(int id);
		Galaxy * getGalaxy(int id);
		Player * getPlayer(int id);

		void filterBuildOrder(QVector<BuildOrder> & orders, Player * player);
		void filterShipMove(QVector<ShipMove> & shipMoves, Player * player);
		void filterScan(QVector<int> & scans);

		QVector<QVector<int> > getDistanceMatrix();

		void sendTurnMessage(QMap<int, QVector<FightReport> > reports);
		QMap<int, QVector<FightReport> > handleBattle(QVector<ShipMovement*> endMovements);
		
		bool hasFleet(Player * p);

        void displayScore();

	private:
		int m_delayBetweenRound;
		int m_currentRound;
		int m_roundCount;
        int m_playerCount;

		AbstractGameModel * m_gameModel;
		QVector<Galaxy*> m_galaxies;
		QVector<Player*> m_players;
        QVector<QTcpSocket*> m_displaySockets;
		QVector<ShipMovement*> m_movements;
		QVector<Planet*> m_planets;
        QMap<Player*, QTcpSocket *> m_playerSocketsMap;

		QTimer * m_timer;

		Player * m_neutralPlayer;
        Player * m_nullPlayer;
};

#endif // GAME_HPP
