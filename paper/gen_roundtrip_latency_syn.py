import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
import json

BOLD_STROKE = [pe.withStroke(linewidth=1.1, foreground='black')]

plt.rcParams["font.family"] = "monospace"
plt.rcParams["font.weight"] = "bold"
plt.rcParams["text.color"] = "black"

with open("/home/harpreetsc/noc/paper/synth_sweeps/roundtrip_sweep.json") as f:
    data = json.load(f)

policies = ['bypass_none', 'jitter_all', 'bypass_all_out']
titles = ['(a) BASELINE', '(b) BOUNDNOC_DELAY', '(c) BOUNDNOC_BYPASS']

fig, axes = plt.subplots(1, 3, figsize=(15.0, 4.4), dpi=200, sharey=True)

for ax, p, title in zip(axes, policies, titles):
    pts = data[p]
    rates = [d['rate'] for d in pts]
    closest = [d['closest'] for d in pts]
    farthest = [d['farthest'] for d in pts]

    ax.plot(rates, closest, color='black', linewidth=2.6, linestyle='-',
            marker='o', markersize=7, markerfacecolor='black', markeredgecolor='black',
            label='closest-dest')
    ax.plot(rates, farthest, color='black', linewidth=2.4, linestyle=(0, (3, 2)),
            marker='o', markersize=7, markerfacecolor='black', markeredgecolor='black',
            label='farthest-dest')

    for spine in ['top', 'right']:
        ax.spines[spine].set_visible(False)
    ax.spines['left'].set_linewidth(2.0)
    ax.spines['bottom'].set_linewidth(2.0)
    ax.tick_params(axis='both', which='both', length=0)
    ax.tick_params(axis='x', pad=10)
    ax.yaxis.grid(True, which='major', color='0.65', linewidth=1.2)
    ax.set_axisbelow(True)

    xt = [0.00, 0.01, 0.02]
    ax.set_xticks(xt)
    xtl = ax.set_xticklabels(['0.00', '0.01', '0.02'], fontsize=22, fontweight='bold')
    for t in xtl:
        t.set_path_effects(BOLD_STROKE)
    ax.set_xlim(min(rates) - 0.001, max(rates) + 0.001)
    ax.set_ylim(0, 108)
    xlab = ax.set_xlabel('injection rate', fontsize=24, fontweight='bold')
    xlab.set_path_effects(BOLD_STROKE)
    ax.set_title('')
    ttl = ax.text(0.5, 1.04, title, transform=ax.transAxes, ha='center', va='bottom',
                  fontsize=23, fontweight='bold')
    ttl.set_path_effects(BOLD_STROKE)

axes[0].set_yticks([0, 20, 40, 60, 80, 100])
ytl = axes[0].set_yticklabels(['0', '20', '40', '60', '80', '100'], fontsize=23, fontweight='bold')
for t in ytl:
    t.set_path_effects(BOLD_STROKE)
ylab = axes[0].set_ylabel('cycles', fontsize=24, fontweight='bold')
ylab.set_path_effects(BOLD_STROKE)

handles = [plt.Line2D([0], [0], color='black', linewidth=2.6, linestyle='-',
                       marker='o', markersize=7, markerfacecolor='black'),
           plt.Line2D([0], [0], color='black', linewidth=2.4, linestyle=(0, (3, 2)),
                      marker='o', markersize=7, markerfacecolor='black')]
leg = fig.legend(handles, ['closest-dest', 'farthest-dest'], loc='upper center', ncol=2,
                  frameon=True, edgecolor='black', fontsize=25, handlelength=2.6,
                  bbox_to_anchor=(0.5, 1.1), prop={'weight': 'bold', 'family': 'monospace', 'size': 25})
leg.get_frame().set_facecolor('white')
leg.get_frame().set_alpha(1.0)
leg.get_frame().set_linewidth(1.6)
for t in leg.get_texts():
    t.set_path_effects(BOLD_STROKE)

fig.tight_layout(rect=[0, 0, 1, 0.98])
fig.subplots_adjust(wspace=0.24, top=0.8)
out = "/home/harpreetsc/noc/paper/figs_new/boundnoc_roundtrip_latency_syn.png"
fig.savefig(out, facecolor='white', bbox_inches='tight')
plt.close(fig)
print("saved", out)
