import matplotlib.pyplot as plt
import numpy as np
# 导入三次样条插值函数（核心：实现曲线平滑）
from scipy.interpolate import make_interp_spline

# ========================================================
# 原始数据（已修复None语法错误）
# ========================================================
A_PAUSE = [
    None,None,None,0.6269,0.7225,0.8123,0.8730,0.9292,0.9665,0.9883,0.9967,0.9945,
    1,0.5372,0.6210,0.5352,0.4433,0.5299,0.4241,0.3256,0.2445,0.3213,0.4432,0.3412,
    0.4770,0.6014,0.7497,0.6362,0.4822,0.6456,0.4302,0.6895,1,0.3777,0.2878,0.2011,
    0.1352,0.0907,0.0513,0.0848,0.1288,0.0844,0.1327,0.0716,0.0279,0.0055,0.0104,0,
    0.5009,0.4194,0.5260,0.4401,0.3447,0.4356,0.5409,0.6589,0.5593,0.4432,0.5647,0.7001,
    0.5718,0.4370,0.2912,0.1503,0.0398,0,0.4819,0.5954,0.6996,0.7882,0.7134,0.6350,
    0.7160,0.7976,0.8889,0.8408,0.9139,0.9617,0.9435,0.9844,1,0.3752,0.4377,0.3601,
    0.2815,0.3505,0.2455,0.1791,0.1193,0.1690,0.2432,0.1670,0.2564,0.3630,0.4905,0.3540,
    0.1950,0.0591,0,0.3212,0.2527,0.3355,0.4220,0.5334,0.6621,0.5699,0.4447,0.3586,
    0.2364,0.3196,0.4349,0.6078,0.4991,0.3396,0.1754,0.0549,0
]

# --------------------------------------------------------
# None 替换为前一个有效值（首项为0.5）
# --------------------------------------------------------
for i in range(len(A_PAUSE)):
    if A_PAUSE[i] is None:
        A_PAUSE[i] = A_PAUSE[i-1] if i > 0 else 0.5

A_PAUSE = np.array(A_PAUSE)
points = np.arange(1, len(A_PAUSE) + 1)

# ========================================================
# 核心：三次样条插值实现曲线平滑
# ========================================================
# 生成500个均匀的X轴插值点（点数越多，曲线越平滑）
x_smooth = np.linspace(points.min(), points.max(), 500)
# 创建三次样条插值函数（k=3表示三次样条，最常用的平滑方式）
spl = make_interp_spline(points, A_PAUSE, k=3)
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
plt.ylabel("Real-time Pause Winning Probability", fontsize=14)
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