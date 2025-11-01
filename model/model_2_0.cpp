#include <iostream>
#include <algorithm>
#include <string>
#include <tuple>
#include <random>
#include <chrono>
#include <cmath>

// 随机数生成器
std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
using PDD = std::pair<double, double>;

struct Player {
    std::string name;
    char id;
    double cap;
    double ser;
    double game_point;
};

struct PointInfo {
    bool win;
    int game_idx;
    double alpha;
    double mom_avg1;
    double mom_avg2;
};

std::vector<Player> initializePlayers() {
    return {
        {"Harimoto", 'H', 0.45, 0.8, 0.5},
        {"Fan Zhendong", 'F', 0.55, 0.9, 0.6}
    };
}
std::vector<Player> players = initializePlayers();
Player& playerA = players[0];
Player& playerB = players[1];

std::vector<PointInfo> playerA_points;
std::vector<PointInfo> playerB_points;

const int WINDOW_SIZE = 4;
const int SIMULATIONS = 10000;
const std::vector<std::string> get_game_score_seqs() {
    return {
        "HFHHHHHHHHHFH",        // 第1局
        "HHFFHFFFHHFHHHFFHFHH", // 第2局
        "FFFFFFHHFHFFFHF",      // 第3局
        "HFHFFHHHFFHHFFFFFF",   // 第4局
        "HHHHFFHHHFHHFHH",      // 第5局
        "FHFFHFFFHHFHHHFFFF",   // 第6局
        "FFHHHHFFFFHHHFFFFF"    // 第7局
    };
}

/**
 * @brief 计算乒乓球比赛的局进程
 * @param scrA 选手A当前得分
 * @param scrB 选手B当前得分
 * @return double 进程值（0.0~1.0），越接近1表示越接近局末
 */
double calc_game_progress(int scrA, int scrB) {
    int maxScr = std::max(scrA, scrB);
    int minScr = std::min(scrA, scrB);
    int scoreDiff = maxScr - minScr;

    if (maxScr < 10) {
        return maxScr / 10.0;
    }

    if (scoreDiff >= 2) {
        return 1.0;
    }

    int extraScore = maxScr - 10;
    double baseProgress = 0.92 + (1 - scoreDiff) * 0.04;
    double extraProgress = extraScore * 0.02;
    double finalProgress = baseProgress + extraProgress;

    return std::clamp(finalProgress, 0.92, 0.99);
}

int next_server(int game_idx, int scrA, int scrB) {
    int first_server = (game_idx % 2 == 1) ? 1 : 2;
    int total_points = scrA + scrB;

    bool deuce = (scrA >= 10 && scrB >= 10);

    if (!deuce) {
        int round = total_points / 2;
        return (round % 2 == 0) ? first_server : 3 - first_server;
    } else {
        int diff = total_points - 20;
        return (diff % 2 == 0) ? first_server : 3 - first_server;
    }
}

int game_point(int scrA, int scrB) {
    // 11分制，至少领先2分才胜
    if (scrA >= 10 && scrA > scrB) {
        // A领先，若A再得1分即可赢
        if ((scrA >= 10 && scrB >= 10 && scrA - scrB == 1) || (scrA == 10 && scrB <= 9))
            return 1;
    }
    if (scrB >= 10 && scrB > scrA) {
        // B领先，若B再得1分即可赢
        if ((scrB >= 10 && scrA >= 10 && scrB - scrA == 1) || (scrB == 10 && scrA <= 9))
            return 2;
    }
    return 0;
}

int is_game_over(int scrA, int scrB) {
    int maxScore = std::max(scrA, scrB);
    int minScore = std::min(scrA, scrB);
    if (maxScore >= 11 && maxScore - minScore >= 2) {
        return (scrA > scrB) ? 1 : 2;
    }
    return 0;
}

double calc_alpha(int scrA, int scrB) {
    return 0.5 * calc_game_progress(scrA, scrB) + 0.5;
}

void calc_momentum(std::vector<PointInfo>& points) {
    int last_idx = points.size() - 1;
    if (points.size() < WINDOW_SIZE) {
        points[last_idx].mom_avg1 = points[last_idx].mom_avg2 = 0.5;
        return ;
    }

    double avg1 = 0, numerator = 0, denominator = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        double weight = 1.0;
        PointInfo& temp = points[last_idx - i];

        if (temp.game_idx != points[last_idx].game_idx) weight = 0.3;

        numerator += temp.alpha * temp.win * weight;
        denominator += temp.alpha * weight;
    }
    avg1 = numerator / denominator;
    points[last_idx].mom_avg1 = avg1;

    double avg2 = 0;
    numerator = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        PointInfo& temp = points[last_idx - i];
        numerator += temp.mom_avg1;
    }
    avg2 = numerator / (1.0 * WINDOW_SIZE);
    points[last_idx].mom_avg2 = avg2;
}

void read_PointInfo(char status, int game_idx, int& scrA, int& scrB) {
    if (status == playerA.id) {
        scrA++;
        double alpha = calc_alpha(scrA, scrB);
        playerA_points.emplace_back(1, game_idx, alpha, 0, 0);
        playerB_points.emplace_back(0, game_idx, alpha, 0, 0);
    } else {
        scrB++;
        double alpha = calc_alpha(scrA, scrB);
        playerA_points.emplace_back(0, game_idx, alpha, 0, 0);
        playerB_points.emplace_back(1, game_idx, alpha, 0, 0);
    }
    calc_momentum(playerA_points);
    calc_momentum(playerB_points);
}


double calc_elo(char id, int game_idx, int scrA, int scrB, Player& player, std::vector<PointInfo>& points) {
    int _player = (id == playerA.id) ? 1 : 2;
    bool is_serve = (_player == next_server(game_idx, scrA, scrB));
    bool is_game_point = game_point(scrA, scrB);

    return 0.5 * player.cap + 0.1 * player.ser * is_serve + 0.2 * (is_game_point ? player.game_point : 1) + 0.2 * points.back().mom_avg2;
}

double monte_carlo_simulation(int current_game,
                              int scrA, int scrB,
                              const std::vector<PointInfo>& historyA,
                              const std::vector<PointInfo>& historyB) {

    int a_wins = 0;

    for (int sim = 0; sim < SIMULATIONS; sim++) {
        // 复制当前状态
        int sim_game = current_game;
        int sim_scrA = scrA;
        int sim_scrB = scrB;

        std::vector<PointInfo> sim_historyA = historyA;
        std::vector<PointInfo> sim_historyB = historyB;

        int game_over = 0;

        while (!game_over) {
            double eloA = 0.0;
            double eloB = 0.0;

            if (!sim_historyA.empty()) {
                eloA = calc_elo(playerA.id, sim_game, sim_scrA, sim_scrB, playerA, sim_historyA);
                eloB = calc_elo(playerB.id, sim_game, sim_scrA, sim_scrB, playerB, sim_historyB);
            } else {
                eloA = 0.5;
                eloB = 0.5;
            }

            double winA = eloA / (eloA + eloB);
            double winB = eloB / (eloA + eloB);

            std::uniform_real_distribution<double> dis(0.0, 1.0);
            double rand_val = dis(gen);

            bool a_won_point = false;
            if (rand_val < winA) {
                a_won_point = true;
                sim_scrA++;
            } else {
                sim_scrB++;
            }

            double alpha = calc_alpha(sim_scrA, sim_scrB);
            sim_historyA.emplace_back(a_won_point, sim_game, alpha, 0, 0);
            sim_historyB.emplace_back(!a_won_point, sim_game, alpha, 0, 0);

            calc_momentum(sim_historyA);
            calc_momentum(sim_historyB);

            game_over = is_game_over(sim_scrA, sim_scrB);
            if (game_over) a_wins += (game_over == 1);
        }
    }

    return 1. * a_wins / SIMULATIONS;
}

int main() {
    std::vector<std::string> game_seqs = get_game_score_seqs();
    int total_points = 0;

    for (int game_idx = 0; game_idx < game_seqs.size(); game_idx++) {
        const std::string& seq = game_seqs[game_idx];
        int scrA = 0, scrB = 0;

        for (int point_idx = 0; point_idx < seq.size(); point_idx++) {
            read_PointInfo(seq[point_idx], game_idx, scrA, scrB);

            if (total_points < 4) {
                puts("undefined");
                continue;
            }


        }
    }
    return 0;
}