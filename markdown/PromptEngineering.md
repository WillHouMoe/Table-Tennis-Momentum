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
                         double w_cap = 0.6, double w_M = 0.2, double w_delta_M = 0.2) {
    double elo = (player.cap * w_cap + (M_self * w_M - delta_M * w_delta_M * (1 - player.psy))) * player.sta;
    return std::max(0.0, std::min(1.0, elo));
} // * passed

// 计算momentum，返回的五个参数：M_A, M_B
std::tuple<double, double> calc_momentum(std::vector<PointInfo>& points, int game_idx) {
    if (points.empty()) return {0.0, 0.0};

    double numerator1 = 0.0, numerator2 = 0.0, denominator = 0.0;
    int start_idx = std::max(0, (int)points.size() - WINDOW_SIZE);

    for (int k = start_idx; k < points.size(); k++) {
        int distance = points.size() - 1 - k;
        double decay = points[k].game_idx ? alpha : beta;
        double weight = pow(decay, distance);
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
PDD winningRate(int scr1, int scr2, int game_idx) {
    int batch_size = 10000;
    int win1 = 0, win2 = 0;
    for (int i = 1; i <= batch_size; i++) {
        int cur_scr1 = scr1, cur_scr2 = scr2;
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
            calc_momentum(sim_points, game_idx);
        }
        if (isGameOver(cur_scr1, cur_scr2) == 1) {
            win1++;
        } else {
            win2++;
        }
    }
    return {1.0 * win1 / batch_size, 1.0 * win2 / batch_size};
}

double calc_leverage(int scr1, int scr2, int game_idx) {
    double rtwp_win = winningRate(scr1 + 1, scr2, game_idx).first;
    double rtwp_lose = winningRate(scr1, scr2 + 1, game_idx).first;
    return rtwp_win - rtwp_lose;
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
```

运行上面的代码，利用代码的输出绘制 M_A/B 的走势图，参考下面这张图的风格。局与局之间用虚线划分。 另外，这个代码的得分序列讲的是东京奥运会时樊振东与张本智和的比赛。图例用他们的英文名字"Fan Zhendong"和"Harimoto"。

---

这是我的最新模型代码：

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

double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

double calc_exponential_decay(double x) {
    // 0.9 * e^(-0.5*(x-1)) + 0.1
    double exponent = -0.5 * (x - 1.0);
    double expResult = exp(exponent);
    double functionValue = 0.9 * expResult + 0.1;
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

// 计算momentum，返回的五个参数：M_A, M_B
std::tuple<double, double> calc_momentum(std::vector<PointInfo>& points, int game_idx) {
    if (points.empty()) return {0.0, 0.0};

    double numerator1 = 0.0, numerator2 = 0.0, denominator = 0.0;
    int start_idx = std::max(0, (int)points.size() - WINDOW_SIZE);

    for (int k = start_idx; k < points.size(); k++) {
        int distance = points.size() - 1 - k;
        double decay = points[k].game_idx ? alpha : beta;
        double weight = pow(decay, distance);
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
```

为了凸显局点得分对于比赛势能的影响，我加入了一个函数来限制 leverage。但是我发现，这样让第一局期间张本连续得分时，M_A太低。你觉得应该怎么修改才能使之

---

我要对整个模型进行重构。具体地，我现在要建立一套模型，计算比赛的实时胜率，然后再引入“暂停”这一机制，通过判断暂停前后对比赛胜率的影响，帮助教练和选手决策暂停时机。我想要通过这样一套逻辑计算胜率：

![image-20251025190554737](E:\课题研究\markdown\PromptEngineering.assets\image-20251025190554737.png)

如图，引入胜率的若干影响因素：`capability` 表示选手本来实力，由用户提供；`serve` 表示当前选手发球轮次得分能力，由用户提供【未来可在这里加入实时计算发球轮次得分能力】；`game_point_scoring_rate` 表示选手赛点得分率，由用户提供；`potential` 表示选手当前势能，由过去得分序列 `history_scoring_sequence` 和当前分的关键程度（图中被写作 `game_point`，不是很恰当）作为系数决定。`potential` 关于 `history_scoring_sequence` 的函数关系用二次移动平均法计算：
$$
M_t ^{(1)} = \frac{\alpha_t Y_t + \alpha_{t-1} Y_{t-1} + \alpha_{t-2} Y_{t-2} + \alpha_{t-3} Y_{t-3}}{\alpha_{t} + \alpha_{t-1} + \alpha_{t-2} + \alpha_{t-3}} \\
M_t ^{(2)} = \frac{M_t ^{(1)} + M_{t-1} ^{(1)} + M_{t-2} ^{(1)} + M_{t-3} ^{(1)}}{4}
$$
其中，$M_t^{(2)}$ 表示势能 `potential`，$\alpha$ 表示这一分的关键程度。

比赛期间，该系统通过蒙特卡洛模拟，计算实时胜率。具体地，用 `elo_rating` 表示当前选手能力状态评估，它是`capability/serve/game_point_scoring_rate/potential` 的线性组合函数，权重你可以自由设定。需要注意的是，进行蒙特卡洛模拟的时候，你也应该通过模拟局面的 `history_scoring_sequence` 计算实时胜率。

下面，请你结合我们上面讨论过的所有内容，重点关注我这条 Prompt，写 c++ 代码实现这个模型。如果你有其他不明确的地方，可以提出来。代码需要尽可能结构化，易于维护和增删。

为了更好地帮你实现这个模型，我需要确认几个关键点：

1. 关于`capability`、`serve`、`game_point_scoring_rate`这三个参数，用户提供的是固定值还是需要动态更新？如果是动态更新，触发更新的条件是什么？

2. 蒙特卡洛模拟的具体要求是什么？比如模拟次数、是否考虑历史得分序列的影响、是否需要并行计算等。

3. 暂停机制的触发条件是什么？比如是否基于胜率变化阈值、时间限制或其他条件？

4. 二次移动平均法中的`α`值（关键程度系数）如何确定？是否需要根据当前比分动态调整？

5. 输出要求是什么？是否需要实时显示胜率变化、建议暂停时机，还是只需要计算胜率？


请提供这些信息，我将为你编写结构化、易于维护的C++代码。

1. 对于前两个，是用户提供的固定值。对于最后这个，用户也会提供初始值，但是“暂停”会提升这个值，提升的幅度你可以自己决定，但要合理。

2. 模拟次数为10000，你要考虑模拟局面的得分序列对于后续模拟的影响。并行计算暂不用考虑。你可以类比我们原先写的这个，加入一个 sim_points 来表示模拟局面。

3. 暂停是你实时模拟的，你要在每一分时模拟两个胜率，一个是不暂停的胜率，另一个是暂停后，改变相应的量后，得到的胜率。后者减前者，即为你需要输出的内容。

4. $\alpha$ 你可以先自己决定，但是一定要满足：比赛前期 $\alpha$ 必须小于比赛局末 $\alpha$，暂停后，对手势能的 $\alpha$ 小于己方势能的 $\alpha$。值得注意的是，$\alpha$ 应该实时改变，随比赛进程整体成上升趋势的同时，在暂停后也要逐渐回到原来的趋势中。

5. 输出要包括每一分后，势能、胜率、暂停胜率、暂停胜率-胜率，当然你也可以多数出些测试信息。

---

现在提供如下这些函数，请用蒙特卡洛模拟实现整局胜率计算。选手 A 的胜率可以这样计算：`winA = playerA_elo / (playerA_elo + playerB_elo);` 选手 B 同理。

请注意，在模拟胜率时，也要符合全局计算 `elo` 的方式，即通过计算 calc_momentum 实时更新模拟对局的双方势能等。

```cpp
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


double calc_elo(char id, int game_idx, int scrA, int scrB) {
    int _player = (id == playerA.id) ? 1 : 2;
    Player& player = (id == playerA.id) ? playerA : playerB;
    auto& points = (id == playerA.id) ? playerA_points : playerB_points;
    bool is_serve = (_player == next_server(game_idx, scrA, scrB));
    bool is_game_point = game_point(scrA, scrB);

    return 0.5 * player.cap + 0.1 * player.ser * is_serve + 0.2 * (is_game_point ? player.game_point : 1) + 0.2 * points.back().mom_avg2;
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

            double playerA_elo = calc_elo(playerA.id, game_idx, scrA, scrB);
            double playerB_elo = calc_elo(playerB.id, game_idx, scrA, scrB);
        }
    }
    return 0;
}
```

