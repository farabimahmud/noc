import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
import numpy as np

BOLD_STROKE = [pe.withStroke(linewidth=0.9, foreground='black')]

plt.rcParams["font.family"] = "monospace"
plt.rcParams["font.weight"] = "bold"
plt.rcParams["text.color"] = "black"

def get_last_value(path, prefix):
    last = None
    with open(path) as f:
        for line in f:
            if line.startswith(prefix):
                last = line
    if last is None:
        return None
    return float(last.split()[1])

# 8-benchmark subset (ML, PARSEC, Rodinia) chosen to include every weak/
# regressed boundnoc_bypass case (IC, KWS-4x8, KWS-8x8, nw) alongside
# already-strong cases (AD, canneal, facesim, leukocyte), to check whether
# boundnoc_low_up / boundnoc_low_closest fix the weak cases without
# regressing the strong ones.
benchmarks = {
    'IC':        'ic_multiinstance32',
    'KWS-4x8':   'kws_multiinstance32',
    'KWS-8x8':   'kws_multiinstance64',
    'AD':        'ad_multiinstance',
    'canneal':   'canneal-1B',
    'facesim':   'facesim-1B',
    'nw':        'nw-1B',
    'leukocyte': 'leukocyte-1B',
}

policies = ['baseline_bypass', 'boundnoc_bypass', 'boundnoc_low_up', 'boundnoc_low_closest']
suffix_for = {
    'baseline_bypass': 'baseline_bypass',
    'boundnoc_bypass': 'boundnoc_bypass',
    'boundnoc_low_up': 'boundnoc_low_up',
    'boundnoc_low_closest': 'boundnoc_low_closest',
}

pct = {}
for name, prefix in benchmarks.items():
    base = get_last_value(f"/home/harpreetsc/noc/results/{prefix}-bypass_none/stats.txt",
                           "system.ruby.network.attack_packet_latency::mean")
    pct[name] = {}
    for p in policies:
        val = get_last_value(f"/home/harpreetsc/noc/results/{prefix}-{suffix_for[p]}/stats.txt",
                              "system.ruby.network.attack_packet_latency::mean")
        pct[name][p] = (val - base) / base * 100

names = list(benchmarks.keys())

labels = ['BASELINE_BYPASS', 'BOUNDNOC_BYPASS', 'BOUNDNOC_LOW_UP', 'BOUNDNOC_RAND']
fills =   ['white', 'black', 'white', 'white']
hatches = [None,    None,    '///',   'xx']

fig, ax = plt.subplots(figsize=(15.0, 4.6), dpi=200)
x = np.arange(len(names))
n_series = len(policies)
bar_w = 0.8 / n_series

for si, (key, lab, fill, hatch) in enumerate(zip(policies, labels, fills, hatches)):
    offs = (si - (n_series-1)/2) * bar_w
    vals = [pct[n][key] for n in names]
    ax.bar(x + offs, vals, width=bar_w*0.92, color=fill, edgecolor='black',
           linewidth=1.4, hatch=hatch, zorder=3)

ax.axhline(0, color='black', linewidth=1.8, zorder=4)
for spine in ['top', 'right', 'left', 'bottom']:
    ax.spines[spine].set_visible(False)
ax.tick_params(axis='both', which='both', length=0)
ax.set_axisbelow(True)
ax.yaxis.grid(True, which='major', color='0.82', linewidth=1.1)
ax.xaxis.grid(False)

ax.set_xticks(x)
xtl = ax.set_xticklabels(names, fontsize=17, fontweight='bold')
for t in xtl:
    t.set_path_effects(BOLD_STROKE)

ylab = ax.set_ylabel('% vs BASELINE', fontsize=16, fontweight='bold')
ylab.set_path_effects(BOLD_STROKE)
ax.tick_params(axis='y', labelsize=14)
for lbl in ax.get_yticklabels():
    lbl.set_fontweight('bold')
    lbl.set_path_effects(BOLD_STROKE)
ax.yaxis.set_major_formatter(lambda v, pos: f"{v:+.0f}%" if v != 0 else "0%")

# vertical separators between category groups (ML | PARSEC | Rodinia)
for sep_x in [3.5, 5.5]:
    ax.axvline(sep_x, color='0.75', linewidth=1.2, linestyle=(0, (2, 2)), zorder=1)

handles = [plt.Rectangle((0,0),1,1, facecolor=f, edgecolor='black', hatch=h, linewidth=1.4)
           for f, h in zip(fills, hatches)]
leg = ax.legend(handles, labels, loc='upper center', ncol=4, frameon=True, edgecolor='black',
          fontsize=13, handlelength=1.8, handleheight=1.3, columnspacing=1.2,
          bbox_to_anchor=(0.5, 1.18), prop={'weight':'bold','family':'monospace','size':13})
leg.set_zorder(10)
leg.get_frame().set_facecolor('white')
leg.get_frame().set_alpha(1.0)
leg.get_frame().set_linewidth(1.4)
for t in leg.get_texts():
    t.set_path_effects(BOLD_STROKE)

fig.tight_layout(rect=[0, 0, 1, 0.94])
out = "/home/harpreetsc/noc/paper/figs_new/boundnoc_variable_workload_pctdelta.png"
fig.savefig(out, facecolor='white', bbox_inches='tight')
plt.close(fig)
print("saved", out)
