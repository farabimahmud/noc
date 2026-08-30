# Experiment reproduction reference

This file documents exactly how every result under `results/` was produced,
for artifact-evaluation / reproducibility purposes. Verbatim commands for
every completed run are auto-extracted (not hand-transcribed) into
`EXPERIMENTS_COMMANDS.txt` alongside this file, from each run's own
`stdout.log` (gem5 prints its own invocation as `command line: ...` at
startup) — that file is the ground truth; this one is the readable guide to
it.

## Building

Two gem5 targets are used:

```
scons -j$(nproc) build/X86_MESI_Two_Level/gem5.opt   # real workloads (SE-mode)
scons -j$(nproc) build/Garnet_standalone/gem5.opt     # (unused for these
                                                       #  results; see below)
```

`X86_MESI_Two_Level` needs the pybind11/Python-3.12/GCC-13 compatibility diff
already committed to this branch (`git log` for the modernization commit) —
if starting from a clean gem5 checkout without that diff, it will not build
on a modern host. `Garnet_standalone` additionally needs the
`bug_fix/SConscript` fix in a separate commit on this branch (protocol-
specific `CoherenceRequestType`/`CoherenceResponseType` handling) — see that
commit's message for why. `Garnet_standalone` was built and smoke-tested this
session for a since-abandoned synthetic-traffic approach (see "Approaches
tried and abandoned" below) but produced none of the results in `results/`.

Guest benchmark binaries are built separately, outside this repo (source
lives in `~/benchmarks/` on the machine these results were produced on, not
under git) — see `patches/README.md` for the source locations, the musl
toolchain recipe, and the exact build command + input-data notes for every
one of the 16 benchmarks below, including facesim's four additional fixes.

## The four defense policies

Every real-workload benchmark below was run once per policy:

| Policy (dashboard name) | `--bypass=` value | Extra flags |
|---|---|---|
| BASELINE | `bypass_none` | — |
| BASELINE_BYPASS | `bypass_x` | `--max-hpc=5 --upper-limit=100` |
| BOUNDNOC_DELAY | `jitter_all` | `--upper-limit=100` |
| BOUNDNOC_BYPASS | `bypass_all_out` | `--max-hpc=5 --upper-limit=100` |

All four also share: `--num-cpus=64 --num-dirs=64 --network=garnet2.0
--topology=Mesh_XY --mesh-rows=8 --ruby --caches --l2cache --num-l2caches=64
--attack-enabled --attack-rate=1`, and (for the 15 real benchmarks + facesim,
i.e. every non-single-pair run) `--attack-node=<all 64 node IDs, comma-
separated> --destination-list=<same 64 IDs>` — every node attacks every
other node simultaneously; the network-interface stats layer buckets each
attacker's own closest/farthest destination automatically (see
`src/mem/ruby/network/garnet2.0/NetworkInterface.cc`). `--target-latency=20`
is set for benchmarks whose steady-state round-trip latency needed seeding
away from the default (see `EXPERIMENTS_COMMANDS.txt` for exactly which).

## Real-workload benchmarks (the 15 + facesim in the dashboard)

`--maxinsts` and the guest binary/args vary per benchmark — most run at
100,000,000 instructions; `lud` and `blackscholes` (excluded) needed only
10,000,000; `facesim` runs at 20,000,000 (see "facesim: thin-signal, not a
bug" in `patches/README.md` for why a larger budget isn't worth the
wall-clock cost). Guest binary/options per benchmark (identical across all
four policies for a given benchmark, only `--bypass`/its extra flags change):

| Benchmark | `--cmd` | `--options` |
|---|---|---|
| leukocyte | `.../leukocyte/OpenMP/leukocyte.out` | `20 64 .../testfile.avi` |
| heartwall | `.../heartwall/heartwall.out` | `.../test.avi 20 64` |
| lud | `.../lud/omp/lud_omp` | `-n 64 -s 4096` |
| srad | `.../srad/srad_v1/srad` | `100 0.5 502 458 64` |
| backprop | `.../backprop/backprop` | `65536` |
| nn | `.../nn/nn_bin` | `.../filelist_4_abs 5 30 90` |
| b+tree | `.../b+tree/b+tree.out` | `core 64 file .../mil.txt command .../command.txt` |
| canneal | `.../canneal/src/canneal` | `64 15000 2000 .../400000.nets 128` |
| particlefilter | `.../particlefilter/particle_filter` | `-x 128 -y 128 -z 10 -np 10000` |
| lavaMD | `.../lavaMD/lavaMD` | `-cores 64 -boxes1d 20` |
| nw | `.../nw/needle` | `4096 10 64` |
| fluidanimate | `.../fluidanimate/src/fluidanimate` | `64 5 .../in_100K.fluid <out>` |
| freqmine | `.../freqmine/src/freqmine` | `.../synth_trans.dat 82` |
| facesim | `.../facesim/Benchmarks/facesim/facesim` | `-timing -threads 64` (run from a cwd containing `Face_Data/`, see `patches/README.md`) |
| cfd | `.../cfd/euler3d_cpu` | `.../synth.domn` |
| bfs | `.../bfs/bfs` | `64 .../synth_graph.txt` |

Full absolute paths and exact flag ordering: `EXPERIMENTS_COMMANDS.txt`,
sections `<benchmark>-bypass_none` / `-baseline_bypass` / `-boundnoc_delay` /
`-boundnoc_bypass`.

## Excluded / could-not-run benchmarks

Documented in the dashboard itself (the "Not Included" section of the
published artifact, backed by `EXCLUDED_MANIFEST` in the dashboard's source)
with the reasoning for each: `hotspot3D`, `pathfinder`, `kmeans`,
`blackscholes` (ran but zero/near-zero signal), `dedup`, `mummergpu`
(could not run at all — `dedup`'s attempt is in `EXPERIMENTS_COMMANDS.txt`;
`mummergpu` was never attempted, no OpenMP variant exists), plus
`swaptions`/`bodytrack`/`ferret`/`vips`/`raytrace`/`x264` (never attempted,
see the manifest for why). `results/hotspot-test_bigger` and
`results/srad_v1-jitter_all-hpc5` are earlier exploratory/superseded runs,
kept on disk but not used anywhere in the dashboard — do not treat them as
current results.

## Single-attacker replication attempt (paper-exact methodology)

The original BoundNoC paper's `impact-on-latency` figure (FaceSim, closest
node-pair `(0,0)` vs farthest `(0,63)`) uses a single fixed attacker with two
fixed destination targets — narrower than the all-64-attackers-aggregate
approach used for every benchmark above. Two things learned attempting to
replicate it exactly:

1. **A literal `(0,0)` reading (attacker targeting its own node) always
   yields zero network samples**, on any benchmark — same-node accesses are
   served by the local cache hierarchy and never traverse Garnet. This is a
   structural property of the memory system, not fixable by choosing a
   richer workload. Use a genuinely adjacent-but-distinct node as the
   "closest" target instead (we used node 1, one hop from attacker node 0).
2. **Per-attacker sample density varies enormously by benchmark**, and
   restricting `--destination-list` to 2 entries (vs. all 64) shrinks the
   attack-taggable address space on top of the 1-of-64-attackers reduction —
   the combined effect is much larger than either alone. `facesim` yielded
   only 7 samples at `--maxinsts=20000000` this way (see
   `results/facesim-singlepair-bypass_none/`) — not usable. `leukocyte`
   (the richest aggregate dataset) yielded 1,975 samples at just
   `--maxinsts=10000000` (`results/leukocyte-singlepair-bypass_none/`, see
   its `stdout.log`/`stats.txt` before it was overwritten by the full-scale
   rerun below) — a working proof of concept.

Full four-policy single-attacker leukocyte runs (`--attack-node=0
--destination-list=1,63`, all other flags matching the standard leukocyte
row above) at the full `--maxinsts=100000000` budget: `results/leukocyte-
singlepair-<policy>/`, all four completed. 3,032 attack samples every run
(1,007 closest-dest, 2,025 farthest-dest, identical across policies since
it's the same workload/instruction budget). Closest/farthest means:

| Policy | closest mean | farthest mean | gap |
|---|---|---|---|
| BASELINE | 19.0 | 56.2 | 37.2 cycles (clearly separable) |
| BASELINE_BYPASS | 19.0 | 12.9 | 6.1 |
| BOUNDNOC_DELAY | 99.0 | 99.0 | 0.0, stdev 0.0 both sides (exact) |
| BOUNDNOC_BYPASS | 19.0 | 20.8 | 1.8 |

A materially cleaner replication of the paper's exact `(0,0)`/`(0,63)`-style
methodology than either the 64-attacker aggregate approach or facesim (which
tops out at 7 samples for this setup, see above) -- validates that the
single-attacker methodology is viable given a high-enough-density benchmark
and a genuinely adjacent (not self-targeting) "closest" node.
