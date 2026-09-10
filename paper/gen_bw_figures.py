import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

plt.rcParams["font.family"] = "monospace"
plt.rcParams["font.weight"] = "bold"
plt.rcParams["axes.linewidth"] = 0
plt.rcParams["text.color"] = "black"

CODE = {
    'backprop': 'BP', 'bfs': 'BF', 'bplustree': 'BT', 'canneal': 'CN',
    'facesim': 'FS', 'freqmine': 'FM', 'leukocyte': 'LK', 'nn': 'NN',
    'nw': 'NW', 'particlefilter': 'PF', 'srad': 'SR',
}

data = {
    'backprop':       dict(baseline=43.265141, baseline_bypass=14.509348, boundnoc_delay=99.024626, boundnoc_bypass=26.079913, bypass_bb=6502314, bypass_bp=6383434, jitter_bd=212272954, jitter_bp=42666932),
    'bfs':            dict(baseline=42.817901, baseline_bypass=12.036606, boundnoc_delay=99.000000, boundnoc_bypass=21.324031, bypass_bb=928315,   bypass_bp=928315,   jitter_bd=31024092,   jitter_bp=5128935),
    'bplustree':      dict(baseline=41.420779, baseline_bypass=11.737824, boundnoc_delay=99.000000, boundnoc_bypass=21.360701, bypass_bb=7438234,  bypass_bp=7438234,  jitter_bd=250691348,  jitter_bp=41896529),
    'canneal':        dict(baseline=42.825368, baseline_bypass=12.078037, boundnoc_delay=99.000000, boundnoc_bypass=21.269413, bypass_bb=6213701,  bypass_bp=6213701,  jitter_bd=209405828,  jitter_bp=34265143),
    'facesim':        dict(baseline=43.013498, baseline_bypass=11.997632, boundnoc_delay=99.000255, boundnoc_bypass=21.304340, bypass_bb=597309,   bypass_bp=597300,   jitter_bd=19768736,   jitter_bp=3286047),
    'freqmine':       dict(baseline=43.024863, baseline_bypass=12.010308, boundnoc_delay=99.000000, boundnoc_bypass=21.311711, bypass_bb=14492874, bypass_bp=14492874, jitter_bd=481060684,  jitter_bp=79938217),
    'leukocyte':      dict(baseline=34.488930, baseline_bypass=13.948733, boundnoc_delay=99.028925, boundnoc_bypass=26.058905, bypass_bb=31435237, bypass_bp=31432038, jitter_bd=1506485166, jitter_bp=282576108),
    'nn':             dict(baseline=119.352429,baseline_bypass=69.816105, boundnoc_delay=159.378226,boundnoc_bypass=85.044564, bypass_bb=3556722,  bypass_bp=3559157,  jitter_bd=53008236,   jitter_bp=17401729),
    'nw':             dict(baseline=42.974039, baseline_bypass=12.131012, boundnoc_delay=99.000000, boundnoc_bypass=41.415220, bypass_bb=7192995,  bypass_bp=7192995,  jitter_bd=238731664,  jitter_bp=123780726),
    'particlefilter': dict(baseline=42.716863, baseline_bypass=11.971468, boundnoc_delay=99.000000, boundnoc_bypass=21.407892, bypass_bb=5447122,  bypass_bp=5447122,  jitter_bd=179819276,  jitter_bp=30148549),
    'srad':           dict(baseline=41.119385, baseline_bypass=14.160235, boundnoc_delay=99.598046, boundnoc_bypass=25.721426, bypass_bb=22815312, bypass_bp=23267502, jitter_bd=819770665,  jitter_bp=172092717),
}

HATCH_NONE = None
HATCH_A = '///'
HATCH_B = 'xx'
BLACK = 'black'
WHITE = 'white'

def style_axes(ax, log=False):
    for spine in ['top', 'right', 'left', 'bottom']:
        ax.spines[spine].set_visible(False)
    ax.tick_params(axis='both', which='both', length=0)
    ax.set_axisbelow(True)
    if log:
        ax.yaxis.grid(True, which='major', color='0.85', linewidth=1.1)
    else:
        ax.yaxis.grid(True, which='major', color='0.85', linewidth=1.1)
    ax.xaxis.grid(False)

def big_num_fmt(v, _pos=None):
    if v >= 1e9:
        return f"{v/1e9:.2f}B" if v/1e9 < 10 else f"{v/1e9:.1f}B"
    if v >= 1e6:
        return f"{v/1e6:.2f}M" if v/1e6 < 10 else f"{v/1e6:.1f}M"
    if v >= 1e3:
        return f"{v/1e3:.1f}K" if v/1e3 < 10 else f"{v/1e3:.0f}K"
    return f"{v:.0f}"

def draw_break(ax, x, w, y):
    """small zigzag break mark at top of a clipped bar"""
    n = 4
    xs = np.linspace(x - w*0.5, x + w*0.5, n*2+1)
    ys = [y + (0.012 if i % 2 == 0 else -0.012) for i in range(len(xs))]
    ax.plot(xs, ys, color='white', linewidth=4, zorder=5, solid_capstyle='round')
    ax.plot(xs, ys, color='black', linewidth=1.6, zorder=6, solid_capstyle='round')

# ---------- Chart 1: Latency (4 series, linear, clipped at 100) ----------
names_sorted = sorted(data.keys(), key=lambda n: -data[n]['baseline'])
codes = [CODE[n] for n in names_sorted]
series = ['baseline', 'baseline_bypass', 'boundnoc_delay', 'boundnoc_bypass']
labels = ['BASELINE', 'BASELINE_BYPASS', 'BOUNDNOC_DELAY', 'BOUNDNOC_BYPASS']
fills = [WHITE, WHITE, WHITE, BLACK]
hatches = [HATCH_NONE, HATCH_A, HATCH_B, HATCH_NONE]

fig, ax = plt.subplots(figsize=(12.4, 2.9), dpi=200)
x = np.arange(len(names_sorted))
n_series = len(series)
bar_w = 0.8 / n_series
YMAX = 100

for si, (key, lab, fill, hatch) in enumerate(zip(series, labels, fills, hatches)):
    offs = (si - (n_series-1)/2) * bar_w
    vals = [data[n][key] for n in names_sorted]
    for xi, v in zip(x, vals):
        h = min(v, YMAX)
        ax.bar(xi + offs, h, width=bar_w*0.92, color=fill, edgecolor='black',
               linewidth=1.4, hatch=hatch, zorder=3)
        if v > YMAX:
            draw_break(ax, xi + offs, bar_w*0.92, YMAX)
            ax.text(xi + offs, YMAX + 4, f"{v:.0f}", ha='center', va='bottom',
                    fontsize=13, fontweight='bold', family='monospace')

ax.set_xticks(x)
ax.set_xticklabels(codes, fontsize=17, fontweight='bold')
ax.set_ylabel('cycles', fontsize=17, fontweight='bold')
ax.set_ylim(0, YMAX * 1.18)
ax.tick_params(axis='y', labelsize=15)
for lbl in ax.get_yticklabels():
    lbl.set_fontweight('bold')
style_axes(ax, log=False)

handles = [plt.Rectangle((0,0),1,1, facecolor=f, edgecolor='black', hatch=h, linewidth=1.4)
           for f, h in zip(fills, hatches)]
ax.legend(handles, labels, loc='upper right', ncol=4, frameon=True, edgecolor='black',
          fontsize=13.5, handlelength=1.6, handleheight=1.4, columnspacing=1.2,
          bbox_to_anchor=(1.0, 1.12), prop={'weight':'bold','family':'monospace','size':13.5})

fig.tight_layout(pad=0.4)
fig.savefig("/tmp/claude-1880884998/-home-harpreetsc-noc/67f3bf0d-266c-499d-82f2-b60e4c6d1153/scratchpad/figures_mpl/boundnoc_latency_combined_1B.png",
            facecolor='white', bbox_inches='tight')
plt.close(fig)
print("latency done")

# ---------- Chart 2: Bypass count (2 series, log) ----------
names_sorted2 = sorted(data.keys(), key=lambda n: -data[n]['bypass_bb'])
codes2 = [CODE[n] for n in names_sorted2]
series2 = ['bypass_bb', 'bypass_bp']
labels2 = ['BASELINE_BYPASS', 'BOUNDNOC_BYPASS']
fills2 = [WHITE, BLACK]
hatches2 = [HATCH_A, HATCH_NONE]

fig, ax = plt.subplots(figsize=(12.4, 2.9), dpi=200)
x = np.arange(len(names_sorted2))
n_series = len(series2)
bar_w = 0.6 / n_series
for si, (key, lab, fill, hatch) in enumerate(zip(series2, labels2, fills2, hatches2)):
    offs = (si - (n_series-1)/2) * bar_w
    vals = [data[n][key] for n in names_sorted2]
    ax.bar(x + offs, vals, width=bar_w*0.92, color=fill, edgecolor='black',
           linewidth=1.4, hatch=hatch, zorder=3)

ax.set_yscale('log')
ax.set_ylim(80, 5e7)
ax.yaxis.set_major_formatter(mticker.FuncFormatter(big_num_fmt))
ax.set_xticks(x)
ax.set_xticklabels(codes2, fontsize=17, fontweight='bold')
ax.set_ylabel('bypassed pkts', fontsize=17, fontweight='bold')
ax.tick_params(axis='y', labelsize=15)
for lbl in ax.get_yticklabels():
    lbl.set_fontweight('bold')
style_axes(ax, log=True)

handles = [plt.Rectangle((0,0),1,1, facecolor=f, edgecolor='black', hatch=h, linewidth=1.4)
           for f, h in zip(fills2, hatches2)]
ax.legend(handles, labels2, loc='upper right', ncol=2, frameon=True, edgecolor='black',
          fontsize=14, handlelength=1.8, handleheight=1.5, columnspacing=1.4,
          bbox_to_anchor=(1.0, 1.12), prop={'weight':'bold','family':'monospace','size':14})

fig.tight_layout(pad=0.4)
fig.savefig("/tmp/claude-1880884998/-home-harpreetsc-noc/67f3bf0d-266c-499d-82f2-b60e4c6d1153/scratchpad/figures_mpl/boundnoc_bypass_count_1B.png",
            facecolor='white', bbox_inches='tight')
plt.close(fig)
print("bypass done")

# ---------- Chart 3: Delay amount (2 series, log) ----------
names_sorted3 = sorted(data.keys(), key=lambda n: -data[n]['jitter_bd'])
codes3 = [CODE[n] for n in names_sorted3]
series3 = ['jitter_bd', 'jitter_bp']
labels3 = ['BOUNDNOC_DELAY', 'BOUNDNOC_BYPASS']
fills3 = [WHITE, BLACK]
hatches3 = [HATCH_A, HATCH_NONE]

fig, ax = plt.subplots(figsize=(12.4, 2.9), dpi=200)
x = np.arange(len(names_sorted3))
n_series = len(series3)
bar_w = 0.6 / n_series
for si, (key, lab, fill, hatch) in enumerate(zip(series3, labels3, fills3, hatches3)):
    offs = (si - (n_series-1)/2) * bar_w
    vals = [data[n][key] for n in names_sorted3]
    ax.bar(x + offs, vals, width=bar_w*0.92, color=fill, edgecolor='black',
           linewidth=1.4, hatch=hatch, zorder=3)

ax.set_yscale('log')
ax.set_ylim(1e6, 3e9)
ax.yaxis.set_major_formatter(mticker.FuncFormatter(big_num_fmt))
ax.set_xticks(x)
ax.set_xticklabels(codes3, fontsize=17, fontweight='bold')
ax.set_ylabel('cycles', fontsize=17, fontweight='bold')
ax.tick_params(axis='y', labelsize=15)
for lbl in ax.get_yticklabels():
    lbl.set_fontweight('bold')
style_axes(ax, log=True)

handles = [plt.Rectangle((0,0),1,1, facecolor=f, edgecolor='black', hatch=h, linewidth=1.4)
           for f, h in zip(fills3, hatches3)]
ax.legend(handles, labels3, loc='upper right', ncol=2, frameon=True, edgecolor='black',
          fontsize=14, handlelength=1.8, handleheight=1.5, columnspacing=1.4,
          bbox_to_anchor=(1.0, 1.12), prop={'weight':'bold','family':'monospace','size':14})

fig.tight_layout(pad=0.4)
fig.savefig("/tmp/claude-1880884998/-home-harpreetsc-noc/67f3bf0d-266c-499d-82f2-b60e4c6d1153/scratchpad/figures_mpl/boundnoc_delay_amount_1B.png",
            facecolor='white', bbox_inches='tight')
plt.close(fig)
print("delay done")
