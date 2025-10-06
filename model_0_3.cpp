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
    double M_self;       // 自身势头
};

// 存储每一分的元数据（用于权重计算）
struct PointInfo {
    double G_A;       // A的杠杆获取量
    double G_B;       // B的杠杆获取量
    double M_A;       // 这一分后，A 的势能
    double M_B;       // 这一分后，B 的势能
    int game_idx;     // 所属局索引（0开始）
};

// 常量定义
const double alpha = 0.33;    // 当前局内衰减系数
const double beta = 0.5;      // 跨局衰减系数
const int WINDOW_SIZE = 5;    // 势能计算窗口

// 初始化球员数据
std::vector<Player> initializePlayers() {
    return {
        {"Player H", 'H', 0.45, 0.8, 0.9, 0.0},
        {"Player F", 'F', 0.55, 0.9, 0.9, 0.0}
    };
}

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
}

// 判断一局是否结束（乒乓球11分制，领先2分获胜）
int isGameOver(int score1, int score2) {
    int maxScore = std::max(score1, score2);
    int minScore = std::min(score1, score2);
    if (maxScore >= 11 && maxScore - minScore >= 2) {
        return (score1 > score2) ? 1 : 2; // 1为A胜，2为B胜
    }
    return 0; // 未结束
}

// 计算elo评分
double calculateEloRating(const Player& player, double M_self, double delta_M,
                         double w_cap = 0.6, double w_M = 0.2, double w_delta_M = 0.2) {
    double elo = (player.cap * w_cap + (M_self * w_M - delta_M * w_delta_M * (1 - player.psy))) * player.sta;
    return std::max(0.0, std::min(1.0, elo));
}

// 计算momentum，返回的五个参数：M_A, M_B
std::tuple<double, double> calc_momentum(
    std::vector<PointInfo> points,
    int pos // 后面 pos 个用衰减系数 alpha，其余用 beta
) {
    if (points.empty()) return {0.0, 0.0};

    double numerator1 = 0.0, numerator2 = 0.0, denominator = 0.0;
    int start_idx = std::max(0, (int)points.size() - WINDOW_SIZE);

    for (int k = start_idx; k < points.size(); k++) {
        int distance = points.size() - 1 - k;
        int decay = distance >= pos ? beta : alpha;
        double weight = pow(decay, distance);
        numerator1 += points[k].G_A * weight;
        numerator2 += points[k].G_B * weight;
        denominator += weight;
    }

    double M1 = (denominator != 0) ? numerator1 / denominator : 0.0;
    double M2 = (denominator != 0) ? numerator2 / denominator : 0.0;
    return {M1, M2};
}

// 使用elo评分计算实时获胜概率（新增当前局索引和当前分索引参数）
PDD winningRate()

// 计算全局比赛的势能相关参数（更新winningRate调用参数）
std::tuple<double, double, double, double, double> calculate_momentum(
    std::vector<PointInfo>& all_points,
    int current_point_idx,
    int game_idx,
    char winner,
    int scrA,
    int scrB,
    const Player& playerA,
    const Player& playerB,
    double alpha,
    double beta,
    int WINDOW_SIZE
) {
    // 计算当前局已进行的分数（用于sim_points初始化）
    int current_point_in_game = 0;
    for (const auto& p : all_points) {
        if (p.game_idx == game_idx) current_point_in_game++;
    }

    // 1. 计算当前分的杠杆L_i
    double current_M_A = 0.0, current_M_B = 0.0;
    if (!all_points.empty()) {
        int start_idx = std::max(0, (int)all_points.size() - (WINDOW_SIZE - 1));
        double numerator_A = 0.0, numerator_B = 0.0, denominator = 0.0;

        for (int k = start_idx; k < all_points.size(); ++k) {
            const PointInfo& p = all_points[k];
            int distance = all_points.size() - k - 1;
            double decay_factor = (p.game_idx == game_idx) ? (1 - alpha) : (1 - beta);
            double weight = pow(decay_factor, distance);
            numerator_A += p.G_A * weight;
            numerator_B += p.G_B * weight;
            denominator += weight;
        }

        current_M_A = (denominator != 0) ? numerator_A / denominator : 0.0;
        current_M_B = (denominator != 0) ? numerator_B / denominator : 0.0;
    }

    // 计算实时获胜概率（传入当前局索引和当前分在局中的位置）
    double rtwp = winningRate(playerA, playerB, scrA, scrB, std::abs(current_M_A), std::abs(current_M_B), game_idx, all_points, current_point_in_game).first;
    double rtwp_win = winningRate(playerA, playerB, scrA + 1, scrB, std::abs(current_M_A), std::abs(current_M_B), game_idx, all_points, current_point_in_game).first;
    double rtwp_lose = winningRate(playerA, playerB, scrA, scrB + 1, std::abs(current_M_A), std::abs(current_M_B), game_idx, all_points, current_point_in_game).first;
    // std::cerr << rtwp << ' ' << rtwp_win << ' ' << rtwp_lose << '\n';
    double L_i = rtwp_win - rtwp_lose;

    // 2. 计算杠杆获取量G_A/G_B
    double ga = (winner == playerA.id) ? L_i : 0.0;
    double gb = (winner == playerB.id) ? -L_i : 0.0;

    all_points.push_back({ga, gb, game_idx});

    // 3. 计算全局势能M_A/M_B
    double numerator_A = 0.0, numerator_B = 0.0, denominator = 0.0;
    int start_idx = std::max(0, current_point_idx - (WINDOW_SIZE - 1));
    for (int k = start_idx; k <= current_point_idx; ++k) {
        const PointInfo& p = all_points[k];
        int distance = current_point_idx - k;
        double decay_factor = (p.game_idx == game_idx) ? (1 - alpha) : (1 - beta);
        double weight = pow(decay_factor, distance);
        numerator_A += p.G_A * weight;
        numerator_B += p.G_B * weight;
        denominator += weight;
    }

    double M_A = (denominator != 0) ? numerator_A / denominator : 0.0;
    double M_B = (denominator != 0) ? numerator_B / denominator : 0.0;

    return {L_i, ga, gb, M_A, M_B};
}

int main() {
    freopen("cpp_output.txt", "w", stdout);

    std::vector<Player> players = initializePlayers();
    Player& playerA = players[0];
    Player& playerB = players[1];

    std::vector<std::string> game_seqs = get_game_score_seqs();
    std::vector<PointInfo> all_points;
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
            auto [L_i, ga, gb, M_A, M_B] = calculate_momentum(
                all_points,
                all_points.size(),
                game_idx,
                winner,
                scrA,
                scrB,
                playerA,
                playerB,
                alpha,
                beta,
                WINDOW_SIZE
            );

            // 更新球员势头和ELO
            playerA.M_self = M_A;
            playerB.M_self = M_B;
            double delta_M_A = M_B - M_A;
            double delta_M_B = M_A - M_B;
            double eloA = calculateEloRating(playerA, M_A, delta_M_A);
            double eloB = calculateEloRating(playerB, M_B, delta_M_B);

            // 更新比分
            if (winner == playerA.id) scrA++;
            else scrB++;
            total_point++;

            // 输出
            std::cout << total_point << "\t\t" << (game_idx + 1) << "\t"
                      << scrA << ":" << scrB << "\t\t"
                      << L_i << "\t" << ga << "\t" << gb << "\t"
                      << M_A << "\t" << M_B << "\t"
                      << eloA << "\t" << eloB << "\n";
        }
    }

    return 0;
}
