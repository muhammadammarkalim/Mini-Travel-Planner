
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <climits>
#include <sstream>

using namespace std;

/// Terminal colours
#define RESET    "\033[0m"
#define RED      "\033[1;31m"
#define GREEN    "\033[1;32m"
#define YELLOW   "\033[1;33m"
#define CYAN     "\033[1;36m"
#define WHITE    "\033[1;37m"
#define BOLD     "\033[1m"
#define MAGENTA  "\033[1;35m"
#define DIM      "\033[2m"

// Data structures
struct Road {
    string dest;
    int    dist = 0;
    int    safety = 0;   // 1–10
    int    scenic = 0;   // 1–10
    int    adventure = 0;   // 1–10
    Road* next = nullptr;
};

struct City {
    string name;
    Road* roadHead = nullptr;
    City* nextCity = nullptr;
    // Dijkstra / Prim fields (reset before each run)
    bool   visited = false;
    int    weight = INT_MAX;
    string parent;
};

// Global city list (singly-linked)
static City* masterCityList = nullptr;

// UI helpers

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void printBanner() {
    cout << MAGENTA << "\n\t   " << string(65, '=') << "\n";
    cout << "\t      " << BOLD << WHITE
        << "P A K I S T A N   T R A V E L   P L A N N E R   v3.0"
        << MAGENTA << "\n";
    cout << "\t   " << string(65, '=') << RESET << "\n";
}

void printBoxHeader(const string& title) {
    const int W = 63;
    int pad = (W - (int)title.size()) / 2;
    pad = (pad < 0) ? 0 : pad;
    cout << CYAN << "\n\t   +" << string(W, '-') << "+\n";
    cout << "\t   |" << string(pad, ' ') << BOLD << WHITE << title
        << string(W - pad - (int)title.size(), ' ') << CYAN << "|\n";
    cout << "\t   +" << string(W, '-') << "+" << RESET << "\n";
}

void showMapStats() {
    int   totalCities = 0, totalRoads = 0, maxRoads = -1;
    string busiest = "None";
    for (City* c = masterCityList; c; c = c->nextCity) {
        totalCities++;
        int count = 0;
        for (Road* r = c->roadHead; r; r = r->next) ++count;
        totalRoads += count;
        if (count > maxRoads) { maxRoads = count; busiest = c->name; }
    }
    // Each road is stored in both directions, so divide by 2 for display
    cout << "\t   " << CYAN << ">> " << WHITE << "Cities: " << YELLOW << totalCities
        << WHITE << "  |  Connections: " << YELLOW << (totalRoads / 2)
        << WHITE << "  |  Hub: " << CYAN << busiest << RESET << "\n";
}

void pressEnter() {
    cout << DIM << "\n\t   [Press Enter to continue...]" << RESET;
    cin.ignore(1000, '\n');
    // consume the newline if cin left one
    if (cin.peek() == '\n') cin.ignore();
}

// Core graph operations

City* findCity(const string& name) {
    for (City* c = masterCityList; c; c = c->nextCity)
        if (c->name == name) return c;
    return nullptr;
}

City* getOrCreateCity(const string& name) {
    City* c = findCity(name);
    if (c) return c;
    City* nc = new City;
    nc->name = name;
    nc->nextCity = masterCityList;
    masterCityList = nc;
    return nc;
}

void addRoad(const string& u, const string& v,
    int d, int saf, int sce, int adv,
    bool persist = false) {
    // Validate ratings
    if (saf < 1 || saf > 10 || sce < 1 || sce > 10 ||
        adv < 1 || adv > 10 || d <= 0) {
        cout << RED << "\t   [!] Invalid values. "
            "Distance>0, ratings 1-10.\n" << RESET;
        return;
    }
    City* src = getOrCreateCity(u);
    Road* r = new Road;
    r->dest = v; r->dist = d; r->safety = saf;
    r->scenic = sce; r->adventure = adv;
    r->next = src->roadHead;
    src->roadHead = r;

    if (persist) {
        ofstream f("cities.txt", ios::app);
        if (f) f << u << " " << v << " " << d
            << " " << saf << " " << sce << " " << adv << "\n";
    }
}

void resetGraph() {
    for (City* c = masterCityList; c; c = c->nextCity) {
        c->visited = false;
        c->weight = INT_MAX;
        c->parent = "";
    }
}

// Algorithm 1: BFS reachability

void findReachableCities(const string& startName) {
    resetGraph();
    City* start = findCity(startName);
    if (!start) {
        cout << RED << "\t   [!] City \"" << startName << "\" not found.\n" << RESET;
        return;
    }
    printBoxHeader("REACHABILITY FROM  " + startName);

    // Simple BFS queue via array (max 200 cities)
    const int QMAX = 200;
    City* queue[QMAX];
    int head = 0, tail = 0;
    queue[tail++] = start;
    start->visited = true;
    int count = 0;

    while (head < tail && tail < QMAX) {
        City* cur = queue[head++];
        count++;
        cout << "\t      " << CYAN << "-> " << WHITE << cur->name << "\n";
        for (Road* r = cur->roadHead; r; r = r->next) {
            City* nb = findCity(r->dest);
            if (nb && !nb->visited && tail < QMAX) {
                nb->visited = true;
                queue[tail++] = nb;
            }
        }
    }
    cout << "\n\t   " << BOLD << WHITE << "Total reachable cities: "
        << YELLOW << count << RESET << "\n";
    pressEnter();
}

// Algorithm 2: Dijkstra best route
// Criteria: 1=Distance 2=Safest 3=Scenic 4=Adventure

static int edgeCost(const Road* r, int criteria) {
    switch (criteria) {
    case 1: return r->dist;
    case 2: return 11 - r->safety;
    case 3: return 11 - r->scenic;
    case 4: return 11 - r->adventure;
    default: return r->dist;
    }
}

void findBestRoute(const string& start, const string& end, int criteria) {
    resetGraph();
    City* src = findCity(start);
    City* dst = findCity(end);

    if (!src) { cout << RED << "\t   [!] City \"" << start << "\" not found.\n" << RESET; return; }
    if (!dst) { cout << RED << "\t   [!] City \"" << end << "\" not found.\n" << RESET; return; }
    if (src == dst) { cout << YELLOW << "\t   [!] Source and destination are the same.\n" << RESET; return; }

    src->weight = 0;

    // O(V^2) Dijkstra: extract min unvisited, relax its neighbours
    int cityCount = 0;
    for (City* c = masterCityList; c; c = c->nextCity) ++cityCount;

    for (int iter = 0; iter < cityCount; ++iter) {
        // Find unvisited city with minimum weight
        City* u = nullptr;
        for (City* c = masterCityList; c; c = c->nextCity)
            if (!c->visited && c->weight != INT_MAX)
                if (!u || c->weight < u->weight) u = c;

        if (!u) break;   // all remaining nodes are unreachable
        u->visited = true;

        // Relax edges from u
        for (Road* r = u->roadHead; r; r = r->next) {
            City* v = findCity(r->dest);
            if (!v || v->visited) continue;
            int newCost = u->weight + edgeCost(r, criteria);
            if (newCost < v->weight) {
                v->weight = newCost;
                v->parent = u->name;
            }
        }
    }

    if (dst->weight == INT_MAX) {
        cout << RED << "\t   [!] No route found between "
            << start << " and " << end << ".\n" << RESET;
        pressEnter();
        return;
    }

    // Reconstruct path (walk parent chain, guard against cycles)
    printBoxHeader("OPTIMISED ROUTE: " + start + " -> " + end);
    string path[100];
    int    pathLen = 0;
    City* cur = dst;
    int    guard = 0;

    while (cur && pathLen < 100 && guard < 200) {
        path[pathLen++] = cur->name;
        if (cur->parent.empty()) break;
        City* prev = findCity(cur->parent);
        if (!prev || prev == cur) break;
        cur = prev;
        guard++;
    }

    // Display path top->bottom (reversed)
    for (int i = pathLen - 1; i >= 0; --i) {
        bool isEndpoint = (i == pathLen - 1 || i == 0);
        cout << "\t\t      |\n";
        cout << "\t\t " << (isEndpoint ? RED : WHITE)
            << "  [ " << BOLD << path[i] << RESET << " ]\n";
        if (i > 0) cout << "\t\t      V\n";
    }

    const char* metricLabel[] = { "", "Distance", "Safety score", "Scenic score", "Adventure score" };
    cout << "\n\t   " << YELLOW << metricLabel[criteria] << " optimised score: "
        << WHITE << dst->weight << RESET << "\n";
    pressEnter();
}

// Algorithm 3: Prim's minimum spanning tree

void findMinimumNetwork() {
    resetGraph();
    if (!masterCityList) {
        cout << RED << "\t   [!] No cities loaded.\n" << RESET;
        return;
    }
    masterCityList->weight = 0;
    printBoxHeader("MINIMUM SPANNING NETWORK  (Prim's)");

    int total = 0;

    while (true) {
        // Extract minimum unvisited
        City* minN = nullptr;
        int   minV = INT_MAX;
        for (City* c = masterCityList; c; c = c->nextCity)
            if (!c->visited && c->weight < minV) { minV = c->weight; minN = c; }

        if (!minN) break;
        minN->visited = true;

        if (!minN->parent.empty()) {
            total += minN->weight;
            cout << "\t   " << WHITE << setw(18) << right << minN->parent
                << YELLOW << " <---> "
                << WHITE << left << setw(18) << minN->name
                << RESET << "(" << minN->weight << " km)\n";
        }

        // Relax neighbours
        for (Road* r = minN->roadHead; r; r = r->next) {
            City* v = findCity(r->dest);
            if (v && !v->visited && r->dist < v->weight) {
                v->weight = r->dist;
                v->parent = minN->name;
            }
        }
    }
    cout << "\n\t   " << BOLD << WHITE << "Total infrastructure distance: "
        << YELLOW << total << " km" << RESET << "\n";
    pressEnter();
}

// Feature: view database with ratings

void viewDatabase() {
    printBoxHeader("REGISTERED CITY DATABASE");
    if (!masterCityList) {
        cout << "\t   " << DIM << "(no cities loaded)\n" << RESET;
        pressEnter();
        return;
    }
    cout << "\t   " << CYAN << left
        << setw(20) << "City"
        << setw(20) << "Connected to"
        << setw(10) << "Dist(km)"
        << setw(8) << "Safety"
        << setw(8) << "Scenic"
        << setw(10) << "Adventr"
        << RESET << "\n";
    cout << "\t   " << string(74, '-') << "\n";

    for (City* c = masterCityList; c; c = c->nextCity) {
        bool first = true;
        for (Road* r = c->roadHead; r; r = r->next) {
            cout << "\t   " << WHITE;
            if (first) { cout << left << setw(20) << c->name; first = false; }
            else        cout << setw(20) << " ";

            // Star ratings (filled vs empty blocks)
            auto stars = [](int v, int max = 10) -> string {
                string s;
                int filled = (v * 5) / max;
                for (int i = 0; i < 5; i++) s += (i < filled ? "*" : ".");
                return s + " (" + to_string(v) + ")";
                };

            cout << setw(20) << r->dest
                << setw(10) << r->dist
                << setw(12) << stars(r->safety)
                << setw(12) << stars(r->scenic)
                << setw(12) << stars(r->adventure)
                << RESET << "\n";
        }
        if (!masterCityList->roadHead) // isolated city
            cout << left << setw(20) << c->name << DIM << " (no roads)\n" << RESET;
    }
    pressEnter();
}

// Feature: add new road

void addNewRoad() {
    string s, e, diS, saS, scS, adS;
    cout << "\t   Source City : "; getline(cin, s);
    cout << "\t   Dest City   : "; getline(cin, e);
    cout << "\t   Distance km : "; getline(cin, diS);
    cout << "\t   Safety  1-10: "; getline(cin, saS);
    cout << "\t   Scenic  1-10: "; getline(cin, scS);
    cout << "\t   Adventr 1-10: "; getline(cin, adS);

    if (s.empty() || e.empty()) {
        cout << RED << "\t   [!] City names cannot be empty.\n" << RESET;
        pressEnter();
        return;
    }
    try {
        int d = stoi(diS), saf = stoi(saS), sce = stoi(scS), adv = stoi(adS);
        addRoad(s, e, d, saf, sce, adv, true);
        addRoad(e, s, d, saf, sce, adv, false);   // bidirectional (not persisted twice)
        cout << GREEN << "\n\t   [OK] Road added and saved to cities.txt.\n" << RESET;
    }
    catch (...) {
        cout << RED << "\n\t   [!] Invalid numeric input. Road not added.\n" << RESET;
    }
    pressEnter();
}

// Feature: find isolated cities (no roads)

void findIsolatedCities() {
    printBoxHeader("ISOLATED CITIES  (no connections)");
    int count = 0;
    for (City* c = masterCityList; c; c = c->nextCity) {
        if (!c->roadHead) {
            cout << "\t      " << YELLOW << "- " << WHITE << c->name << "\n";
            ++count;
        }
    }
    if (count == 0)
        cout << "\t   " << GREEN << "All cities are connected.\n" << RESET;
    else
        cout << "\t   " << BOLD << WHITE << "Total isolated: " << YELLOW << count << RESET << "\n";
    pressEnter();
}

// Memory cleanup

void clearMemory() {
    City* city = masterCityList;
    while (city) {
        Road* road = city->roadHead;
        while (road) {
            Road* del = road;
            road = road->next;
            delete del;
        }
        City* del = city;
        city = city->nextCity;
        delete del;
    }
    masterCityList = nullptr;
}

// File loading

bool loadFromFile(const string& filename) {
    ifstream f(filename);
    if (!f) return false;

    string line;
    int loaded = 0;
    while (getline(f, line)) {
        if (line.empty() || line[0] == '#') continue; // skip comments
        istringstream ss(line);
        string u, v; int d, saf, sce, adv;
        if (ss >> u >> v >> d >> saf >> sce >> adv) {
            addRoad(u, v, d, saf, sce, adv);
            addRoad(v, u, d, saf, sce, adv);
            ++loaded;
        }
    }
    cout << GREEN << "\t   [OK] Loaded " << loaded
        << " route(s) from " << filename << ".\n" << RESET;
    return true;
}

// Main

int main() {
    clearScreen();
    printBanner();

    if (!loadFromFile("cities.txt"))
        cout << YELLOW << "\t   [!] cities.txt not found. Starting with empty graph.\n" << RESET;

    string line;
    int choice = 0;

    do {
        clearScreen();
        printBanner();
        showMapStats();

        cout << CYAN
            << "\t   +----------------------------------------------------------+\n"
            << "\t   |                                                          |\n"
            << "\t   |  " << WHITE << "1. VIEW DATABASE   " << CYAN << "|  " << WHITE << "5. ADD NEW ROAD          " << CYAN << "|\n"
            << "\t   |                                                          |\n"
            << "\t   |  " << WHITE << "2. REACHABILITY    " << CYAN << "|  " << WHITE << "6. ISOLATED CITIES       " << CYAN << "|\n"
            << "\t   |                                                          |\n"
            << "\t   |  " << WHITE << "3. PLAN BEST ROUTE " << CYAN << "|  " << WHITE << "7. MINIMUM NETWORK       " << CYAN << "|\n"
            << "\t   |                                                          |\n"
            << "\t   |  " << WHITE << "0. EXIT            " << CYAN << "|                             |\n"
            << "\t   |                                                          |\n"
            << "\t   +----------------------------------------------------------+\n"
            << RESET;
        cout << "\t   " << BOLD << WHITE << "Selection >> " << RESET;

        getline(cin, line);
        if (line.empty()) continue;
        try { choice = stoi(line); }
        catch (...) { choice = -1; }

        switch (choice) {
        case 1: viewDatabase();          break;
        case 2: {
            string s;
            cout << "\t   Starting city: "; getline(cin, s);
            findReachableCities(s);
            break;
        }
        case 3: {
            string s, e, cStr; int c;
            cout << "\t   From         : "; getline(cin, s);
            cout << "\t   To           : "; getline(cin, e);
            cout << "\t   Criterion    : 1=Distance  2=Safety  3=Scenic  4=Adventure\n";
            cout << "\t   Choice       : "; getline(cin, cStr);
            try { c = stoi(cStr); }
            catch (...) { c = 1; }
            if (c < 1 || c > 4) { c = 1; cout << YELLOW << "\t   (defaulting to Distance)\n" << RESET; }
            findBestRoute(s, e, c);
            break;
        }
        case 5: addNewRoad();            break;
        case 6: findIsolatedCities();    break;
        case 7: findMinimumNetwork();    break;
        case 0: break;
        default:
            cout << RED << "\t   [!] Invalid option. Try again.\n" << RESET;
            pressEnter();
        }
    } while (choice != 0);

    clearMemory();
    cout << GREEN << "\n\t   Goodbye!\n\n" << RESET;
    return 0;
}
