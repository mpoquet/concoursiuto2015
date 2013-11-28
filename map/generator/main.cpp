#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>

#include <fstream>
#include <vector>
#include <string>

using namespace std;

struct planet_s
{    
    float x, y;
    
    int size;
    int galaxyId;
    bool initial;
    bool neutral;
};

void usage(const char *name)
{
    printf("usage: \t\t%s file galaxy X Y radius players planets\n", name);
    printf("map: \t\toutput file name\n");
    printf("galaxy: \tid of the galaxy [1-N]\n");
    printf("X, Y: \t\tposition of the galaxy [0-N], [0-N]\n");
    printf("players: \tnumber of players, [1-N]\n");
    printf("planets: \tnumber of planets, [1-N]\n");
    printf("radius: \tradius of the galaxy / 2, [1-N]\n");
    printf("layers: \tnumber of planet layers in the galaxy, [1-N]\n");
    printf("\nExample: %s test.map 1 0 0 100 4 16 4\n", name);
    printf("will result in a galaxy of 16 planets with 4 planets per player.\n");
    printf("If layer = 1, there will be one circle of planets.\n");
    printf("If layer = N, there will be N of planets.\n");
}

int main(int argc, char **argv)
{
    string filename;
    int galaxyId = -1, X = -1, Y = -1, nPlayers = -1, nPlanets = -1, radius = -1, nLayers = -1;
    
    if(argc < 9)
    {
        usage(argv[0]);
        return 0;
    }
    
    filename = argv[1];
    galaxyId = atoi(argv[2]);
    X = atoi(argv[3]);
    Y = atoi(argv[4]);
    radius = atoi(argv[5]);
    nPlayers = atoi(argv[6]);
    nPlanets = atoi(argv[7]);
    nLayers = atoi(argv[8]);
    
    if(filename.empty() || galaxyId < 1 || radius < 1 || X < 0 || Y < 0 || nPlayers < 1 || nPlanets < 1)
    {
        usage(argv[0]);
        return 0;
    }
    
    printf("Generating %s with id=%d, X=%d, Y=%d, radius=%d, players=%d, planets=%d, layers=%d\n", 
        filename.c_str(), galaxyId, X, Y, radius, nPlayers, nPlanets, nLayers);
    
    srand(time(0));
    
    int nPlanetPerPlayer = -1;
    int nPlanetPerLayer = -1;
    int nPlanetRest = -1;
    
    nPlanetPerPlayer = max(nPlanets / nPlayers, 1);
    nPlanetPerLayer = max(nPlanetPerPlayer / nLayers, 1);
    nPlanetRest = (nPlanetPerPlayer == 1) ? 0 : nPlanetPerPlayer % nLayers;
	
    vector<planet_s> planets(nPlayers * nPlanetPerPlayer);

    // Pour chaque anneau de planètes,
    // on répartie les planètes équitablement
    float angle = (360 / nPlayers);

    // On place toutes les planètes à la même distance du centre (range)
    int id = 0;
    int range = rand() % radius + 1;
    for(int i = 0; i < nPlayers; ++i)
    {
        for(int j = 0; j <= nLayers; ++j)
        {
            int nPlanetInLayer = -1;
            if(j < nLayers) nPlanetInLayer = nPlanetPerLayer;
            else if(j == nLayers) nPlanetInLayer = nPlanetRest;

            for(int k = 0; k < nPlanetInLayer; ++k)
            {
                struct planet_s p;
                
                // Calcul des coordonnées polaires
                float planetAngle = (angle * i) + (angle / nPlanetPerLayer) * k;
                int planetRadius = range;

                // Calcul des coordonnées cartésiennes
                p.x = X + cos(planetAngle) * radius * (j + 1);
                p.y = Y + sin(planetAngle) * radius * (j + 1);
                
                p.galaxyId = galaxyId;
                p.size = 1;
                p.initial = false;
                p.neutral = false;
                
                planets[id++] = p;
            }
        }
        
        // Une planète initiale par joueur ...
        planets[id-1].initial = true;
    }
    
    ofstream out(filename.c_str(), ofstream::out);
    
    for(unsigned int i = 0; i < planets.size(); ++i)
    {
        planet_s &p = planets[i];
        
        // write map
        out << (int)p.x << " " << (int)p.y << " " << p.size << " " << p.galaxyId << " " << p.initial << " " << p.neutral << "\n";
    }
    
    out.close();
    
    return 0;
}
