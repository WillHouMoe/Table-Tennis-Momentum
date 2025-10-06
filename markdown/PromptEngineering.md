```cpp
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <utility>
#include <cmath>
#include <iomanip>

std::mt19937 gen(std::chrono::system_clock().now().time_since_epoch().count());
using PDD = std::pair<double, double>;

int isGameOver(int score1, int score2) {
    int maxScore, maxPlayer, minScore, minPlayer;
    if (score1 > score2) {
        maxScore = score1;
        maxPlayer = 1;
        minScore = score2;
        minPlayer = 2;
    } else {
        maxScore = score2;
        maxPlayer = 2;
        minScore = score1;
        minPlayer = 1;
    }
    if ((maxScore >= 11) && (maxScore - minScore >= 2)) {
        return maxPlayer;
    } else {
        return 0;
    }
}

PDD winningRate(double pot1, double pot2, int scr1, int scr2) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    std::uniform_real_distribution<double> distribution(0.0, pot1 + pot2);
    for (int i = 1; i <= batch_size; i++) {
        int cur1 = scr1, cur2 = scr2;
        while (isGameOver(cur1, cur2) == false) {
            double dice = distribution(gen);
            if (dice <= pot1) {
                cur1++;
            } else {
                cur2++;
            }
        }
        if (isGameOver(cur1, cur2) == 1) {
            win1++;
        } else {
            win2++;
        }
    }
    return std::make_pair(1.0 * win1 / batch_size, 1.0 * win2 / batch_size);
}

const char PLAYER_A = 'H';
const char PLAYER_B = 'F';
const double alpha = 0.33;
const double pot1 = 0.45;
const double pot2 = 0.55;

const std::string get_full_score_seq() {
    std::vector<std::string> score_parts = {
        // "HFHHHHHHHHHFH"
        // "HHFFHFFFHHFHHHFFHFHH"
        // "FFFFFFHHFHFFFHF"
        // "HFHFFHHHFFHHFFFFFF"
        // "HHHHFFHHHFHHFHH"
        // "FHFFHFFFHHFHHHFFFF"
        "FFHHHHFFFFHHHFFFFF"
        // "FFFFFFFFFFF"
    };
    std::string full_seq;
    for (const auto& part : score_parts) {
        full_seq += part;
    }
    return full_seq;
}

int main() {
    std::string score_seq = get_full_score_seq();
    int scrA = 0, scrB = 0;
    std::vector<double> G_A, G_B;
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Point #N\tScore(A:B)\tL_i\t\tG_A\t\tG_B\t\tM_A\t\tM_B\n";
    std::cout << "-------------------------------------------------------------------------------------------------------\n";

    double M_A = pot1, M_B = pot2;

    for (int i = 0; i < score_seq.size(); i++) {
        char current_winner = score_seq[i];
        double rtwp_win, rtwp_lose, L_i;

        rtwp_win = winningRate(M_A, M_B, scrA + 1, scrB).first;
        rtwp_lose = winningRate(M_A, M_B, scrA, scrB + 1).first;
        L_i = rtwp_win - rtwp_lose;
        std::cerr << std::fixed << std::setprecision(6) << rtwp_win << " " << rtwp_lose << '\n';

        double ga, gb;

        if (current_winner == PLAYER_A) {
            scrA++, ga = L_i, gb = 0;
        } else {
            scrB++, ga = 0, gb = -L_i;
        }
        G_A.push_back(ga);
        G_B.push_back(gb);

        double numerator_A = 0.0, numerator_B = 0.0, denominator = 0.0;
        int current_idx = i;
        for (int k = 0; k <= current_idx; k++) {
            double weight = pow(1 - alpha, current_idx - k);
            numerator_A += G_A[k] * weight;
            numerator_B += G_B[k] * weight;
            denominator += weight;
        }
        double M_A = numerator_A / denominator;
        double M_B = numerator_B / denominator;

        std::cout << i + 1 << "\t\t" << scrA << ":" << scrB << "\t\t"
                  << L_i << "\t" << ga << "\t" << gb << "\t"
                  << M_A << "\t" << M_B << "\n";
    }

    return 0;
}
```

下面，我希望你不止笼统地把所有局的比赛情况串联起来。我希望你考虑换局对势能的影响。具体的，我需要你做出如下更改：

1. 开始下一局球后，上一局球的比分对势能的影响权重减小，对上一局球的 `G_A/B` 的系数 $1 - \alpha$ 更小，但保持当前这一局球的 `G_A/B` 系数不变。
2. 引入新的假设：只有每一球的及其前四球（共5球，如果有换局就用上一局的比分情况，并且根据 1 做出相应权重调整；如果不到四球，就认为其先所有球都对这球势能有影响）。

---

```cpp
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
PDD winningRate(double pot1, double pot2, int scr1, int scr2) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    std::uniform_real_distribution<double> distribution(0.0, pot1 + pot2);
    for (int i = 0; i < batch_size; ++i) {
        int cur1 = scr1, cur2 = scr2;
        while (!isGameOver(cur1, cur2)) {
            double dice = distribution(gen);
            if (dice <= pot1) cur1++;
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
                rtwp_win = winningRate(pot1, pot2, scrA + 1, scrB).first;
                rtwp_lose = winningRate(pot1, pot2, scrA, scrB + 1).first;
            } else {
                rtwp_win = winningRate(pot1, pot2, scrA + 1, scrB).first;
                rtwp_lose = winningRate(pot1, pot2, scrA, scrB + 1).first;
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
```

将上面的代码改写成python，增加并且增加绘制 M_A/B 的走势图，参考下面这张图的风格。局与局之间用虚线划分。 另外，这个代码的得分序列讲的是东京奥运会时樊振东与张本智和的比赛。用具体的名字代替“H”,"F"。

---

```cpp
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
PDD winningRate(double pot1, double pot2, int scr1, int scr2) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    std::uniform_real_distribution<double> distribution(0.0, pot1 + pot2);
    for (int i = 0; i < batch_size; ++i) {
        int cur1 = scr1, cur2 = scr2;
        while (!isGameOver(cur1, cur2)) {
            double dice = distribution(gen);
            if (dice <= pot1) cur1++;
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
                rtwp_win = winningRate(pot1, pot2, scrA + 1, scrB).first;
                rtwp_lose = winningRate(pot1, pot2, scrA, scrB + 1).first;
            } else {
                rtwp_win = winningRate(pot1, pot2, scrA + 1, scrB).first;
                rtwp_lose = winningRate(pot1, pot2, scrA, scrB + 1).first;
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
```
把 momentum 的计算单独提出来，写成一个函数，从而实现类似于这样的效果：

```cpp
int main() {
    std::vector<std::string> game_seqs = get_game_score_seqs(); // 按局存储的得分序列
    std::vector<PointInfo> all_points; // 存储所有分的元数据（跨局）
    int total_point = 0;               // 全局得分计数（跨局）

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Point #N\tGame\tScore(A:B)\tL_i\t\tG_A\t\tG_B\t\tM_A\t\tM_B\n";
    std::cout << "-----------------------------------------------------------------------------------------------------------------\n";

    // 遍历每一局
    for (int game_idx = 0; game_idx < game_seqs.size(); ++game_idx) {
        const std::string& seq = game_seqs[game_idx];
        int scrA = 0, scrB = 0; // 本局内得分（每局重置）

        // 遍历本局每一分
        for (char winner : seq) {
            ( calc_momentum(...); ) // 传入参数应包括当前比分、这一回合谁得分，有必要的话可以引入其他参数，实现M_A, M_B的计算。
            // 当然，calc_momentum 不一定非得是 void 函数，可以根据需要自由设定。
            
            total_point++;


            // 5. 输出结果
            std::cout << total_point << "\t\t" << (game_idx + 1) << "\t"
                      << scrA << ":" << scrB << "\t\t"
                      << L_i << "\t" << ga << "\t" << gb << "\t"
                      << M_A << "\t" << M_B << "\n";
        }
    }

    return 0;
}
```

---

> 给每个选手添加 `struct`，计算 `elo_rating`。用这个 `elo` 代替
>
> `database` 应该包括：
>
> 1. 初始实力（预设，用世界排名反映）
> 2. 心理素质（预设，用职业生涯时长反映）
>
> 心理素质和势能综合起来，对 `elo_rating` 产生影响。具体地，心理素质越强，势能对其发挥影响越小。

现在，`winning_rate` 函数只使用了选手实力作为唯一的局势评判标准。我现在想要增加心理素质、状态系数作为第二、三个评价标准。具体地，可以用下面这个数学式子计算实时 `elo_rating`。

`cap` 表示选手实力，`psy` 表示心理素质，`sta` 表示状态系数，`|M|` 表示选手那一分前的 momentum 的绝对值。则 `elo_rating = (cap * 0.7 + psy * 0.2 + |M| * 0.1) * sta`
