import matplotlib.pyplot as plt
import numpy as np
# 导入三次样条插值函数（核心：实现曲线平滑）
from scipy.interpolate import make_interp_spline

# ========================================================
# 原始数据（已修复None语法错误）
# ========================================================
A_NOPAUSE = [
    None,None,None,0.6377,0.7278,0.8209,0.8816,0.9269,0.9708,0.9887,0.9979,0.9951,
    1,0.5326,0.6284,0.5208,0.4257,0.5180,0.4127,0.3106,0.2204,0.3153,0.4333,0.3218,
    0.4676,0.6110,0.7545,0.6309,0.4527,0.6375,0.4140,0.6889,1,0.3558,0.2697,0.1851,
    0.1272,0.0867,0.0470,0.0810,0.1281,0.0700,0.1314,0.0570,0.0185,0.0030,0.0083,0,
    0.5047,0.4150,0.5189,0.4335,0.3217,0.4224,0.5373,0.6633,0.5491,0.4224,0.5566,0.6995,
    0.5575,0.4135,0.2562,0.1271,0.0327,0,0.4873,0.6054,0.6970,0.7881,0.7148,0.6163,
    0.7224,0.8064,0.8942,0.8371,0.9165,0.9635,0.9425,0.9866,1,0.3475,0.4326,0.3411,
    0.2581,0.3363,0.2533,0.1651,0.1057,0.1588,0.2444,0.1607,0.2491,0.3566,0.5102,0.3348,
    0.1677,0.0490,0,0.3017,0.2411,0.3275,0.4324,0.5496,0.6724,0.5619,0.4402,0.3232,
    0.2157,0.3139,0.4370,0.6048,0.4857,0.3105,0.1481,0.0444,0
]

# --------------------------------------------------------
# None 替换为前一个有效值（首项为0.5）
# --------------------------------------------------------
for i in range(len(A_NOPAUSE)):
    if A_NOPAUSE[i] is None:
        A_NOPAUSE[i] = A_NOPAUSE[i-1] if i > 0 else 0.5

A_NOPAUSE = np.array(A_NOPAUSE)
points = np.arange(1, len(A_NOPAUSE) + 1)

# ========================================================
# 核心：三次样条插值实现曲线平滑
# ========================================================
# 生成500个均匀的X轴插值点（点数越多，曲线越平滑）
x_smooth = np.linspace(points.min(), points.max(), 500)
# 创建三次样条插值函数（k=3表示三次样条，最常用的平滑方式）
spl = make_interp_spline(points, A_NOPAUSE, k=3)
# 计算插值后的平滑胜率值
A_smooth = spl(x_smooth)
B_smooth = 1 - A_smooth  # B的概率同步平滑

# ========================================================
# 分界点
# ========================================================
set_boundaries = [13,33,48,66,81,99]

# ========================================================
# 绘图（使用平滑后的数据）
# ========================================================
plt.figure(figsize=(14, 5))

# 用平滑后的数据填充区域，边界更顺滑
plt.fill_between(x_smooth, 0, A_smooth, color="#b7e3a1", alpha=0.95)  # Player A 绿色
plt.fill_between(x_smooth, A_smooth, 1, color="#f7b5a6", alpha=0.95)  # Player B 浅红
# 绘制平滑后的白色分割线
plt.plot(x_smooth, A_smooth, color="white", linewidth=2, antialiased=True)  # antialiased开启抗锯齿

# 大局分界线（改为窄白条，保留之前的优化）
for b in set_boundaries:
    plt.axvline(b, color="black", linestyle="--", linewidth=2, alpha=0.7)

# 小分细线（保留原有样式）
for b in range(10, int(points[-1]), 10):
    plt.axvline(b, color="black", linestyle=":", linewidth=0.5, alpha=0.4)

# 坐标轴与标签
plt.ylim(0, 1)
plt.xlim(points.min(), points.max())
plt.ylabel("Real-time Winning Probability", fontsize=14)
plt.xlabel("Points", fontsize=14)

# 隐藏顶部和右侧边框
plt.gca().spines['top'].set_visible(False)
plt.gca().spines['right'].set_visible(False)

# 标签对应正确的区域
plt.text(5, 0.10, "Harimoto", color="#003000", fontsize=16, weight="bold")
# Fan Zhendong 调整到右上角：x设为数据最大值-5，y设为0.9，右对齐+上对齐
plt.text(
    points[-1] - 5,  # x坐标：最后一个点的位置减5（留边距）
    0.9,             # y坐标：贴近顶部（0-1范围）
    "Fan Zhendong",
    color="#7c0000",
    fontsize=16,
    weight="bold",
    ha='right',      # 水平右对齐
    va='top'         # 垂直上对齐
)

plt.tight_layout()
plt.show()