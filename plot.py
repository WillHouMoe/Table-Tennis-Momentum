
# ! deprecated
# * explanation: unknown error
# * version explanation: plot

import pandas as pd
import matplotlib.pyplot as plt

# --------------------------
# 1. 数据读取与预处理
# --------------------------
# 读取output.txt，处理多空格分隔，跳过开头的分隔线（skiprows=1）
# 手动指定列名（确保与文档2的字段完全匹配）
columns = ["Point #N", "Game", "Score(H:F)", "L_i", "G_A", "G_B", "M_A", "M_B", "Elo_H", "Elo_F"]
df = pd.read_csv(
    "output.txt",  # 请确保该文件与代码在同一目录下，否则需写完整路径（如"D:/data/output.txt"）
    sep="\s+",     # 处理多空格分隔
    skiprows=1,    # 跳过开头的"--------"分隔线
    header=None,   # 原始数据第一行（非分隔线）为列名，故先设为None
    names=columns  # 手动赋值列名
)

# 提取核心数据：横轴（累计得分数）、纵轴（双方势能）
x = df["Point #N"]       # 横轴：累计得分数（Point #N）
y1 = df["M_A"]           # 纵轴1：球员A势能（对应张本智和，Elo_H）
y2 = df["M_B"]           # 纵轴2：球员B势能（对应樊振东，Elo_F）


# --------------------------
# 2. 图表绘制与美化
# --------------------------
# 设置中文字体（避免中文乱码）
plt.rcParams['font.sans-serif'] = ['SimHei']  # 兼容Windows和Linux
plt.rcParams['axes.unicode_minus'] = False  # 解决负号显示问题

# 创建画布（调整尺寸，确保横轴数据显示完整）
fig, ax = plt.subplots(figsize=(14, 7))  # 宽14英寸，高7英寸

# 绘制双折线：势能走势
line1 = ax.plot(
    x, y1,
    label="张本智和",  # 标签：球员+数据字段对应关系
    color="#1f77b4",  # 蓝色（区分双方）
    linewidth=1.8,    # 线条粗细
    alpha=0.8         # 透明度（避免过于刺眼）
)
line2 = ax.plot(
    x, y2,
    label="樊振东",
    color="#ff7f0e",  # 橙色
    linewidth=1.8,
    alpha=0.8
)

# --------------------------
# 3. 关键信息标注（参考image.png样式）
# --------------------------
# 3.1 轴标签与标题
ax.set_xlabel("累计得分数（Point #N）", fontsize=12, fontweight="bold")
ax.set_ylabel("势能（M_A / M_B）", fontsize=12, fontweight="bold")
ax.set_title(
    "张本智和 vs 樊振东 势能走势图",
    fontsize=16,
    fontweight="bold",
    pad=20  # 标题与图表的间距
)

# 1. 设置x轴刻度位置：从0开始，每隔50取一个值，直到覆盖最大累积分数（避免刻度超出数据范围）
ax.set_xticks(range(0, int(max(x)) + 20, 20))  # max(x)是最大累积分数（如117），+50确保最后一个刻度覆盖数据

# 2. （可选）强制x轴范围左边界为0（让“0”刻度对齐更美观，避免左边界从1开始导致0刻度悬空）
ax.set_xlim(0, max(x))  # 左边界0，右边界为最大累积分数（如117）

# 3.2 网格线（辅助读取数据）
ax.grid(True, alpha=0.3, linestyle="--")  # 虚线网格，透明度0.3

# # 3.3 排名标注（参考image.png的"Current ranking"）
# # 位置可根据实际数据调整（x取中间值，y取势能较高处，避免遮挡）
# ax.text(
#     x=60,  # 横轴位置（第60分附近）
#     y=max(y1.max(), y2.max()) - 0.03,  # 纵轴位置（略低于最大势能）
#     s="当前排名：1（张本智和）",
#     color="#1f77b4",
#     fontsize=11,
#     fontweight="bold",
#     bbox=dict(boxstyle="round,pad=0.3", facecolor="#e8f4fd", alpha=0.7)  # 带背景框，更醒目
# )
# ax.text(
#     x=60,
#     y=max(y1.max(), y2.max()) - 0.06,
#     s="当前排名：2（樊振东）",
#     color="#ff7f0e",
#     fontsize=11,
#     fontweight="bold",
#     bbox=dict(boxstyle="round,pad=0.3", facecolor="#fff3e0", alpha=0.7)
# )

# 3.4 图例（区分两条线）
ax.legend(
    loc="upper right",  # 图例位置（右上角，不遮挡数据）
    fontsize=10,
    frameon=True,
    fancybox=True,  # 圆角图例框
    shadow=True     # 阴影效果
)

# # 3.5 调整轴范围（让数据更舒展，避免边缘挤压）
# y_min = min(y1.min(), y2.min()) - 0.05  # 纵轴最小值（留0.05余量）
# y_max = max(y1.max(), y2.max()) + 0.05  # 纵轴最大值（留0.05余量）
# ax.set_ylim(y_min, y_max)
# ax.set_xlim(x.min(), x.max())  # 横轴范围：从第1分到最后1分


# --------------------------
# 4. 保存与显示
# --------------------------
# 保存图片（高分辨率，避免标签截断）
plt.tight_layout()  # 自动调整布局，防止标签被截断
plt.savefig(
    "双方势能走势图.png",
    dpi=300,          # 分辨率300dpi（高清）
    bbox_inches="tight"  # 紧凑布局，避免白边过大
)

# 显示图片（运行代码时会弹出窗口）
plt.show()