import pandas as pd
import matplotlib.pyplot as plt
import re
import os
from matplotlib.ticker import MaxNLocator

def cpp_output_to_excel(cpp_output_file, excel_file):
    """将C++程序输出的文本文件转换为Excel表格"""
    with open(cpp_output_file, 'r') as f:
        lines = f.readlines()

    # 提取标题行
    header_line = lines[0].strip()
    headers = re.split(r'\t+', header_line)
    headers = [h.strip() for h in headers if h.strip()]

    # 提取数据行
    data = []
    for line in lines[2:]:  # 跳过标题和分隔线
        line = line.strip()
        if not line:
            continue
        parts = re.split(r'\t+', line)
        parts = [p.strip() for p in parts if p.strip()]
        data.append(parts)

    # 创建DataFrame并保存为Excel
    df = pd.DataFrame(data, columns=headers)

    # 转换数值列的数据类型
    for col in df.columns:
        if col not in ['Point #N', 'Game', 'Score(H:F)']:
            df[col] = pd.to_numeric(df[col])

    df.to_excel(excel_file, index=False)
    print(f"数据已成功保存到 {excel_file}")
    return df

def plot_momentum(df, output_image="momentum_plot.png"):
    """绘制Momentum走势图，模仿参考图风格"""
    # 设置中文字体
    plt.rcParams["font.family"] = ["SimHei", "WenQuanYi Micro Hei", "Heiti TC"]
    plt.rcParams["axes.unicode_minus"] = False  # 正确显示负号

    # 创建图形和轴
    fig, ax = plt.subplots(figsize=(12, 6))

    # 提取数据
    points = df['Point #N'].astype(int)
    momentum_a = df['M_A']  # Player H的动量
    momentum_b = df['M_B']  # Player F的动量

    # 绘制线条
    line_a, = ax.plot(points, momentum_a, color='#FF6B6B', linewidth=2, label='Alcaraz')
    line_b, = ax.plot(points, momentum_b, color='#4ECDC4', linewidth=2, label='Djokovic')

    # 填充区域
    ax.fill_between(points, momentum_a, 0, color='#FF6B6B', alpha=0.3)
    ax.fill_between(points, momentum_b, 0, color='#4ECDC4', alpha=0.3)

    # 添加水平线（零点）
    ax.axhline(y=0, color='gray', linestyle='-', alpha=0.3)

    # 设置标题和标签
    ax.set_title('Momentum Tendency', fontsize=16, pad=20)
    ax.set_xlabel('Points', fontsize=12)
    ax.set_ylabel('Momentum', fontsize=12)

    # 设置坐标轴范围
    ax.set_xlim(0, max(points) + 10)
    ax.set_ylim(min(min(momentum_a), min(momentum_b)) - 0.02,
                max(max(momentum_a), max(momentum_b)) + 0.02)

    # 添加网格线
    ax.grid(True, linestyle='--', alpha=0.3)

    # 添加图例和当前排名
    # ax.legend(loc='upper left', fontsize=10)
    ax.text(0.02, 0.98, 'Harimoto', transform=ax.transAxes,
            verticalalignment='top', bbox=dict(boxstyle='round', facecolor='#FF6B6B', alpha=0.2))
    ax.text(0.02, 0.85, 'Fan Zhendong', transform=ax.transAxes,
            verticalalignment='top', bbox=dict(boxstyle='round', facecolor='#4ECDC4', alpha=0.2))

    # 设置刻度
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))

    # 添加垂直虚线分隔不同阶段（模拟）
    stage_points = [50, 150, 200, 270]  # 假设的阶段分隔点
    for point in stage_points:
        ax.axvline(x=point, color='gray', linestyle='--', alpha=0.5)

    # 调整布局并保存图像
    plt.tight_layout()
    plt.savefig(output_image, dpi=300, bbox_inches='tight')
    print(f"动量走势图已保存到 {output_image}")
    plt.show()

def main():
    # C++程序输出的文本文件（需要先运行C++程序并将输出重定向到该文件）
    cpp_output_file = "cpp_output.txt"
    # 要生成的Excel文件
    excel_file = "tennis_analysis.xlsx"

    # 检查C++输出文件是否存在
    if not os.path.exists(cpp_output_file):
        print(f"错误：未找到C++输出文件 {cpp_output_file}")
        print("请先运行C++程序，并将输出重定向到该文件，例如：")
        print("./tennis_sim > cpp_output.txt")
        return

    # 将C++输出转换为Excel
    df = cpp_output_to_excel(cpp_output_file, excel_file)

    # 绘制动量走势图
    plot_momentum(df)

if __name__ == "__main__":
    main()
