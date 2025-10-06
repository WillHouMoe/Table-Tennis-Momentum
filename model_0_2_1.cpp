/* deprecated */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <utility>
#include <cmath>
#include <iomanip>
#include <sstream>

// 随机数生成器（保持不变）
std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
using PDD = std::pair<double, double>;

// 判断一局是否结束（乒乓球11分制，领先2分获胜）
int isGameOver(int score1, int score2) {
    int maxScore = std::max(score1, score2);
    int minScore = std::min(score1, score2);
    if (maxScore >= 11 && maxScore - minScore >= 2) {
        return (score1 > score2) ? 1 : 2; // 1为A胜，2为B胜
    }
    return 0; // 未结束
}

// 蒙特卡洛模拟实时获胜概率（保持不变）
PDD winningRate(double mom1, double mom2, int scr1, int scr2) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    std::uniform_real_distribution<double> distribution(mom2, mom1);
    for (int i = 0; i < batch_size; ++i) {
        int cur1 = scr1, cur2 = scr2;
        while (!isGameOver(cur1, cur2)) {
            double dice = distribution(gen);
            if (dice > 0) cur1++;
            else cur2++;
        }
        if (isGameOver(cur1, cur2) == 1) win1++;
        else win2++;
    }
    return {1.0 * win1 / batch_size, 1.0 * win2 / batch_size};
}

// 常量定义（新增跨局衰减系数beta，beta > alpha确保上一局权重衰减更快）
const char PLAYER_A = 'H';
const char PLAYER_B = 'F';
const double alpha = 0.33;    // 当前局内衰减系数（1-alpha为局内权重）
const double beta = 0.5;      // 跨局衰减系数（1-beta为跨局权重，比1-alpha小）
const double pot1 = 0.45;     // 初始势能权重（A）
const double pot2 = 0.55;     // 初始势能权重（B）
const int WINDOW_SIZE = 5;    // 势能计算窗口（当前球+前4球）

// 按局拆分得分序列（不再拼接为长字符串，保留局边界）
const std::vector<std::string> get_game_score_seqs() {
    return {
        "HFHHHHHHHHHFH",        // 第1局
        "HHFFHFFFHHFHHHFFHFHH", // 第2局
        "FFFFFFHHFHFFFHF",      // 第3局
        "HFHFFHHHFFHHFFFFFF",   // 第4局
        "HHHHFFHHHFHHFHH",      // 第5局
        "FHFFHFFFHHFHHHFFFF",   // 第6局
        "FFHHHHFFFFHHHFFFFF"     // 第7局（测试用）
    };
}

// 存储每一分的元数据（用于权重计算）
struct PointInfo {
    double G_A;       // A的杠杆获取量
    double G_B;       // B的杠杆获取量
    int game_idx;     // 所属局索引（0开始）
};

int main() {
    std::vector<std::string> game_seqs = get_game_score_seqs(); // 按局存储的得分序列
    std::vector<PointInfo> all_points; // 存储所有分的元数据（跨局）
    int total_point = 0;               // 全局得分计数（跨局）

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Point #N\tGame\tScore(A:B)\tL_i\t\tG_A\t\tG_B\t\tM_A\t\tM_B\n";
    std::cout << "-----------------------------------------------------------------------------------------------------------------\n";

    double M_A = 0.3, M_B = -0.2;

    // 遍历每一局
    for (int game_idx = 0; game_idx < game_seqs.size(); ++game_idx) {
        const std::string& seq = game_seqs[game_idx];
        int scrA = 0, scrB = 0; // 本局内得分（每局重置）

        // 遍历本局每一分
        for (char winner : seq) {
            total_point++;
            double rtwp_win, rtwp_lose, L_i;

            // 1. 计算当前分的杠杆L_i（实时获胜概率差）
            if (winner == PLAYER_A) {
                rtwp_win = winningRate(M_A, M_B, scrA + 1, scrB).first;
                rtwp_lose = winningRate(M_A, M_B, scrA, scrB + 1).first;
            } else {
                rtwp_win = winningRate(M_A, M_B, scrA + 1, scrB).first;
                rtwp_lose = winningRate(M_A, M_B, scrA, scrB + 1).first;
            }
            L_i = rtwp_win - rtwp_lose;

            // 2. 计算杠杆获取量G_A/G_B（文档定义）
            double ga = (winner == PLAYER_A) ? L_i : 0.0;
            double gb = (winner == PLAYER_B) ? -L_i : 0.0; // B的G为-L_i（保持符号一致性）
            all_points.push_back({ga, gb, game_idx});

            // 3. 更新本局比分
            if (winner == PLAYER_A) scrA++;
            else scrB++;

            // 4. 计算势能M_A/M_B（核心：5球窗口+跨局权重调整）
            double numerator_A = 0.0, numerator_B = 0.0, denominator = 0.0;
            int current_point_idx = all_points.size() - 1; // 当前分在全局的索引

            // 取窗口内的分（当前分+前4分，共WINDOW_SIZE分）
            int start_idx = std::max(0, current_point_idx - (WINDOW_SIZE - 1));
            for (int k = start_idx; k <= current_point_idx; ++k) {
                const PointInfo& p = all_points[k];
                int distance = current_point_idx - k; // 与当前分的距离（0为当前分，1为前1分...）

                // 确定权重衰减系数：同局用1-alpha，跨局用1-beta（衰减更快）
                double decay_factor = (p.game_idx == game_idx) ? (1 - alpha) : (1 - beta);
                double weight = pow(decay_factor, distance); // 距离越远权重越小

                // 累加势能计算所需值
                numerator_A += p.G_A * weight;
                numerator_B += p.G_B * weight;
                denominator += weight;
            }

            // 计算当前势能（避免除零）
            double M_A = (denominator != 0) ? numerator_A / denominator : 0.0;
            double M_B = (denominator != 0) ? numerator_B / denominator : 0.0;

            // 5. 输出结果
            std::cout << total_point << "\t\t" << (game_idx + 1) << "\t"
                      << scrA << ":" << scrB << "\t\t"
                      << L_i << "\t" << ga << "\t" << gb << "\t"
                      << M_A << "\t" << M_B << "\n";
        }
    }

    return 0;
}
