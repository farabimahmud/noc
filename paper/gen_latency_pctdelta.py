import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams["font.family"] = "monospace"
plt.rcParams["font.weight"] = "bold"
plt.rcParams["text.color"] = "black"

CODE = {
    'backprop': 'BP', 'bfs': 'BF', 'bplustree': 'BT', 'canneal': 'CN',
    'facesim': 'FS', 'freqmine': 'FM', 'leukocyte': 'LK', 'nn': 'NN',
    'nw': 'NW', 'particlefilter': 'PF', 'srad': 'SR',
}

data = {
    'backprop':       dict(baseline=43.265141, baseline_bypass=14.509348, boundnoc_delay=99.024626, boundnoc_bypass=26.079913),
    'bfs':            dict(baseline=42.817901, baseline_bypass=12.036606, boundnoc_delay=99.000000, boundnoc_bypass=21.324031),
    'bplustree':      dict(baseline=41.420779, baseline_bypass=11.737824, boundnoc_delay=99.000000, boundnoc_bypass=21.360701),
    'canneal':        dict(baseline=42.825368, baseline_bypass=12.078037, boundnoc_delay=99.000000, boundnoc_bypass=21.269413),
    'facesim':        dict(baseline=43.013498, baseline_bypass=11.997632, boundnoc_delay=99.000255, boundnoc_bypass=21.304340),
    'freqmine':       dict(baseline=43.024863, baseline_bypass=12.010308, boundnoc_delay=99.000000, boundnoc_bypass=21.311711),
    'leukocyte':      dict(baseline=34.488930, baseline_bypass=13.948733, boundnoc_delay=99.028925, boundnoc_bypass=26.058905),
    'nn':             dict(baseline=119.352429,baseline_bypass=69.816105, boundnoc_delay=159.378226,boundnoc_bypass=85.044564),
    'nw':             dict(baseline=42.974039, baseline_bypass=12.131012, boundnoc_delay=99.000000, boundnoc_bypass=41.415220),
    'particlefilter': dict(baseline=42.716863, baseline_bypass=11.971468, boundnoc_delay=99.000000, boundnoc_bypass=21.407892),
    'srad':           dict(baseline=41.119385, baseline_bypass=14.160235, boundnoc_delay=99.598046, boundnoc_bypass=25.721426),
}

pct = {}
for n, d in data.items():
    b = d['baseline']
    pct[n] = {
        'baseline_bypass': (d['baseline_bypass'] - b) / b * 100,
        'boundnoc_delay':  (d['boundnoc_delay']  - b) / b * 100,
        'boundnoc_bypass': (d['boundnoc_bypass'] - b) / b * 100,
    }

names_sorted = sorted(pct.keys(), key=lambda n: -pct[n]['boundnoc_delay'])
codes = [CODE[n] for n in names_sorted]

series = ['baseline_bypass', 'boundnoc_delay', 'boundnoc_bypass']
labels = ['BASELINE_BYPASS', 'BOUNDNOC_DELAY', 'BOUNDNOC_BYPASS']
fills = [WHITE if False else 'white', 'white', 'black']
hatches = ['///', 'xx', None]

fig, ax = plt.subplots(figsize=(12.4, 3.4), dpi=200)
x = np.arange(len(names_sorted))
n_series = len(series)
bar_w = 0.75 / n_series

for si, (key, lab, fill, hatch) in enumerate(zip(series, labels, fills, hatches)):
    offs = (si - (n_series-1)/2) * bar_w
    vals = [pct[n][key] for n in names_sorted]
    ax.bar(x + offs, vals, width=bar_w*0.92, color=fill, edgecolor='black',
           linewidth=1.4, hatch=hatch, zorder=3)

ax.axhline(0, color='black', linewidth=1.6, zorder=4)
for spine in ['top', 'right', 'left', 'bottom']:
    ax.spines[spine].set_visible(False)
ax.tick_params(axis='both', which='both', length=0)
ax.set_axisbelow(True)
ax.yaxis.grid(True, which='major', color='0.85', linewidth=1.1)
ax.xaxis.grid(False)

ax.set_xticks(x)
ax.set_xticklabels(codes, fontsize=17, fontweight='bold')
ax.set_ylabel('% vs BASELINE', fontsize=16, fontweight='bold')
ax.tick_params(axis='y', labelsize=14)
for lbl in ax.get_yticklabels():
    lbl.set_fontweight('bold')
ax.yaxis.set_major_formatter(lambda v, pos: f"{v:+.0f}%" if v != 0 else "0%")

handles = [plt.Rectangle((0,0),1,1, facecolor=f, edgecolor='black', hatch=h, linewidth=1.4)
           for f, h in zip(fills, hatches)]
ax.legend(handles, labels, loc='upper right', ncol=3, frameon=True, edgecolor='black',
          fontsize=13.5, handlelength=1.6, handleheight=1.4, columnspacing=1.2,
          bbox_to_anchor=(1.0, 1.13), prop={'weight':'bold','family':'monospace','size':13.5})

fig.tight_layout(pad=0.4)
out = "/tmp/claude-1880884998/-home-harpreetsc-noc/67f3bf0d-266c-499d-82f2-b60e4c6d1153/scratchpad/figures_mpl/boundnoc_latency_pctdelta_1B.png"
fig.savefig(out, facecolor='white', bbox_inches='tight')
plt.close(fig)
print("saved", out)
