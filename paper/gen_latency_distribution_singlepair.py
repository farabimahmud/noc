import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe
import numpy as np
import re

BOLD_STROKE = [pe.withStroke(linewidth=1.1, foreground='black')]

plt.rcParams["font.family"] = "monospace"
plt.rcParams["font.weight"] = "bold"
plt.rcParams["text.color"] = "black"

def parse_hist_line(line):
    # extract the 2nd number (percent) from each pipe-separated segment
    segs = line.split('|')[1:]  # first split before '|' is the stat name/prefix
    pcts = []
    for seg in segs:
        nums = re.findall(r'[\d.]+%', seg)
        if len(nums) >= 1:
            pcts.append(float(nums[0].rstrip('%')))
    return pcts

def get_last_line(path, prefix):
    last = None
    with open(path) as f:
        for line in f:
            if line.startswith(prefix):
                last = line
    return last

policies = ['bypass_none', 'baseline_bypass', 'boundnoc_delay', 'boundnoc_bypass']
titles = ['(a) BASELINE', '(b) BASELINE_BYPASS', '(c) BOUNDNOC_DELAY', '(d) BOUNDNOC_BYPASS']

data = {}
for p in policies:
    path = f"/home/harpreetsc/noc/results/leukocyte-singlepair-{p}/stats.txt"
    closest_line = get_last_line(path, "system.ruby.network.closest_dest_attack_packet_latency ")
    farthest_line = get_last_line(path, "system.ruby.network.farthest_dest_attack_packet_latency ")
    data[p] = {
        'closest': parse_hist_line(closest_line),
        'farthest': parse_hist_line(farthest_line),
    }

# bucket edges: 0,5,...,100 (20 buckets) + overflow -> 21 bins, x-tick positions at bucket starts
n_bins = len(data['bypass_none']['closest'])
bucket_size = 5
edges = np.arange(0, bucket_size*(n_bins), bucket_size)  # left edges, last one = overflow marker position

fig, axes = plt.subplots(1, 4, figsize=(19.0, 4.2), dpi=200, sharey=True)

for ax, p, title in zip(axes, policies, titles):
    closest = data[p]['closest']
    farthest = data[p]['farthest']
    # step plot: repeat each value for [edge, edge+bucket_size)
    ax.step(edges, closest, where='post', color='black', linewidth=2.6, linestyle='-', label='closest-dest')
    ax.step(edges, farthest, where='post', color='black', linewidth=2.4, linestyle=(0, (3,2)), label='farthest-dest')

    for spine in ['top', 'right']:
        ax.spines[spine].set_visible(False)
    ax.spines['left'].set_linewidth(2.0)
    ax.spines['bottom'].set_linewidth(2.0)
    ax.tick_params(axis='both', which='both', length=0)
    ax.yaxis.grid(True, which='major', color='0.65', linewidth=1.2)
    ax.set_axisbelow(True)

    xt = [0, 50, 100, 165]
    ax.set_xticks(xt)
    xtl = ax.set_xticklabels(['0','50','100','>105'], fontsize=22, fontweight='bold')
    for t in xtl:
        t.set_path_effects(BOLD_STROKE)
    ax.set_xlim(0, 165)
    ax.set_ylim(0, 108)
    xlab = ax.set_xlabel('cycles', fontsize=24, fontweight='bold')
    xlab.set_path_effects(BOLD_STROKE)
    ax.set_title('')
    ttl = ax.text(0.5, -0.42, title, transform=ax.transAxes, ha='center', va='top',
            fontsize=25, fontweight='bold')
    ttl.set_path_effects(BOLD_STROKE)

axes[0].set_yticks([0,20,40,60,80,100])
ytl = axes[0].set_yticklabels(['0%','20%','40%','60%','80%','100%'], fontsize=23, fontweight='bold')
for t in ytl:
    t.set_path_effects(BOLD_STROKE)
ylab = axes[0].set_ylabel('% of samples', fontsize=24, fontweight='bold')
ylab.set_path_effects(BOLD_STROKE)

handles = [plt.Line2D([0],[0], color='black', linewidth=2.6, linestyle='-'),
           plt.Line2D([0],[0], color='black', linewidth=2.4, linestyle=(0,(3,2)))]
leg = fig.legend(handles, ['closest-dest', 'farthest-dest'], loc='upper center', ncol=2,
           frameon=True, edgecolor='black', fontsize=25, handlelength=2.6,
           bbox_to_anchor=(0.5, 1.18), prop={'weight':'bold','family':'monospace','size':25})
leg.get_frame().set_facecolor('white')
leg.get_frame().set_alpha(1.0)
leg.get_frame().set_linewidth(1.6)
for t in leg.get_texts():
    t.set_path_effects(BOLD_STROKE)

fig.tight_layout(rect=[0, 0.07, 1, 1])
fig.subplots_adjust(wspace=0.24)
out = "/home/harpreetsc/noc/paper/figs_new/boundnoc_latency_distribution_singlepair.png"
fig.savefig(out, facecolor='white', bbox_inches='tight')
plt.close(fig)
print("saved", out)
