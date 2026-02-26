#include <iostream>
#include <algorithm>
#include <string>
#include <tuple>
#include <random>
#include <chrono>
#include <cmath>
#include <iomanip>

// 随机数生成器
std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
using PDD = std::pair<double, double>;

struct Player {
    std::string name;
    char id;
    double cap;
    double ser;
    double psy;
    double hot;
    double cld;
};

struct PointInfo {
    bool win;
    int game_idx;
    double alpha;
    double mom_avg1;
    double mom_avg2;
    bool post_pause;
    double advantage;

    PointInfo(bool win_, int game_idx_, double alpha_, double mom_avg1_,
            double mom_avg2_, bool post_pause_, double advantage_)
        : win(win_), game_idx(game_idx_), alpha(alpha_), mom_avg1(mom_avg1_),
        mom_avg2(mom_avg2_), post_pause(post_pause_), advantage(advantage_) {}
};

std::vector<Player> initializePlayers() {
    return {
        {"Harimoto", 'H', 0.45, 0.5, 0.8, 0.15, 0.04},
        {"Fan Zhendong", 'F', 0.55, 0.6, 0.9, 0.1, 0.04}
    };
}
std::vector<Player> players = initializePlayers();
Player& playerA = players[0];
Player& playerB = players[1];

std::vector<PointInfo> playerA_points;
std::vector<PointInfo> playerB_points;

const int WINDOW_SIZE = 4;
const int SIMULATIONS = 10000;
const double AFTER_PAUSE_GO_MID = 0.8;
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
inline double calc_game_progress(int scrA, int scrB) {
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

    return std::min(std::max(finalProgress, 0.92), 0.99);
}

inline int next_server(int game_idx, int scrA, int scrB) {
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

inline int game_point(int scrA, int scrB) {
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

inline int is_game_over(int scrA, int scrB) {
    int maxScore = std::max(scrA, scrB);
    int minScore = std::min(scrA, scrB);
    if (maxScore >= 11 && maxScore - minScore >= 2) {
        return (scrA > scrB) ? 1 : 2;
    }
    return 0;
}

inline double calc_alpha(int scrA, int scrB) {
    return 0.5 * calc_game_progress(scrA, scrB) + 0.5;
}

inline double calc_regular_advantage(double k, int points) {
    double val = k * points;
    if (val >= 0.8) return 0.8;
    return val;
}

inline double calc_pause_advantage(double k, int points) {
    double val = 1 - points * k;
    if (val <= 0.8) return 0.8;
    return val;
}

inline double calc_opponent_pause_advantage(double k, int delta) {
    double val = 0.4 + k * delta;
    if (val >= 0.8) return 0.8;
    return val;
}

void calc_momentum(std::vector<PointInfo>& points) {
    int last_idx = points.size() - 1;
    if (points.size() < WINDOW_SIZE) {
        points[last_idx].mom_avg1 = points[last_idx].mom_avg2 = 0.5;
        return;
    }

    double avg1 = 0, numerator = 0, denominator = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        double weight = 1.0;
        PointInfo& temp = points[last_idx - i];

        if (temp.game_idx != points[last_idx].game_idx) weight = 0.3;
        if (temp.post_pause != points[last_idx].post_pause) weight = 0.3;

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

void read_PointInfo(char status, int game_idx, int& scrA, int& scrB, int total_points) {
    double a_advantage = calc_regular_advantage(playerA.hot, total_points);
    double b_advantage = calc_regular_advantage(playerB.hot, total_points);
    if (status == playerA.id) {
        scrA++;
        double alpha = calc_alpha(scrA, scrB);
        playerA_points.emplace_back(1, game_idx, alpha, 0, 0, false, a_advantage);
        playerB_points.emplace_back(0, game_idx, alpha, 0, 0, false, b_advantage);
    } else {
        scrB++;
        double alpha = calc_alpha(scrA, scrB);
        playerA_points.emplace_back(0, game_idx, alpha, 0, 0, false, a_advantage);
        playerB_points.emplace_back(1, game_idx, alpha, 0, 0, false, b_advantage);
    }
    calc_momentum(playerA_points);
    calc_momentum(playerB_points);
}

double calc_elo(char id, int game_idx, int scrA, int scrB, Player& player, std::vector<PointInfo>& points) {
    int _player = (id == playerA.id) ? 1 : 2;
    bool is_serve = (_player == next_server(game_idx, scrA, scrB));
    bool is_game_point = game_point(scrA, scrB);
    double advantage = (points.empty() ? 0 : points.back().advantage);
    double momentum = (points.empty() ? 0 : points.back().mom_avg2);

    return 0.4 * player.cap + 0.1 * player.ser * is_serve +
        0.15 * ((calc_game_progress(scrA, scrB) > 0.95) ? player.psy : 1) +
        0.15 * advantage + 0.2 * momentum;
}

/**
 * @brief 蒙特卡洛模拟胜率（支持暂停场景）
 * @param current_game 当前局数
 * @param scrA 当前A得分
 * @param scrB 当前B得分
 * @param total_points 当前总分数
 * @param historyA A的历史得分信息
 * @param historyB B的历史得分信息
 * @param pause_caller 暂停方：0=无暂停，1=A叫暂停，2=B叫暂停
 * @param pause_total_points 暂停发生时的总分数
 * @return A的胜率
 */
double monte_carlo_simulation(int current_game,
                            int scrA, int scrB, int total_points,
                            const std::vector<PointInfo>& historyA,
                            const std::vector<PointInfo>& historyB,
                            int pause_caller = 0,
                            int pause_total_points = 0) {

    int a_wins = 0;

    for (int sim = 0; sim < SIMULATIONS; sim++) {
        // 复制当前状态
        int sim_game = current_game;
        int sim_scrA = scrA;
        int sim_scrB = scrB;
        int sim_total_points = total_points;

        std::vector<PointInfo> sim_historyA = historyA;
        std::vector<PointInfo> sim_historyB = historyB;

        int game_over = is_game_over(sim_scrA, sim_scrB);
        if (game_over) {  // 初始比分已结束，直接判定胜负
            a_wins += (game_over == 1);
            continue;  // 跳过后续模拟循环
        }

        if (pause_caller != 0) {
            auto& caller_hist = (pause_caller == 1 ? sim_historyA : sim_historyB);
            auto& opp_hist = (pause_caller == 1 ? sim_historyB : sim_historyA);

            if (!caller_hist.empty()) {
                caller_hist.back().advantage = 1.0;
            }

            if (!opp_hist.empty()) {
                opp_hist.back().advantage = 0.4;
            }
        }

        while (!game_over) {
            double eloA = calc_elo(playerA.id, sim_game, sim_scrA, sim_scrB, playerA, sim_historyA);
            double eloB = calc_elo(playerB.id, sim_game, sim_scrA, sim_scrB, playerB, sim_historyB);

            double winA = eloA / (eloA + eloB);
            std::uniform_real_distribution<double> dis(0.0, 1.0);
            double rand_val = dis(gen);

            bool a_won_point = rand_val < winA;
            a_won_point ? sim_scrA++ : sim_scrB++;
            sim_total_points++;

            // 计算暂停相关的参数
            bool post_pause = false;
            double a_advantage = 0.0, b_advantage = 0.0;
            int delta = sim_total_points - pause_total_points; // 暂停后经过的分数

            if (pause_caller != 0 && delta >= 0) {
                post_pause = true;
                if (pause_caller == 1) {
                    // A叫暂停：A用pause优势，B用常规优势
                    a_advantage = calc_pause_advantage(playerA.cld, delta);
                    b_advantage = calc_opponent_pause_advantage(playerB.hot, sim_total_points);
                } else if (pause_caller == 2) {
                    // B叫暂停：B用pause优势，A用常规优势
                    a_advantage = calc_opponent_pause_advantage(playerA.hot, sim_total_points);
                    b_advantage = calc_pause_advantage(playerB.cld, delta);
                }
            } else {
                // 无暂停：双方用常规优势
                a_advantage = calc_regular_advantage(playerA.hot, sim_total_points);
                b_advantage = calc_regular_advantage(playerB.hot, sim_total_points);
            }

            double alpha = calc_alpha(sim_scrA, sim_scrB);
            sim_historyA.emplace_back(a_won_point, sim_game, alpha, 0, 0, post_pause, a_advantage);
            sim_historyB.emplace_back(!a_won_point, sim_game, alpha, 0, 0, post_pause, b_advantage);

            calc_momentum(sim_historyA);
            calc_momentum(sim_historyB);

            game_over = is_game_over(sim_scrA, sim_scrB);
            if (game_over) a_wins += (game_over == 1);
        }
    }

    return static_cast<double>(a_wins) / SIMULATIONS;
}

int main() {
    std::vector<std::string> game_seqs = get_game_score_seqs();
    int total_points = 0;
    double A_DELTA_MAX = -1, B_DELTA_MAX = -1;
    int A_DELTA_MAX_ID, B_DELTA_MAX_ID;
    // 设置输出格式，对齐列头
    std::cout << std::left
            << std::setw(10) << "POINT_ID"
            << std::setw(10) << "GAME_ID"
            << std::setw(10) << "scrA"
            << std::setw(10) << "scrB"
            << std::setw(15) << "A_NO_PAUSE"
            << std::setw(15) << "B_NO_PAUSE"
            << std::setw(15) << "A_PAUSE"
            << std::setw(15) << "B_PAUSE"
            << std::setw(15) << "A_DELTA"
            << std::setw(15) << "B_DELTA"
            << std::endl;
    std::cout << std::string(124, '-') << std::endl;

    for (int game_idx = 0; game_idx < game_seqs.size(); game_idx++) {
        const std::string& seq = game_seqs[game_idx];
        int scrA = 0, scrB = 0;

        for (int point_idx = 0; point_idx < seq.size(); point_idx++) {
            total_points++;
            read_PointInfo(seq[point_idx], game_idx, scrA, scrB, total_points);

            if (total_points < 4) {
                // 总分数不足4时，胜率为undefined
                std::cout << std::left
                        << std::setw(10) << total_points
                        << std::setw(10) << game_idx
                        << std::setw(10) << scrA
                        << std::setw(10) << scrB
                        << std::setw(15) << "undefined"
                        << std::setw(15) << "undefined"
                        << std::setw(15) << "undefined"
                        << std::setw(15) << "undefined"
                        << std::setw(15) << "undefined"
                        << std::setw(15) << "undefined"
                        << std::endl;
                continue;
            }

            // 1. 无暂停场景的胜率
            double A_NO_PAUSE = monte_carlo_simulation(game_idx, scrA, scrB, total_points, playerA_points, playerB_points);
            double B_NO_PAUSE = 1 - A_NO_PAUSE;

            // 2. A叫暂停的胜率（pause_caller=1，暂停发生在当前total_points）
            double A_PAUSE = monte_carlo_simulation(game_idx, scrA, scrB, total_points, playerA_points, playerB_points, 1, total_points);
            double B_PAUSE = 1 - monte_carlo_simulation(game_idx, scrA, scrB, total_points, playerA_points, playerB_points, 2, total_points);

            double A_DELTA = A_PAUSE - A_NO_PAUSE;
            double B_DELTA = B_PAUSE - B_NO_PAUSE;
            if (A_DELTA > A_DELTA_MAX) {
                A_DELTA_MAX = A_DELTA;
                A_DELTA_MAX_ID = total_points;
            }
            if (B_DELTA > B_DELTA_MAX) {
                B_DELTA_MAX = B_DELTA;
                B_DELTA_MAX_ID = total_points;
            }
            // 输出结果（保留6位小数）
            std::cout << std::left
                    << std::setw(10) << total_points
                    << std::setw(10) << game_idx
                    << std::setw(10) << scrA
                    << std::setw(10) << scrB
                    << std::setw(15) << std::fixed << std::setprecision(6) << A_NO_PAUSE
                    << std::setw(15) << std::fixed << std::setprecision(6) << B_NO_PAUSE
                    << std::setw(15) << std::fixed << std::setprecision(6) << A_PAUSE
                    << std::setw(15) << std::fixed << std::setprecision(6) << B_PAUSE
                    << std::setw(15) << std::fixed << std::setprecision(6) << A_DELTA
                    << std::setw(15) << std::fixed << std::setprecision(6) << B_DELTA
                    << std::endl;
        }
    }
    std::cout << "A_DELTA_MAX = " << A_DELTA_MAX << std::endl;
    std::cout << "A_DELTA_MAX_ID = " << A_DELTA_MAX_ID << std::endl;
    std::cout << "B_DELTA_MAX = " << B_DELTA_MAX << std::endl;
    std::cout << "B_DELTA_MAX_ID = " << B_DELTA_MAX_ID << std::endl;
    return 0;
}