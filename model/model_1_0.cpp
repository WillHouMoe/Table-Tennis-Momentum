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

/********************************definition***********************************/

// 球员结构体 - 存储球员数据
struct Player {
    std::string name;    // 球员名称
    char id;             // 球员标识(H/F)
    double cap;          // 基础实力
    double psy;          // 心理素质
    double sta;          // 状态系数

    double elo(double M_self, double delta_M,
                       double w_cap = 0.7, double w_M = 0.2, double w_delta_M = 0.1) const {
        double elo = (cap * w_cap + (M_self * w_M - delta_M * w_delta_M * (1 - psy))) * sta;
        return sigmoid(elo);
    }

    double elo(double w_cap = 0.8, double w_psy = 0.2) {
        double elo = cap * w_cap + psy * w_psy;
        return elo;
    }

private:
    double sigmoid(double x) const {
        return 1.0 / (1.0 + std::exp(-x));
    }
};

// 存储每一分的元数据（用于权重计算）
struct PointInfo {
    int W;
    double L;
    double G_A;       // A的杠杆获取量
    double G_B;       // B的杠杆获取量
    double M_A;       // 这一分后，A 的势能
    double M_B;       // 这一分后，B 的势能
    int game_idx;     // 所属局索引（0开始）
    PointInfo(double w, double l, double ga, double gb, double ma, double mb, int g_idx)
        : W(w), L(l), G_A(ga), G_B(gb), M_A(ma), M_B(mb), game_idx(g_idx) {}
};
std::vector<PointInfo> all_points;

// 初始化球员数据
std::vector<Player> initializePlayers() {
    return {
        {"Harimoto", 'H', 0.45, 0.8, 0.9},
        {"Fan Zhendong", 'F', 0.55, 0.9, 0.9}
    };
} // * passed
std::vector<Player> players = initializePlayers();
Player& playerA = players[0];
Player& playerB = players[1];

/********************************definition***********************************/

/*******************************constant value********************************/

// 常量定义
const double alpha = 0.33;    // 当前局内衰减系数
const double beta = 0.5;      // 跨局衰减系数
const int WINDOW_SIZE = 5;    // 势能计算窗口
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

/*******************************constant value********************************/

int is_game_over(int score1, int score2) {
    int maxScore = std::max(score1, score2);
    int minScore = std::min(score1, score2);
    if (maxScore >= 11 && maxScore - minScore >= 2) {
        return (score1 > score2) ? 1 : 2;
    }
    return 0;
}

void fill(std::vector<PointInfo> points) {
    int points1 = 0, points2 = 0; // points the player scored in the past 8
    int consecutive = 0; // how many points does the player get consecutively

}

std::tuple<int, int> simulation(std::vector<PointInfo> points, int scr1, int scr2, int game_idx) {
    int cur_scr1 = scr1, cur_scr2 = scr2, cnt = 0;
    while (!is_game_over(cur_scr1, cur_scr2)) {
        double cur_M1, cur_M2;
        double cur_delta_M1, cur_delta_M2;
        if (points.empty()) {
            cur_M1 = cur_M2 = cur_delta_M1 = cur_delta_M2 = 0;
        } else {
            PointInfo &p = points.back();
            cur_M1 = std::abs(p.M_A);
            cur_M2 = std::abs(p.M_B);
            cur_delta_M1 = cur_M2 - cur_M1;
            cur_delta_M2 = cur_M1 - cur_M2;
        }
        double cur_elo1 = playerA.elo(cur_M1, cur_delta_M1);
        double cur_elo2 = playerB.elo(cur_M2, cur_delta_M2);
        std::uniform_real_distribution<double> distribution(0.0, cur_elo1 + cur_elo2);
        double dice = distribution(gen);
        if (dice <= cur_elo1) {
            cur_scr1++;
            points.emplace_back(1, 0, 0, 0, 0, 0, game_idx);
        } else {
            cur_scr2++;
            points.emplace_back(2, 0, 0, 0, 0, 0, game_idx);
        }
        cnt++;
        fill(points);
    }
    return {is_game_over(cur_scr1, cur_scr2), cnt};
}

std::tuple<double, double, double> winning_rate_montecarlo(int scr1, int scr2, int game_idx) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    double avg_cnt
}

int main() {
    std::vector<std::string> game_seqs = get_game_score_seqs();
    int total_points = 0;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Point #N\tGame\tScore(" << playerA.id << ":" << playerB.id
              << ")\tL_i\t\tG_A\t\tG_B\t\tM_A\t\tM_B\t\tElo_" << playerA.id
              << "\t\tElo_" << playerB.id << "\n";
    std::cout << "-----------------------------------------------------------------------------------------------------------------------------------------------------------------\n";

    all_points.emplace_back(0, 0, 0, 0, 0, 0, -1);
    for (int game_idx = 0; game_idx < game_seqs.size(); ++game_idx) {
        const std::string& seq = game_seqs[game_idx];
        int scrA = 0, scrB = 0;

        for (char winner : seq) {
            // 计算 winning rate

        }
    }

    return 0;
}