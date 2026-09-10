import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

plt.rcParams["font.family"] = "monospace"
plt.rcParams["font.weight"] = "bold"
plt.rcParams["text.color"] = "black"

# all 6 confirmed-completed variants: AD normal/anomaly input, KWS 8x8/4x8 mesh,
# IC and VWW (both 4x8 mesh / 32-instance, natural-completion runs)
data = {
    'AD-N':    dict(baseline=36.623973, baseline_bypass=13.483637, boundnoc_delay=99.000000, boundnoc_bypass=25.451258),
    'AD-A':    dict(baseline=36.742733, baseline_bypass=13.466403, boundnoc_delay=99.000000, boundnoc_bypass=26.205279),
    'KWS-8x8': dict(baseline=37.979236, baseline_bypass=14.603282, boundnoc_delay=99.080478, boundnoc_bypass=41.428511),
    'KWS-4x8': dict(baseline=32.449025, baseline_bypass=12.706151, boundnoc_delay=99.000000, boundnoc_bypass=33.769569),
    'IC':      dict(baseline=32.065694, baseline_bypass=12.500662, boundnoc_delay=99.000000, boundnoc_bypass=29.506422),
    'VWW':     dict(baseline=31.356945, baseline_bypass=12.366764, boundnoc_delay=99.000000, boundnoc_bypass=20.634162),
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
codes = names_sorted  # already short

series = ['baseline_bypass', 'boundnoc_delay', 'boundnoc_bypass']
labels = ['BASELINE_BYPASS', 'BOUNDNOC_DELAY', 'BOUNDNOC_BYPASS']
fills = ['white', 'white', 'black']
hatches = ['///', 'xx', None]

fig, ax = plt.subplots(figsize=(8.4, 3.6), dpi=200)
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
ax.set_ylabel('% vs BASELINE', fontsize=15, fontweight='bold')
ax.tick_params(axis='y', labelsize=13)
for lbl in ax.get_yticklabels():
    lbl.set_fontweight('bold')
ax.yaxis.set_major_formatter(lambda v, pos: f"{v:+.0f}%" if v != 0 else "0%")

handles = [plt.Rectangle((0,0),1,1, facecolor=f, edgecolor='black', hatch=h, linewidth=1.4)
           for f, h in zip(fills, hatches)]
leg = ax.legend(handles, labels, loc='upper right', ncol=3, frameon=True, edgecolor='black',
          fontsize=11, handlelength=1.6, handleheight=1.3, columnspacing=1.0,
          bbox_to_anchor=(1.0, 1.14), prop={'weight':'bold','family':'monospace','size':11})
leg.set_zorder(10)
leg.get_frame().set_facecolor('white')
leg.get_frame().set_alpha(1.0)

fig.tight_layout(pad=0.4)
out = "/home/harpreetsc/noc/paper/figs_new/boundnoc_ml_latency_pctdelta.png"
fig.savefig(out, facecolor='white', bbox_inches='tight')
plt.close(fig)
print("saved", out)
