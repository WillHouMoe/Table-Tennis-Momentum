
# ! deprecated
# * explanation: unknown error
# * version explanation: plot

import numpy as np
import matplotlib.pyplot as plt
from collections import namedtuple
import random

# 函数：判断一局是否结束（乒乓球11分制，领先2分获胜）
def is_game_over(score1, score2):
    max_score = max(score1, score2)
    min_score = min(score1, score2)
    if max_score >= 11 and max_score - min_score >= 2:
        return 1 if score1 > score2 else 2  # 1=樊振东胜，2=张本智和胜
    return 0

# 函数：蒙特卡洛模拟实时获胜概率
def winning_rate(pot1, pot2, scr1, scr2, batch_size=10000):
    win1, win2 = 0, 0
    for _ in range(batch_size):
        cur1, cur2 = scr1, scr2
        while not is_game_over(cur1, cur2):
            dice = random.uniform(0.0, pot1 + pot2)
            cur1 += 1 if dice <= pot1 else 0
            cur2 += 1 if dice > pot1 else 0
        win1 += 1 if is_game_over(cur1, cur2) == 1 else 0
        win2 += 1 if is_game_over(cur1, cur2) == 2 else 0
    return win1 / batch_size, win2 / batch_size

# 常量与得分序列定义
PLAYER_A_NAME = "Fan Zhendong"  # 对应原代码中 'F'
PLAYER_B_NAME = "Harimoto"  # 对应原代码中 'H'
ALPHA = 0.33        # 局内衰减系数
BETA = 0.5          # 跨局衰减系数（比局内衰减更快）
POT1 = 0.45         # 樊振东基础得分权重
POT2 = 0.55         # 张本智和基础得分权重
WINDOW_SIZE = 5     # 势能计算窗口（当前球 + 前4球）

def get_game_score_seqs():
    """按局返回得分序列，'F'=樊振东得分，'H'=张本智和得分"""
    return [
        "HFHHHHHHHHHFH",        # 第1局
        "HHFFHFFFHHFHHHFFHFHH", # 第2局
        "FFFFFFHHFHFFFHF",      # 第3局
        "HFHFFHHHFFHHFFFFFF",   # 第4局
        "HHHHFFHHHFHHFHH",      # 第5局
        "FHFFHFFFHHFHHHFFFF",   # 第6局
        "FFHHHHFFFFHHHFFFFF"    # 第7局
    ]

# 存储每一分元数据的结构
PointInfo = namedtuple('PointInfo', ['G_A', 'G_B', 'game_idx'])

def main():
    game_seqs = get_game_score_seqs()
    all_points = []          # 存储所有分的杠杆与局信息
    ma_values = []           # 樊振东的势能序列
    mb_values = []           # 张本智和的势能序列
    point_indices = []       # 得分点序号（用于x轴）
    game_boundaries = [0]    # 局分界点（第几个球）

    for game_idx, seq in enumerate(game_seqs):
        scrA, scrB = 0, 0    # 本局内得分（每局重置）
        for winner_char in seq:
            total_point = len(point_indices) + 1
            point_indices.append(total_point)

            # 1. 计算当前分的杠杆 L_i（实时获胜概率差）
            if winner_char == 'H':  # 张本智和得分
                rtwp_win, _ = winning_rate(POT1, POT2, scrA + 1, scrB)
                _, rtwp_lose = winning_rate(POT1, POT2, scrA, scrB + 1)
            else:  # 樊振东得分
                rtwp_win, _ = winning_rate(POT1, POT2, scrA + 1, scrB)
                _, rtwp_lose = winning_rate(POT1, POT2, scrA, scrB + 1)
            L_i = rtwp_win - rtwp_lose

            # 2. 计算杠杆获取量 G_A（樊振东）、G_B（张本智和）
            ga = L_i if winner_char == 'F' else 0.0
            gb = -L_i if winner_char == 'H' else 0.0
            all_points.append(PointInfo(G_A=ga, G_B=gb, game_idx=game_idx))

            # 3. 更新本局比分
            scrA += 1 if winner_char == 'F' else 0
            scrB += 1 if winner_char == 'H' else 0

            # 4. 计算势能 M_A（樊振东）、M_B（张本智和）
            numerator_a, numerator_b, denominator = 0.0, 0.0, 0.0
            current_idx = len(all_points) - 1
            start_idx = max(0, current_idx - (WINDOW_SIZE - 1))
            for k in range(start_idx, current_idx + 1):
                p = all_points[k]
                distance = current_idx - k
                # 同局用ALPHA衰减，跨局用BETA衰减（更快）
                decay = (1 - ALPHA) if p.game_idx == game_idx else (1 - BETA)
                weight = decay ** distance
                numerator_a += p.G_A * weight
                numerator_b += p.G_B * weight
                denominator += weight
            m_a = numerator_a / denominator if denominator != 0 else 0.0
            m_b = numerator_b / denominator if denominator != 0 else 0.0
            ma_values.append(m_a)
            mb_values.append(m_b)

        # 记录本局结束的得分点（用于绘制局间虚线）
        game_boundaries.append(len(point_indices))

    print(ma_values)

    print(mb_values)

    # # --------------------- 绘制势能走势图 ---------------------
    # plt.figure(figsize=(12, 6))
    # # 樊振东的势能
    # plt.plot(point_indices, ma_values, label=PLAYER_A_NAME,
    #          color="#FF6B6B", linewidth=2)
    # # 张本智和的势能
    # plt.plot(point_indices, mb_values, label=PLAYER_B_NAME,
    #          color="#45B798", linewidth=2)
    # # 局间分隔虚线
    # for boundary in game_boundaries[1:-1]:  # 排除首尾（起点和终点）
    #     plt.axvline(x=boundary, color="gray", linestyle="--", alpha=0.7)
    # # 图表样式设置
    # plt.title("Tokyo Olympic (Fan Zhendong vs Harimoto) Potential(M)", fontsize=15)
    # plt.xlabel("Points", fontsize=12)
    # plt.ylabel("Potential(M)", fontsize=12)
    # plt.legend(fontsize=12)
    # plt.grid(alpha=0.3)
    # plt.tight_layout()
    # plt.show()

if __name__ == "__main__":
    main()
