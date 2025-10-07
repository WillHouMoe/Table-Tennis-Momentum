import pandas as pd
import matplotlib.pyplot as plt

# --------------------------
# 1. 数据读取与得分方判断
# --------------------------
columns = ["Point #N", "Game", "Score(H:F)", "L_i", "G_A", "G_B", "M_A", "M_B", "Elo_H", "Elo_F"]
df = pd.read_csv(
    "E:\课题研究\plot\output_model_0_4.txt",  # 替换为实际文件路径
    sep="\s+",
    skiprows=1,
    header=None,
    names=columns
)

x = df["Point #N"]       # 累计得分数（横轴）
y1, y2 = df["M_A"], df["M_B"]  # 双方势能（纵轴折线）

# 解析Score列，判断每一分的得分者
h_scores, f_scores = [], []
for score_str in df["Score(H:F)"]:
    h, f = map(int, score_str.split(":"))
    h_scores.append(h)
    f_scores.append(f)

# 生成柱状图高度：张本得分→0.1，樊振东得分→-0.1，无得分→0
bar_heights = []
h_prev, f_prev = 0, 0
for h, f in zip(h_scores, f_scores):
    if h > h_prev:
        bar_heights.append(0.1)
    elif f > f_prev:
        bar_heights.append(-0.1)
    else:
        bar_heights.append(0)  # 应对开局等特殊情况
    h_prev, f_prev = h, f

# 提取Game列，找到每局的分界点（Game值变化时的Point #N）
game_values = df["Game"].values
game_change_points = []
prev_game = game_values[0]
for i in range(1, len(game_values)):
    if game_values[i] != prev_game:
        game_change_points.append(df["Point #N"].iloc[i])
        prev_game = game_values[i]


# --------------------------
# 2. 图表绘制与美化
# --------------------------
plt.rcParams['font.sans-serif'] = ['SimHei', 'DejaVu Sans']  # 中文字体
plt.rcParams['axes.unicode_minus'] = False  # 负号显示
fig, ax = plt.subplots(figsize=(14, 7))

# ---- 步骤1：绘制得分柱状图 ----
ax.bar(
    x,
    bar_heights,
    width=1,  # 柱宽与得分点间隔匹配
    color=['#ff7f0e' if h > 0 else '#1f77b4' for h in bar_heights],  # 张本橙色，樊振东蓝色
    alpha=0.3,  # 透明度（避免遮挡折线）
    edgecolor='none',  # 无边框
    zorder=1  # 柱状图在折线图下方
)

# ---- 步骤2：绘制势能折线图 ----
ax.plot(
    x, y1,
    label="张本智和",
    color="#ff7f0e",
    linewidth=1.8,
    alpha=0.8,
    zorder=2  # 折线在柱状图上方
)
ax.plot(
    x, y2,
    label="樊振东",
    color="#1f77b4",
    linewidth=1.8,
    alpha=0.8,
    zorder=2
)

# ---- 绘制局之间的垂直虚线 ----
for point in game_change_points:
    ax.axvline(
        x=point,
        color='gray',    # 虚线颜色
        linestyle='--',  # 虚线样式
        alpha=0.5,       # 透明度
        zorder=0         # 虚线在最底层，不遮挡其他元素
    )

# ---- 步骤3：轴与标题设置 ----
ax.set_xlabel("累计得分数（Point #N）", fontsize=12, fontweight="bold")
ax.set_ylabel("势能 / 得分柱", fontsize=12, fontweight="bold")
ax.set_title(
    "张本智和 vs 樊振东 势能与得分走势图",
    fontsize=16,
    fontweight="bold",
    pad=20
)

# 对称y轴范围（可根据数据调整，示例设为-0.5到0.5）
ax.set_ylim(-0.5, 0.5)

# x轴刻度：每隔20个得分显示
ax.set_xticks(range(0, int(max(x)) + 20, 20))
ax.set_xlim(0, max(x))

# ---- 步骤4：辅助元素（网格、排名标注、图例） ----
ax.grid(True, alpha=0.3, linestyle="--", axis='y')  # 仅y轴网格

# （可选）排名标注，取消注释即可显示
# ax.text(
#     60, 0.4,
#     "当前排名：1（张本智和）",
#     color="#ff7f0e",
#     fontsize=11,
#     fontweight="bold",
#     bbox=dict(boxstyle="round,pad=0.3", facecolor="#fff3e0", alpha=0.7)
# )
# ax.text(
#     60, -0.4,
#     "当前排名：2（樊振东）",
#     color="#1f77b4",
#     fontsize=11,
#     fontweight="bold",
#     bbox=dict(boxstyle="round,pad=0.3", facecolor="#e8f4fd", alpha=0.7)
# )

ax.legend(
    loc="upper right",
    fontsize=10,
    frameon=True,
    fancybox=True,
    shadow=True
)

# --------------------------
# 3. 保存与显示
# --------------------------
plt.tight_layout()  # 自动调整布局，防止标签截断
plt.savefig(
    "势能与得分走势图.png",
    dpi=300,
    bbox_inches="tight"
)
plt.show()