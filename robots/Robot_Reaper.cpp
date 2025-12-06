#include "RobotBase.h"
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <random>
#include <string>
#include <iostream>
#include <cstdlib>
//#include "Logger.h"

//LOCAL logger JUST for Reaper
namespace {
    std::ofstream& reaper_log() {
        static std::ofstream out("reaper_stats.csv",
                                 std::ios::out | std::ios::app);
        return out;
    }
}


class Robot_Reaper : public RobotBase {

private:
    static constexpr int WEIGHT_COUNT = 19;
    enum WeightIndex {
        W_FLAME_TILE = 0,
        W_PIT_TILE,
        W_X_TILE,
        W_UNKNOWN_TILE,
        W_FLAMER_ZONE,
        W_NEAR_PIT,
        W_ENEMY_PROX,
        W_DANGERLINE,
        W_LIVE_CHEB2,
        W_LIVE_CHEB3,
        W_LIVE_CHEB4,
        W_HUNT_MANHATTAN,
        W_EXPLORE_UNKNOWN_BONUS,
        W_HUNT_ENEMY_LINE,
        W_STEP_PENALTY,
        W_BACKTRACK_PENALTY,
        W_RECENT_POS_PENALTY,
        W_EDGE_HUNT_BIAS,
        W_DAMAGE_PANIC_BOOST
    };

    static double s_weights[WEIGHT_COUNT];
    static double s_bestWeights[WEIGHT_COUNT];
    static double s_bestReward;
    static bool   s_weightsInitialized;
    static int    s_totalGames;
    static int    s_totalPitDeaths;
    static int    s_totalFlameDeaths;
    static int    s_totalOtherDeaths;
    static int    s_totalAliveEnd;
    //static constexpr int SAVE_INTERVAL = 50;

    static void loadDefaultWeights() {
        s_bestReward = 6341.8;

        const double tuned[WEIGHT_COUNT] = {
            0.498988,
            0.989355,
            1.15186,
            0.292784,
            0.606814,
            1.35822,
            0.534138,
            0.797492,
            0.995754,
            0.405609,
            2.21026,
            2.09008,
            0.350699,
            1.285592,
            0.254481,
            0.354518,
            4.54949,
            0.0,
            0.813103
        };

        for (int i = 0; i < WEIGHT_COUNT; ++i) {
            s_weights[i]     = tuned[i];
            s_bestWeights[i] = tuned[i];
        }
    }

    static void loadWeightsFromFile() {
        std::ifstream in("reaper_weights.txt");
        if (!in) {
            loadDefaultWeights();
            return;
        }

        std::vector<double> vals;
        double v;
        while (in >> v) {
            vals.push_back(v);
        }

        if ((int)vals.size() == WEIGHT_COUNT + 1) {
            s_bestReward = vals[0];
            for (int i = 0; i < WEIGHT_COUNT; ++i) {
                s_bestWeights[i] = vals[i + 1];
                s_weights[i]     = s_bestWeights[i];//need this to tsart from best
            }
        } else if ((int)vals.size() == WEIGHT_COUNT) {
            for (int i = 0; i < WEIGHT_COUNT; ++i) {
                s_bestWeights[i] = vals[i];
                s_weights[i]     = vals[i];
            }
            s_bestReward = -1e18; 
        } else {
            //fvall back to defauls
            loadDefaultWeights();
        }
    }

    static void saveBestWeightsToFile() {
        std::ofstream out("reaper_weights.txt", std::ios::trunc);
        if (!out) return;

        out << s_bestReward;
        for (int i = 0; i < WEIGHT_COUNT; ++i) {
            if (i) out << ' ';
            out << ' ' << s_bestWeights[i];
        }
        out << "\n";
    }


    static void initWeightsIfNeeded() {
        if (!s_weightsInitialized) {
            loadWeightsFromFile();
            s_weightsInitialized = true;
        }
    }

    static void mutateWeights() {
        static std::mt19937 rng{std::random_device{}()};
        std::normal_distribution<double> noise(0.0, 0.20); // 20% std

        for (int i = 0; i < WEIGHT_COUNT; ++i) {
            double base = s_weights[i];
            double factor = 1.0 + noise(rng);
            double mutated = base * factor;

            //clampt o ranges
            if (i == W_EDGE_HUNT_BIAS) {
                if (mutated < 0.0) mutated = 0.0;
                if (mutated > 5.0) mutated = 5.0;
            } else {
                if (mutated < 0.05) mutated = 0.05;
                if (mutated > 10.0) mutated = 10.0;
            }
            s_weights[i] = mutated;
        }
    }


    struct ReaperStatsRow {
        long gameId = 0;
        int  won = 0;
        int  shotsFired = 0;
        int  shotsHit = 0;
        int  kills = 0;
        int  damageDealt = 0;
        int  damageTaken = 0;
        int  roundsSurvived = 0;
        int  timesStuck = 0;
        bool valid = false;
        bool diedByRail = false;
    };

    static bool parseReaperRow(const std::string& line, ReaperStatsRow& row) {
        // gameId,name,weapon,won,
        // shotsFired,shotsHit,kills,
        // damageDealt,damageTaken,causeOfDeath,
        // roundsSurvived,deathRow,deathCol,timesStuck
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> fields;
        while (std::getline(ss, token, ',')) {
            fields.push_back(token);
        }
        if (fields.size() < 14) return false;

        ReaperStatsRow r;
        try {
            r.gameId        = std::stol(fields[0]);
            r.won           = std::stoi(fields[3]);
            r.shotsFired    = std::stoi(fields[4]);
            r.shotsHit      = std::stoi(fields[5]);
            r.kills         = std::stoi(fields[6]);
            r.damageDealt   = std::stoi(fields[7]);
            r.damageTaken   = std::stoi(fields[8]);
            r.roundsSurvived= std::stoi(fields[10]);
            r.timesStuck    = std::stoi(fields[13]);
        } catch (...) {
            return false;
        }
        r.valid = true;
        row = r;
        return true;
    }

    static bool loadLastReaperStatsForName(const std::string& name, ReaperStatsRow& outRow) {
        std::ifstream in("reaper_only_stats.csv");
        if (!in) return false;

        std::string line;
        bool firstLine = true;
        ReaperStatsRow last;
        bool found = false;

        while (std::getline(in, line)) {
            if (firstLine) {
                firstLine = false;
                continue; // skip header
            }
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string field;
            std::vector<std::string> fields;
            while (std::getline(ss, field, ',')) {
                fields.push_back(field);
            }
            if (fields.size() < 2) continue;

            if (fields[1] == name) {
                ReaperStatsRow r;
                if (parseReaperRow(line, r) && r.valid) {
                    last = r;
                    found = true;
                }
            }
        }
        if (found) {
            outRow = last;
            return true;
        }
        return false;
    }

    static void logLearningRow(const std::string& name,
                               const ReaperStatsRow& st,
                               double reward)
    {
        std::ofstream out("reaper_learning_log.csv", std::ios::app);
        if (!out) return;

        if (out.tellp() == 0) {
            out << "gameId,name,won,kills,damageDealt,damageTaken,"
            "roundsSurvived,timesStuck,reward";
            for (int i = 0; i < WEIGHT_COUNT; ++i) {
                out << ",W" << i;
            }
            out << "\n";
        }

        out << st.gameId << ","
        << name << ","
        << st.won << ","
        << st.kills << ","
        << st.damageDealt << ","
        << st.damageTaken << ","
        << st.roundsSurvived << ","
        << st.timesStuck << ","
        << reward;

        for (int i = 0; i < WEIGHT_COUNT; ++i) {
            out << "," << s_weights[i];
        }
        out << "\n";
    }

    static double computeReward(const ReaperStatsRow& st,
                                int deathBucket,
                                int internalTimesStuck)
    {
        double reward = 0.0;
        reward += st.kills       * 200.0;
        reward += st.damageDealt *  2.0;
        reward -= st.damageTaken *  1.0;
        reward += st.won         ? 400.0 : 0.0;
        reward += st.roundsSurvived * 0.8;

        int ts = std::max(st.timesStuck, internalTimesStuck);
        reward -= ts * 2.0;

        if (deathBucket == 1) reward -= 300.0;//pit
        else if (deathBucket == 2) reward -= 500.0;

        if (st.diedByRail) {
            reward -= 900.0;
            }

        return reward;
    }

    // ===============BRAIN=============================================

    std::vector<std::pair<int,int>> last_seen_this_turn;
    int locked_dir = 0;
    int sweep_idx = 1;
    int dangerLine[9] = {0};
    int closeThreatDirCount[9] = {0};
    int liveThreatDir[9] = {0};
    int turnsSinceLastSeen = 0;
    int lastHealth = -1;
    int damagePanicTurns = 0;
    int currentTurn = 0;
    int enemyThreatMemory = 0;
    std::vector<std::vector<int>> flameLastSeenTurn;

    bool memoryInitialized = false;
    std::vector<std::vector<char>> terrainMemory; // '.', 'M','P','F','X'
    std::vector<std::vector<bool>> enemyEverSeen;

    static int s_modeMoveCount[9];
    static int s_modeStayCount[9];
    static int s_dirChosenCount[9];

    static int s_escapeCauseCloseThreat;
    static int s_escapeCauseFlamer;
    static int s_escapeCauseRail;
    static int s_escapeCauseDamage;

    static int s_knownBucketCount[3];

    int modeMoveCount[9] = {0};
    int modeStayCount[9] = {0};
    int dirChosenCount[9] = {0};

    int escapeCauseCloseThreat = 0;
    int escapeCauseFlamer      = 0;
    int escapeCauseRail        = 0;
    int escapeCauseDamage      = 0;

    int knownBucketCount[3] = {0};

    bool inBounds(int r, int c) const {
        return (r >= 0 && r < m_board_row_max &&
        c >= 0 && c < m_board_col_max);
    }

    bool isFlamerZone(int r, int c) const {
        if (!memoryInitialized) return false;
        int radius = 4;
        const int FLAME_TTL = 9999999;

        for (int dr = -radius; dr <= radius; ++dr) {
            for (int dc = -radius; dc <= radius; ++dc) {
                int nr = r + dr;
                int nc = c + dc;
                if (!inBounds(nr, nc)) continue;

                if (terrainMemory[nr][nc] == 'F') {
                    int last = flameLastSeenTurn[nr][nc];
                    if (last < 0) continue;
                    if (currentTurn - last <= FLAME_TTL) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    int dirFromVec(int sgnr,int sgnc){
        if      (sgnr==-1 && sgnc==0) return 1;
        else if (sgnr==-1 && sgnc==1) return 2;
        else if (sgnr==0  && sgnc==1) return 3;
        else if (sgnr==1  && sgnc==1) return 4;
        else if (sgnr==1  && sgnc==0) return 5;
        else if (sgnr==1  && sgnc==-1)return 6;
        else if (sgnr==0  && sgnc==-1)return 7;
        else if (sgnr==-1 && sgnc==-1)return 8;
        return 0;
    }

    void ensureMemory() {
        if (memoryInitialized) return;
        if (m_board_row_max <= 0 || m_board_col_max <= 0) return;

        terrainMemory.assign(m_board_row_max,std::vector<char>(m_board_col_max, '?'));
        enemyEverSeen.assign(m_board_row_max, std::vector<bool>(m_board_col_max, false));
        flameLastSeenTurn.assign(m_board_row_max,std::vector<int>(m_board_col_max, -1));
        memoryInitialized = true;
    }

    double mapKnownFraction() const {
        if (!memoryInitialized || m_board_row_max <= 0 || m_board_col_max <= 0) {
            return 0.0;
        }
        int total = m_board_row_max * m_board_col_max;
        int known = 0;
        for (int r = 0; r < m_board_row_max; ++r) {
            for (int c = 0; c < m_board_col_max; ++c) {
                if (terrainMemory[r][c] != '?') {
                    ++known;
                }
            }
        }
        return (total > 0) ? (double)known / (double)total : 0.0;
    }

    int getMaxStepForKnowledge() {
        int base = get_move_speed();
        double known = mapKnownFraction();

        if (known < 0.10) {
            return std::min(base, 2);
        } else if (known < 0.35) {
            return std::min(base, 3);
        } else if (known < 0.70) {
            return std::min(base, 4);
        } else {
            return base;
        }
    }

    int scoreTile(int r, int c) const {
        if (!inBounds(r, c)) return 1'000'000;

        char t = terrainMemory[r][c];

        if (t == 'M' || t == 'X' || t == 'P' || t == 'F') {
            return 900'000;
        }

        int score = 0;

        if (t == 'X') {
            score += int(12000 * s_weights[W_X_TILE]);
        }

        if (t == '?') {
            score += int(2000 * s_weights[W_UNKNOWN_TILE]);
        }

        if (t == 'F') {
            score += int(100'000 * s_weights[W_FLAME_TILE]);
        }

        if (t == 'P') {
            score += int(100'000 * s_weights[W_PIT_TILE]);
        }

        if (isFlamerZone(r, c)) {
            score += int(25'000 * s_weights[W_FLAMER_ZONE]);
        }

        bool nearPit = false;
        for (int dr = -2; dr <= 2 && !nearPit; ++dr) {
            for (int dc = -2; dc <= 2; ++dc) {
                int rr = r + dr, cc = c + dc;
                if (!inBounds(rr, cc)) continue;
                if (terrainMemory[rr][cc] == 'P') {
                    nearPit = true;
                    break;
                }
            }
        }
        if (nearPit) score += int(5'000 * s_weights[W_NEAR_PIT]);

        int minEnemyDist = 1'000;
        for (int rr = 0; rr < m_board_row_max; ++rr) {
            for (int cc = 0; cc < m_board_col_max; ++cc) {
                if (!enemyEverSeen[rr][cc]) continue;
                int d = std::abs(rr - r) + std::abs(cc - c);
                if (d < minEnemyDist) minEnemyDist = d;
            }
        }
        if (minEnemyDist < 1'000) {
            int d = std::min(minEnemyDist, 20);
            score += int((20 - d) * 80 * s_weights[W_ENEMY_PROX]);
        }

        //e-b
        if (m_board_row_max > 0 && m_board_col_max > 0) {
            int rmax = m_board_row_max - 1;
            int cmax = m_board_col_max - 1;
            int distEdge = std::min(std::min(r, rmax - r),
                                    std::min(c, cmax - c));
            score += int(distEdge * 100 * s_weights[W_EDGE_HUNT_BIAS]);
        }

        return score;
    }

    int scorePositionWithLive(int r, int c) {
        for (auto [er, ec] : last_seen_this_turn) {
            if (er == r && ec == c) {
                return 1'000'000;
            }
        }
        int s = scoreTile(r, c);

        if (!last_seen_this_turn.empty()) {
            int minLive = 1000;
            for (auto [er, ec] : last_seen_this_turn) {
                int dd = std::max(std::abs(er - r), std::abs(ec - c));
                if (dd < minLive) minLive = dd;
            }
            if (minLive <= 2)      s += int(120'000 * s_weights[W_LIVE_CHEB2]);
            else if (minLive <= 3) s += int(60'000  * s_weights[W_LIVE_CHEB3]);
            else if (minLive <= 4) s += int(15'000  * s_weights[W_LIVE_CHEB4]);
        }
        return s;
    }

    int scorePathRisk(int cr, int cc, int dir, int dist) {
        static const std::pair<int,int> dirs[9] = {
            {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        };

        if (dir == 0 || dist <= 0) {
            return scorePositionWithLive(cr, cc);
        }

        int r = cr;
        int c = cc;
        int worstStep = 0;

        for (int step = 1; step <= dist; ++step) {
            r += dirs[dir].first;
            c += dirs[dir].second;

            if (!inBounds(r, c)) {
                return 1'000'000;
            }

            int s = scorePositionWithLive(r, c);

            if (s >= 900'000) {
                return 1'000'000;
            }

            if (s > worstStep) worstStep = s;
        }

        worstStep += int(dangerLine[dir] * 2500 * s_weights[W_DANGERLINE]);
        return worstStep;
    }

    int scoreEscapeDir(int cr, int cc, int d) {
        static const std::pair<int,int> dirs[9] = {
            {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        };

        int nr = cr + dirs[d].first;
        int nc = cc + dirs[d].second;
        int s  = scoreTile(nr, nc);
        if (d != 0) {
            s += int(dangerLine[d] * 2500 * s_weights[W_DANGERLINE]);
        }

        if (!last_seen_this_turn.empty()) {
            int minLive = 1000;
            for (auto [er, ec] : last_seen_this_turn) {
                int dd = std::max(std::abs(er - nr), std::abs(ec - nc));
                if (dd < minLive) minLive = dd;
            }
            if (minLive <= 2)      s += int(100'000 * s_weights[W_LIVE_CHEB2]);
            else if (minLive <= 3) s += int(60'000  * s_weights[W_LIVE_CHEB3]);
            else if (minLive <= 4) s += int(15'000  * s_weights[W_LIVE_CHEB4]);
        }

        return s;
    }

    void chooseHuntCorner(int& targetRow, int& targetCol) const {
        targetRow = -1;
        targetCol = -1;
        if (!memoryInitialized || m_board_row_max <= 0 || m_board_col_max <= 0) return;

        int rmax = m_board_row_max - 1;
        int cmax = m_board_col_max - 1;
        int corners[4][2] = {
            {0,      0},
            {0,      cmax},
            {rmax,   0},
            {rmax,   cmax}
        };

        int bestScore = 1'000'000;

        for (int i = 0; i < 4; ++i) {
            int r = corners[i][0];
            int c = corners[i][1];
            if (!inBounds(r, c)) continue;

            int base = scoreTile(r, c);
            if (base >= 900'000) continue;

            int coverage = 0;
            for (int rr = 0; rr < m_board_row_max; ++rr) {
                for (int cc = 0; cc < m_board_col_max; ++cc) {
                    if (!enemyEverSeen[rr][cc]) continue;
                    if (rr == r || cc == c || std::abs(rr - r) == std::abs(cc - c)) {
                        ++coverage;
                    }
                }
            }

            int cornerScore = base - coverage * 200;
            if (cornerScore < bestScore) {
                bestScore = cornerScore;
                targetRow = r;
                targetCol = c;
            }
        }
    }

    void chooseRepositionCorner(int cr, int cc,
                                int& targetRow, int& targetCol) const {
                                    targetRow = -1;
                                    targetCol = -1;
                                    if (!memoryInitialized || m_board_row_max <= 0 || m_board_col_max <= 0) return;

                                    int rmax = m_board_row_max - 1;
                                    int cmax = m_board_col_max - 1;
                                    int corners[4][2] = {
                                        {0,      0},
                                        {0,      cmax},
                                        {rmax,   0},
                                        {rmax,   cmax}
                                    };

                                    int bestScore = 1'000'000;

                                    for (int i = 0; i < 4; ++i) {
                                        int r = corners[i][0];
                                        int c = corners[i][1];
                                        if (!inBounds(r, c)) continue;

                                        int base = scoreTile(r, c);
                                        if (base >= 900'000) continue;

                                        int dist = std::abs(r - cr) + std::abs(c - cc);
                                        if (dist <= 2) continue;

                                        int score = base - dist * 40;
                                        if (score < bestScore) {
                                            bestScore = score;
                                            targetRow = r;
                                            targetCol = c;
                                        }
                                    }
                                }

                                void chooseBestFreeMove(int cr, int cc,
                                                        int& outDir, int& outDist,
                                                        bool huntMode,
                                                        int huntTargetRow,
                                                        int huntTargetCol,
                                                        bool exploreMode)
                                {
                                    static const std::pair<int,int> dirs[9] = {
                                        {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
                                    };

                                    int maxStep = getMaxStepForKnowledge();

                                    int bestScore = 1'000'000;
                                    outDir  = 0;
                                    outDist = 0;

                                    int stayScore = scorePositionWithLive(cr, cc);
                                    bestScore = stayScore;
                                    outDir  = 0;
                                    outDist = 0;

                                    for (int d = 1; d <= 8; ++d) {
                                        for (int k = 1; k <= maxStep; ++k) {
                                            int fr = cr + dirs[d].first * k;
                                            int fc = cc + dirs[d].second * k;

                                            if (!inBounds(fr, fc)) break;

                                            int risk = scorePathRisk(cr, cc, d, k);
                                            if (risk >= 1'000'000) {
                                                break;
                                            }

                                            int heuristic = 0;

                                            if (huntMode && huntTargetRow >= 0 && huntTargetCol >= 0) {
                                                int manhattan = std::abs(fr - huntTargetRow) + std::abs(fc - huntTargetCol);
                                                heuristic += int(manhattan * 30 * s_weights[W_HUNT_MANHATTAN]);
                                            }

                                            if (exploreMode) {
                                                char t = terrainMemory[fr][fc];
                                                if (t == '?') {
                                                    heuristic -= int(2500 * s_weights[W_EXPLORE_UNKNOWN_BONUS]);
                                                }
                                            }

                                            if (huntMode) {
                                                int minEver = 1000;
                                                for (int rr = 0; rr < m_board_row_max; ++rr) {
                                                    for (int cc2 = 0; cc2 < m_board_col_max; ++cc2) {
                                                        if (!enemyEverSeen[rr][cc2]) continue;
                                                        int dEnemy = std::abs(rr - fr) + std::abs(cc2 - fc);
                                                        if (dEnemy < minEver) minEver = dEnemy;
                                                    }
                                                }
                                                if (minEver < 1000) {
                                                    int dE = std::min(minEver, 20);
                                                    heuristic -= int((20 - dE) * 300 * s_weights[W_HUNT_ENEMY_LINE]);
                                                }
                                            }

                                            int total = risk + heuristic - int(k * 5 * s_weights[W_STEP_PENALTY]);

                                            if (fr == last_r && fc == last_c) {
                                                total += int(5000 * s_weights[W_BACKTRACK_PENALTY]);
                                            }

                                            for (const auto& p : recentPositions) {
                                                if (p.first == fr && p.second == fc) {
                                                    total += int(80'000 * s_weights[W_RECENT_POS_PENALTY]);
                                                    break;
                                                }
                                            }

                                            if (total < bestScore ||
                                                (total == bestScore && k > outDist)) {
                                                bestScore = total;
                                            outDir  = d;
                                            outDist = k;
                                                }
                                        }
                                    }
                                }

                                void recordCloseThreatChoice(int dir) {
                                    if (dir < 0 || dir > 8) return;
                                    ++closeThreatDirCount[dir];
                                }

                                enum DeathBucket {
                                    DB_ALIVE = 0,
                                    DB_PIT,
                                    DB_FLAME,
                                    DB_OTHER
                                };

                                DeathBucket classifyDeathBucket() {
                                    int cr, cc;
                                    get_current_location(cr, cc);

                                    if (!memoryInitialized || !inBounds(cr, cc)) {
                                        return DB_OTHER;
                                    }

                                    char t = terrainMemory[cr][cc];

                                    if (t == 'P') return DB_PIT;
                                    if (t == 'F' || isFlamerZone(cr, cc)) return DB_FLAME;

                                    return DB_OTHER;
                                }


                                enum MoveMode {
                                    MODE_ESCAPE = 0,
                                    MODE_EXPLORE,
                                    MODE_HUNT,
                                    MODE_REPOSITION,
                                    MODE_DRIFT,
                                    MODE_COUNT
                                };

                                void logGameSummary() {
                                    DeathBucket bucket = classifyDeathBucket();

                                    ++s_totalGames;
                                    switch (bucket) {
                                        case DB_PIT:   ++s_totalPitDeaths;   break;
                                        case DB_FLAME: ++s_totalFlameDeaths; break;
                                        case DB_OTHER: ++s_totalOtherDeaths; break;
                                        case DB_ALIVE: ++s_totalAliveEnd;    break;
                                    }

                                    reaper_log()
                                    << "[SWEEPER-BRAIN-SUMMARY] " << m_name
                                    << " game=" << s_totalGames
                                    << " movesByMode(ESC,EXP,HUNT,REPOS,DRIFT)="
                                    << modeMoveCount[MODE_ESCAPE]      << ","
                                    << modeMoveCount[MODE_EXPLORE]     << ","
                                    << modeMoveCount[MODE_HUNT]        << ","
                                    << modeMoveCount[MODE_REPOSITION]  << ","
                                    << modeMoveCount[MODE_DRIFT]
                                    << " staysByMode(ESC,EXP,HUNT,REPOS,DRIFT)="
                                    << modeStayCount[MODE_ESCAPE]      << ","
                                    << modeStayCount[MODE_EXPLORE]     << ","
                                    << modeStayCount[MODE_HUNT]        << ","
                                    << modeStayCount[MODE_REPOSITION]  << ","
                                    << modeStayCount[MODE_DRIFT]
                                    << " dirCounts(0..8)="
                                    << dirChosenCount[0] << ","
                                    << dirChosenCount[1] << ","
                                    << dirChosenCount[2] << ","
                                    << dirChosenCount[3] << ","
                                    << dirChosenCount[4] << ","
                                    << dirChosenCount[5] << ","
                                    << dirChosenCount[6] << ","
                                    << dirChosenCount[7] << ","
                                    << dirChosenCount[8]
                                    << " escapeCauses(close,flame,rail,damage)="
                                    << escapeCauseCloseThreat << ","
                                    << escapeCauseFlamer      << ","
                                    << escapeCauseRail        << ","
                                    << escapeCauseDamage
                                    << " knownBuckets(low,mid,high)="
                                    << knownBucketCount[0] << ","
                                    << knownBucketCount[1] << ","
                                    << knownBucketCount[2]
                                    << " timesStuck=" << timesStuck
                                    << " | GLOBAL_movesByMode(ESC,EXP,HUNT,REPOS,DRIFT)="
                                    << s_modeMoveCount[MODE_ESCAPE]      << ","
                                    << s_modeMoveCount[MODE_EXPLORE]     << ","
                                    << s_modeMoveCount[MODE_HUNT]        << ","
                                    << s_modeMoveCount[MODE_REPOSITION]  << ","
                                    << s_modeMoveCount[MODE_DRIFT]
                                    << " GLOBAL_knownBuckets(low,mid,high)="
                                    << s_knownBucketCount[0] << ","
                                    << s_knownBucketCount[1] << ","
                                    << s_knownBucketCount[2]
                                    << "\n";
                                }

public:
    Robot_Reaper(): RobotBase(/*move*/4, /*armor*/3, /*weapon*/railgun) {
        initWeightsIfNeeded();
    }

    int last_r=-1;
    int last_c=-1;
    int last_move_dir=0;
    int last_move_dist=0;

    std::vector<std::pair<int,int>> recentPositions;

    int timesStuck = 0;
    //int get_times_stuck() const override { return timesStuck; } //FOR DEBUGGING //OBSOLETE


    ~Robot_Reaper() override {
        logGameSummary();

        ReaperStatsRow st;
        if (loadLastReaperStatsForName(m_name, st) && st.valid) {
            DeathBucket b = classifyDeathBucket();
            int bucketInt = 3;
            if (b == DB_PIT)      bucketInt = 1;
            else if (b == DB_FLAME) bucketInt = 2;
            else if (b == DB_ALIVE) bucketInt = 0;

            double reward = computeReward(st, bucketInt, timesStuck);

            logLearningRow(m_name, st, reward);

            if (reward > s_bestReward) {
                s_bestReward = reward;
                for (int i = 0; i < WEIGHT_COUNT; ++i) {
                    s_bestWeights[i] = s_weights[i];
                }
                saveBestWeightsToFile();
            } else {
                for (int i = 0; i < WEIGHT_COUNT; ++i) {
                    s_weights[i] = s_bestWeights[i];
                }
            }
            mutateWeights();
        }
    }



    void get_radar_direction(int& radar_direction) override {
        int cr,cc;
        get_current_location(cr,cc);

        if (locked_dir != 0) {
            radar_direction = locked_dir;
        } else if (last_move_dir != 0) {
            radar_direction = last_move_dir;
        } else {
            if (sweep_idx < 1 || sweep_idx > 8) sweep_idx = 1;
            radar_direction = sweep_idx;
            sweep_idx = (sweep_idx % 8) + 1;
        }
    }

    void process_radar_results(const std::vector<RadarObj>& radar_results) override {
        ensureMemory();
        last_seen_this_turn.clear();

        for (int d = 1; d <= 8; ++d) {
            dangerLine[d]    = std::max(0, dangerLine[d] - 1);
            liveThreatDir[d] = 0;
        }

        int cr, cc;
        get_current_location(cr, cc);

        if (memoryInitialized && inBounds(cr, cc)) {
            char &t = terrainMemory[cr][cc];
            if (t == '?') t = '.';
        }

        bool have_lock = false;

        auto isEnemyBot = [](char t) {
            return (t == 'R' || /*t == 'F' ||*/ t == 'H' || t == 'G');
        };
        auto isRailgunBot = [](char t) {
            return (t == 'R');
        };

        for (const auto& obj : radar_results) {
            if (!memoryInitialized) continue;
            int rr  = obj.m_row;
            int cc2 = obj.m_col;
            if (!inBounds(rr, cc2)) continue;

            char ot = obj.m_type;

            if (ot == 'M' || ot == 'P' || ot == 'F' || ot == 'X') {
                terrainMemory[rr][cc2] = ot;
                if (ot == 'F') {
                    flameLastSeenTurn[rr][cc2] = currentTurn;
                }
            } else if (ot == '.') {
                if (terrainMemory[rr][cc2] == '?') {
                    terrainMemory[rr][cc2] = '.';
                }
            }

            if (!isEnemyBot(ot)) {
                continue;
            }

            if (terrainMemory[rr][cc2] == '?') {
                terrainMemory[rr][cc2] = '.';
            }

            int dr = rr - cr;
            int dc = cc2 - cc;

            if ((dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc))) {
                int sgnr = (dr > 0) - (dr < 0);
                int sgnc = (dc > 0) - (dc < 0);
                int d = dirFromVec(sgnr, sgnc);
                if (d != 0) {
                    dangerLine[d] += 1;

                    if (isRailgunBot(ot)) {
                        int cheb = std::max(std::abs(dr), std::abs(dc));
                        if (cheb <= 6) {
                            liveThreatDir[d] = 1;
                        }
                    }
                }
            }

            last_seen_this_turn.push_back({rr, cc2});
            enemyEverSeen[rr][cc2] = true;

            bool aligned = (dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc));
            if (aligned && !have_lock) {
                int sgnr = (dr > 0) - (dr < 0);
                int sgnc = (dc > 0) - (dc < 0);
                locked_dir = dirFromVec(sgnr, sgnc);
                have_lock  = true;
            }
        }

        if (last_seen_this_turn.empty()) {
            ++turnsSinceLastSeen;
        } else {
            turnsSinceLastSeen = 0;
        }

        if (!have_lock) {
            locked_dir = 0;
        }
    }


    bool get_shot_location(int& shot_row, int& shot_col) override {
        if (last_seen_this_turn.empty()) {
            return false;
        }

        int cr, cc;
        get_current_location(cr, cc);

        static const std::pair<int,int> dirs[9] = {
            {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        };

        auto alignedWith = [&](int r, int c, int dir) -> bool {
            int dr = r - cr;
            int dc = c - cc;
            if (!((dr == 0) || (dc == 0) || (std::abs(dr) == std::abs(dc)))) return false;
            if (dir == 0) return true;
            int sgnr = (dr > 0) - (dr < 0);
            int sgnc = (dc > 0) - (dc < 0);
            return dirs[dir].first == sgnr && dirs[dir].second == sgnc;
        };

        std::pair<int,int> best = {-1, -1};
        int bestd = std::numeric_limits<int>::max();

        if (locked_dir != 0) {
            for (auto [r, c] : last_seen_this_turn) {
                if (!alignedWith(r, c, locked_dir)) continue;
                int d = std::max(std::abs(r - cr), std::abs(c - cc));
                if (d < bestd) {
                    bestd = d;
                    best  = {r, c};
                }
            }
        }

        if (bestd != std::numeric_limits<int>::max() && bestd >= 1) {
            shot_row = best.first;
            shot_col = best.second;
            return true;
        }

        best = {-1, -1};
        bestd = -1;
        for (auto [r, c] : last_seen_this_turn) {
            if (!alignedWith(r, c, 0)) continue;
            int d = std::max(std::abs(r - cr), std::abs(c - cc));
            if (d > bestd) {
                bestd = d;
                best  = {r, c};
            }
        }

        if (bestd >= 1) {
            shot_row = best.first;
            shot_col = best.second;
            return true;
        }

        return false;
    }

    void get_move_direction(int& direction, int& distance) override {
        currentTurn++;
        int cr, cc;
        get_current_location(cr, cc);
        ensureMemory();

        if (memoryInitialized) {
            char &t = terrainMemory[cr][cc];
            if (t == '?') {
                t = '.';
            }
        }

        int currentHealth = get_health();
        if (lastHealth < 0) {
            lastHealth = currentHealth;
        }

        bool tookDamage = (currentHealth < lastHealth);
        if (tookDamage) {
            damagePanicTurns = int(10 * s_weights[W_DAMAGE_PANIC_BOOST]);

            if (locked_dir != 0) {
                dangerLine[locked_dir] += 8;

                int opposite = (locked_dir + 4 - 1) % 8 + 1;
                dangerLine[opposite] += 3;
            }
        } else if (damagePanicTurns > 0) {
            --damagePanicTurns;
        }
        lastHealth = currentHealth;
        bool damageThreat = (damagePanicTurns > 0);

        recentPositions.push_back({cr, cc});
        if (recentPositions.size() > 10) {
            recentPositions.erase(recentPositions.begin());
        }

        bool stuck = (last_move_dir != 0 && cr == last_r && cc == last_c);
        if (stuck) {
            ++timesStuck;
        }

        if (!last_seen_this_turn.empty()) {
            enemyThreatMemory = 3; //3turn memory
        } else if (enemyThreatMemory > 0) {
            --enemyThreatMemory;
        }

        bool closeThreat = false;
        if (enemyThreatMemory > 0) {
            for (auto [r, c] : last_seen_this_turn) {
                if (std::abs(r - cr) <= 6 && std::abs(c - cc) <= 6) {
                    closeThreat = true;
                    break;
                }
            }
        }

        //bool hereIsFlamerZone = isFlamerZone(cr, cc);
        bool currentRailThreat = (locked_dir != 0 && liveThreatDir[locked_dir] > 0);
        bool escapeMode = (closeThreat || currentRailThreat || damageThreat);


        static const std::pair<int,int> dirs[9] = {
            {0,0},{-1,0},{-1,1},{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1}
        };

        auto commitMove = [&](int d, int dist) {
            direction = d;
            distance  = (d == 0) ? 0 : dist;
            last_r        = cr;
            last_c        = cc;
            last_move_dir = direction;
            last_move_dist= distance;
        };

        if (escapeMode) {
            int bestDir   = 0;
            int bestDist  = 0;
            int bestScore = 1'000'000;

            int maxStep = std::min(get_move_speed(), 3);

            for (int d = 0; d <= 8; ++d) {
                int startK = (d == 0 ? 0 : 1);
                for (int k = startK; k <= maxStep; ++k) {
                    int fr = cr + dirs[d].first  * k;
                    int fc = cc + dirs[d].second * k;

                    if (!inBounds(fr, fc)) break;

                    int baseRisk = scorePathRisk(cr, cc, d, k);
                    if (baseRisk >= 1'000'000) {
                        break;
                    }

                    int extra = 0;

                    if (!last_seen_this_turn.empty()) {
                        int bestLiveDistNow = 1000;
                        int bestLiveDistNew = 1000;
                        bool hasAlignedNow  = false;
                        bool hasAlignedNew  = false;

                        for (auto [er, ec] : last_seen_this_turn) {
                            int drNow = er - cr;
                            int dcNow = ec - cc;
                            int drNew = er - fr;
                            int dcNew = ec - fc;
                            int chebNow = std::max(std::abs(drNow), std::abs(dcNow));
                            int chebNew = std::max(std::abs(drNew), std::abs(dcNew));

                            bool alignedNow = (drNow == 0) || (dcNow == 0) || (std::abs(drNow) == std::abs(dcNow));
                            bool alignedNew = (drNew == 0) || (dcNew == 0) || (std::abs(drNew) == std::abs(dcNew));

                            if (chebNow < bestLiveDistNow) bestLiveDistNow = chebNow;
                            if (chebNew < bestLiveDistNew) bestLiveDistNew = chebNew;

                            hasAlignedNow = hasAlignedNow || alignedNow;
                            hasAlignedNew = hasAlignedNew || alignedNew;
                        }

                        //further from enemy bias
                        if (bestLiveDistNew > bestLiveDistNow) {
                            extra -= 500 * (bestLiveDistNew - bestLiveDistNow);
                        }

                        //expensive to break LOS
                        if (hasAlignedNow && !hasAlignedNew) {
                            extra += 4000;
                        }
                    }

                    if (last_move_dir != 0 && d != 0) {
                        int opposite = (last_move_dir + 4 - 1) % 8 + 1;
                        if (d == opposite) {
                            extra += 2000;
                        }
                    }

                    if (fr == last_r && fc == last_c) {
                        extra += 3000;
                    }

                    int total = baseRisk + extra;

                    if (total < bestScore ||
                        (total == bestScore && k > bestDist)) {
                        bestScore = total;
                    bestDir   = d;
                    bestDist  = k;
                        }
                }
            }

            if (closeThreat) {
                recordCloseThreatChoice(bestDir);
            }

            MoveMode mode = MODE_ESCAPE;

            double knownFrac = mapKnownFraction();
            int kb = (knownFrac < 0.33) ? 0 : (knownFrac < 0.66 ? 1 : 2);
            ++knownBucketCount[kb];
            ++s_knownBucketCount[kb];

            if (closeThreat)      { ++escapeCauseCloseThreat; ++s_escapeCauseCloseThreat; }
            //if (hereIsFlamerZone) { ++escapeCauseFlamer;      ++s_escapeCauseFlamer;      }
            if (currentRailThreat){ ++escapeCauseRail;        ++s_escapeCauseRail;        }
            if (damageThreat)     { ++escapeCauseDamage;      ++s_escapeCauseDamage;      }

            bool stayed = (bestDir == 0 || bestDist == 0);
            if (stayed) {
                ++modeStayCount[mode];
                ++s_modeStayCount[mode];
            } else {
                ++modeMoveCount[mode];
                ++s_modeMoveCount[mode];
                ++dirChosenCount[bestDir];
                ++s_dirChosenCount[bestDir];
            }

            commitMove(bestDir, bestDist);
            return;
        }

        double knownFrac = mapKnownFraction();
        bool   exploreMode    = (knownFrac < 0.3);
        bool   huntMode       = false;
        bool   repositionMode = false;

        auto bucketKnownFrac = [&](double k) {
            if (k < 0.10) return 0;
            if (k < 0.25) return 1;
            return 2;
        };

        if (turnsSinceLastSeen > 5 && knownFrac >= 0.3) {
            huntMode = true;
        }

        int huntTargetRow = -1;
        int huntTargetCol = -1;
        if (huntMode) {
            chooseHuntCorner(huntTargetRow, huntTargetCol);
            if (huntTargetRow < 0 || huntTargetCol < 0) {
                huntMode = false;
            }
            int distToCorner = std::abs(cr - huntTargetRow) + std::abs(cc - huntTargetCol);
            if (distToCorner <= 2 && turnsSinceLastSeen > 20) {
                huntMode = false;
            }
        }

        const int REPOSITION_TURNS = 70;
        if (!escapeMode && turnsSinceLastSeen >= REPOSITION_TURNS) {
            int repRow = -1, repCol = -1;
            chooseRepositionCorner(cr, cc, repRow, repCol);
            if (repRow >= 0 && repCol >= 0) {
                huntMode       = true;
                repositionMode = true;
                huntTargetRow  = repRow;
                huntTargetCol  = repCol;
            }
        }

        MoveMode mode;

        if (escapeMode) {
            mode = MODE_ESCAPE;
        } else if (repositionMode) {
            mode = MODE_REPOSITION;
        } else if (exploreMode) {
            mode = MODE_EXPLORE;
        } else if (huntMode) {
            mode = MODE_HUNT;
        } else {
            mode = MODE_DRIFT;
        }

        int kb = bucketKnownFrac(knownFrac);
        ++knownBucketCount[kb];
        ++s_knownBucketCount[kb];

        int bestDir  = 0;
        int bestDist = 0;

        chooseBestFreeMove(cr, cc, bestDir, bestDist,
                           huntMode, huntTargetRow, huntTargetCol,
                           exploreMode);

        bool stayed = (bestDir == 0 || bestDist == 0);
        if (stayed) {
            ++modeStayCount[mode];
            ++s_modeStayCount[mode];
        } else {
            ++modeMoveCount[mode];
            ++s_modeMoveCount[mode];
            ++dirChosenCount[bestDir];
            ++s_dirChosenCount[bestDir];
        }

        commitMove(bestDir, bestDist);
    }

};


double Robot_Reaper::s_weights[Robot_Reaper::WEIGHT_COUNT];
double Robot_Reaper::s_bestWeights[Robot_Reaper::WEIGHT_COUNT];
double Robot_Reaper::s_bestReward = 6341.8;
bool   Robot_Reaper::s_weightsInitialized = false;

int Robot_Reaper::s_totalGames       = 0;
int Robot_Reaper::s_totalPitDeaths   = 0;
int Robot_Reaper::s_totalFlameDeaths = 0;
int Robot_Reaper::s_totalOtherDeaths = 0;
int Robot_Reaper::s_totalAliveEnd    = 0;

int Robot_Reaper::s_modeMoveCount[9] = {0};
int Robot_Reaper::s_modeStayCount[9] = {0};
int Robot_Reaper::s_dirChosenCount[9] = {0};

int Robot_Reaper::s_escapeCauseCloseThreat = 0;
int Robot_Reaper::s_escapeCauseFlamer      = 0;
int Robot_Reaper::s_escapeCauseRail        = 0;
int Robot_Reaper::s_escapeCauseDamage      = 0;

int Robot_Reaper::s_knownBucketCount[3] = {0};

extern "C" RobotBase* create_robot(){return new Robot_Reaper();}
