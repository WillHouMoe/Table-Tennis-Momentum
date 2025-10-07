#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <utility>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <tuple>

// 随机数生成器
std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
using PDD = std::pair<double, double>;

// 球员结构体 - 存储球员数据
struct Player {
    std::string name;    // 球员名称
    char id;             // 球员标识(H/F)
    double cap;          // 基础实力
    double psy;          // 心理素质
    double sta;          // 状态系数
    // double M_self;       // 自身势头
};

// 存储每一分的元数据（用于权重计算）
struct PointInfo {
    double G_A;       // A的杠杆获取量
    double G_B;       // B的杠杆获取量
    double M_A;       // 这一分后，A 的势能
    double M_B;       // 这一分后，B 的势能
    int game_idx;     // 所属局索引（0开始）
    PointInfo(double ga, double gb, double ma, double mb, int g_idx)
        : G_A(ga), G_B(gb), M_A(ma), M_B(mb), game_idx(g_idx) {}
};

std::vector<PointInfo> all_points;

// 常量定义
const double alpha = 0.33;    // 当前局内衰减系数
const double beta = 0.5;      // 跨局衰减系数
const int WINDOW_SIZE = 5;    // 势能计算窗口

// 初始化球员数据
std::vector<Player> initializePlayers() {
    return {
        {"Player H", 'H', 0.45, 0.8, 0.9},
        {"Player F", 'F', 0.55, 0.9, 0.9}
    };
} // * passed
std::vector<Player> players = initializePlayers();
Player& playerA = players[0];
Player& playerB = players[1];

// 按局拆分得分序列
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
} // * passed

void print(std::vector<PointInfo> points) {
    std::cout << "-------------------PointInfo----------------------\n";
    std::cout << "GameIDX\t" << "G_A\t" << "G_B\t" << "M_A\t" << "M_B\n";
    for (auto p : points) {
        std::cout << p.game_idx << '\t' << p.G_A << '\t' << p.G_B << '\t' << p.M_A << '\t' << p.M_B << '\n';
    }
    std::cout << "-------------------PointInfoEnd-------------------\n";
}

double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

double calc_exponential_decay(double x) {
    // 0.9 * e^(-0.5*(x-1)) + 0.1
    double exponent = -0.2 * (x - 1.0);
    double expResult = exp(exponent);
    double functionValue = 0.7 * expResult + 0.3;
    return functionValue;
}

// 判断一局是否结束（乒乓球11分制，领先2分获胜）
int isGameOver(int score1, int score2) {
    int maxScore = std::max(score1, score2);
    int minScore = std::min(score1, score2);
    if (maxScore >= 11 && maxScore - minScore >= 2) {
        return (score1 > score2) ? 1 : 2; // 1为A胜，2为B胜
    }
    return 0; // 未结束
} // * passed

// 计算elo评分
double calculateEloRating(const Player& player, double M_self, double delta_M,
                         double w_cap = 0.7, double w_M = 0.2, double w_delta_M = 0.1) {
    double elo = (player.cap * w_cap + (M_self * w_M - delta_M * w_delta_M * (1 - player.psy))) * player.sta;
    return sigmoid(elo);
} // * passed

std::tuple<int, int> consecutive_scoring(std::vector<PointInfo>& points) {
    if (points.size() == 0) return {0, 0};
    PointInfo p = points.back();
    int scorer = p.G_A ? 1 : 2;
    int pos = points.size() - 1, cnt = 0;
    while (points[pos].game_idx == p.game_idx) {
        int cur_scorer = (points[pos].G_A) ? 1 : 2;
        if (cur_scorer == scorer) cnt++;
        else break;
        pos--;
    }
    return {cnt, scorer};
}

// 计算momentum，返回的五个参数：M_A, M_B
std::tuple<double, double> calc_momentum(std::vector<PointInfo>& points, int game_idx) {
    if (points.empty()) return {0.0, 0.0};

    double numerator1 = 0.0, numerator2 = 0.0, denominator = 0.0;
    int start_idx = std::max(0, (int)points.size() - WINDOW_SIZE);

    for (int k = start_idx; k < points.size(); k++) {
        int distance = points.size() - 1 - k;
        double decay = (points[k].game_idx == game_idx) ? alpha : beta;
        double weight = pow(1 - decay, distance);
        numerator1 += points[k].G_A * weight;
        numerator2 += points[k].G_B * weight;
        denominator += weight;
    }

    double M1 = (denominator != 0) ? numerator1 / denominator : 0.0;
    double M2 = (denominator != 0) ? numerator2 / denominator : 0.0;
    points.back().M_A = M1, points.back().M_B = M2;
    return {M1, M2};
}

// 使用elo评分计算实时获胜概率（新增当前局索引和当前分索引参数）
std::tuple<double, double, double> winningRate(int scr1, int scr2, int game_idx) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    double avg_cnt = 0;
    for (int i = 1; i <= batch_size; i++) {
        int cur_scr1 = scr1, cur_scr2 = scr2;
        int cnt = 0;
        std::vector<PointInfo> sim_points;
        for (auto p : all_points) {
            if (p.game_idx < game_idx - 1) continue;
            if (p.game_idx > game_idx) break;
            sim_points.emplace_back(p);
        }
        while (!isGameOver(cur_scr1, cur_scr2)) {
            double current_M1, current_M2;
            double current_delta_M1, current_delta_M2;
            if (sim_points.empty()) {
                current_M1 = current_M2 = current_delta_M1 = current_delta_M2 = 0;
            } else {
                PointInfo &p = sim_points.back();
                current_M1 = std::abs(p.M_A);
                current_M2 = std::abs(p.M_B);
                current_delta_M1 = current_M2 - current_M1;
                current_delta_M2 = current_M1 - current_M2;
            }
            double current_elo1 = calculateEloRating(playerA, current_M1, current_delta_M1);
            double current_elo2 = calculateEloRating(playerB, current_M2, current_delta_M2);
            std::uniform_real_distribution<double> distribution(0.0, current_elo1 + current_elo2);
            double dice = distribution(gen);
            if (dice <= current_elo1) {
                cur_scr1++;
                sim_points.emplace_back(current_elo1, 0.0, 0.0, 0.0, game_idx);
            } else {
                cur_scr2++;
                sim_points.emplace_back(0.0, -current_elo2, 0.0, 0.0, game_idx);
            }
            cnt++;
            calc_momentum(sim_points, game_idx);
        }
        // if (game_idx >= 2) print(sim_points);
        avg_cnt += cnt;
        if (isGameOver(cur_scr1, cur_scr2) == 1) {
            win1++;
        } else {
            win2++;
        }
    }
    return {1.0 * win1 / batch_size, 1.0 * win2 / batch_size, avg_cnt / batch_size};
}

double calc_leverage(int scr1, int scr2, int game_idx) {
    auto [rtwp_win, _1, _2] = winningRate(scr1 + 1, scr2, game_idx);
    auto [rtwp_lose, _3, _4] = winningRate(scr1, scr2 + 1, game_idx);
    auto [_5, _6, weight] = winningRate(scr1, scr2, game_idx);
    weight = calc_exponential_decay(weight);
    return std::min((rtwp_win - rtwp_lose) * weight, 0.2);
}

int main() {
    std::vector<std::string> game_seqs = get_game_score_seqs();
    int total_point = 0;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Point #N\tGame\tScore(" << playerA.id << ":" << playerB.id
              << ")\tL_i\t\tG_A\t\tG_B\t\tM_A\t\tM_B\t\tElo_" << playerA.id
              << "\t\tElo_" << playerB.id << "\n";
    std::cout << "-----------------------------------------------------------------------------------------------------------------------------------------------------------------\n";

    for (int game_idx = 0; game_idx < game_seqs.size(); ++game_idx) {
        const std::string& seq = game_seqs[game_idx];
        int scrA = 0, scrB = 0;

        for (char winner : seq) {
            double L = calc_leverage(scrA, scrB, game_idx);
            double ga = (winner == playerA.id) ? L : 0.0;
            double gb = (winner == playerB.id) ? -L : 0.0;
            all_points.emplace_back(ga, gb, 0.0, 0.0, game_idx);
            calc_momentum(all_points, game_idx);

            // 更新比分
            if (winner == playerA.id) scrA++;
            else scrB++;
            total_point++;

            /* test output */
            // 更新球员势头和ELO
            double M_A = all_points.back().M_A;
            double M_B = all_points.back().M_B;
            double delta_M_A = M_B - M_A;
            double delta_M_B = M_A - M_B;
            double eloA = calculateEloRating(playerA, M_A, delta_M_A);
            double eloB = calculateEloRating(playerB, M_B, delta_M_B);

            // 输出
            std::cout << total_point << "\t\t" << (game_idx + 1) << "\t"
                      << scrA << ":" << scrB << "\t\t"
                      << L << "\t" << ga << "\t" << gb << "\t"
                      << M_A << "\t" << M_B << "\t"
                      << eloA << "\t" << eloB << "\n";
            /* test output */
        }
    }

    return 0;
}
