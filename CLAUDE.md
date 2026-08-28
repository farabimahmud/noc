# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

This is a fork of the **gem5** computer-architecture simulator (http://www.gem5.org), customized for research
into **timing-channel attacks and defenses in Network-on-Chip (NoC) interconnects**. The upstream gem5 code is
largely untouched; the interesting work lives in `src/mem/ruby/network/garnet2.0/` (the Garnet2.0 NoC model),
plus a set of attack proof-of-concept programs, run scripts, and result-processing scripts at the repo root.

Key custom mechanisms added on top of stock Garnet2.0 (search these files first when working on attack/defense
logic):
- `src/mem/ruby/network/garnet2.0/NetworkInterface.{hh,cc}` — flit jitter injection, RTT-based latency tracking,
  attack-flit handling (`sendAttackflit`), moving-average/target-latency defense logic.
- `src/mem/ruby/network/garnet2.0/GarnetNetwork.{hh,cc}` — global attack/defense knobs (`jitter_all`, `optimized`,
  `bypass_all`, `bypass_x`, `upper_limit`, jitter/latency stats counters).
- `src/mem/ruby/network/garnet2.0/flit.{hh,cc}` — per-flit `jitter`/attack-flit metadata.
- `configs/network/Network.py` — command-line options for the above: `--bypass`, `--bypass_enabled`,
  `--flit_jitter_threshold`, `--optimization_rate`, `--max-hpc`, `--delta-s`, `--lower-limit`, `--upper-limit`,
  `--target-latency`, plus attack-injection options consumed by `configs/example/se.py`
  (`--attack-enabled`, `--attack-rate`, `--attack-node`, `--destination-list`).
- Root-level attack PoCs: `attack_code/` (cache/NoC covert-channel sender-receiver, `lat_test.cpp`,
  `plot_heatmap.py`), `simple-attack-func.cc`, `acc_lat_test.c` — these are guest workloads compiled and run
  inside simulated (or native, for calibration) systems to exercise the covert channel.
- `runx86`, `500.slurm.jobs`, `rundebug.sh`, `run_rodinia.sh`, `generate_parsec_scripts.py`,
  `parsec_script_generator/`, `scripts_x86/`, `create_micro_results_parsec.py`,
  `create_isca_results_rodinia.py`, `parse_results.py`, `draw_plots.py` — experiment orchestration
  (launching many gem5 runs, one per benchmark/attack config, typically on a Slurm cluster) and post-processing
  of stats into plots/tables. These embed absolute paths from the original authors' machines (e.g.
  `/home/grads/f/farabi/noc/...`) — treat them as templates to adapt, not as portable scripts.

Do not "clean up" or refactor the upstream gem5 portions of the tree unless asked — the goal here is research
experiments, and touching unrelated subsystems makes it hard to diff against upstream gem5 later.

## Build

gem5 is built with **SCons**, one binary per ISA/coherence-protocol combination. Build targets are
`build/<OPT>/gem5.<variant>` where `<OPT>` is one of the directory names under `build_opts/` (e.g. `X86`,
`X86_MESI_Two_Level`, `Garnet_standalone`, `NULL`, `ARM`, ...) and `<variant>` is `debug`, `opt`, `fast`, or
`prof`.

```shell
# Standard full-system/SE X86 build with the MESI Two-Level Ruby protocol
scons -j$(nproc) build/X86_MESI_Two_Level/gem5.opt

# Standalone Garnet network simulator (no CPU model, synthetic traffic only) — the
# fastest way to iterate on router/NI/topology logic in src/mem/ruby/network/garnet2.0
scons -j$(nproc) build/Garnet_standalone/gem5.opt

# Debug build (assertions on, no optimization) of the same target
scons -j$(nproc) build/X86_MESI_Two_Level/gem5.debug
```

`build_opts/<NAME>` files define `TARGET_ISA`, `CPU_MODELS`, and `PROTOCOL` for that build directory; check the
file before assuming what a given `build/<NAME>` target contains. Build products are cached in `build/`
(gitignored) — expect the first build of a target to take a long time; incremental builds are fast.

## Running simulations

Two entry points matter most for this fork's work:

- `configs/example/se.py` — full-system-less (syscall-emulation) runs with real CPU models + Ruby/Garnet memory
  system. This is what the attack experiments use (see `runx86` for a real example invocation with
  `--network=garnet2.0 --topology=Mesh_XY --ruby --caches --l2cache`, `--bypass=jitter_all`,
  `--attack-enabled`, `--attack-node=...`, `--target-latency=...`).
- `configs/example/garnet_synth_traffic.py` — Garnet-only synthetic traffic generator, no CPUs; use with the
  `Garnet_standalone` build for pure network-microarchitecture experiments.

Ruby/Garnet-specific CLI options live in `configs/network/Network.py` (`common.Options` supplies the generic
gem5 options); grep there before adding a new flag — most attack/defense knobs already have one.

## Testing

Full details are in `TESTING.md` — read it for anything non-trivial. Quick reference:

```shell
# Unit tests (Google Test, built via SCons)
scons build/NULL/unittests.opt
scons build/NULL/base/bitunion.test.opt && ./build/NULL/base/bitunion.test.opt
./build/NULL/base/bitunion.test.opt --gtest_filter=BitUnionData.NormalBitfield

# System-level regression tests (tests/ directory, "Whimsy" framework in ext/testlib)
cd tests
./main.py run                                  # quick tests for X86, ARM, RISC-V
./main.py run --isa X86 --variant opt
./main.py list --all-tags                      # see available isa/length/variant tags
./main.py rerun                                 # rerun only tests that failed last time
./main.py run --skip-build -t 3                 # parallel, 3 suites at once
```

There is no test coverage for the attack/defense additions in `garnet2.0` or the root-level attack scripts —
validate changes there by running an actual simulation (e.g. via `runx86`-style invocation) and inspecting the
stats/log output, not via `tests/`.

## Architecture orientation (stock gem5, for context)

- `src/` — simulator source, organized by subsystem (`arch/` per-ISA decode/exec, `cpu/` CPU models, `mem/`
  memory system including `mem/ruby/` for the Ruby cache-coherence + interconnect models, `sim/` core
  event-driven simulation loop, `dev/` device models, `python/` the Python config/SimObject binding layer).
- `configs/` — Python simulation-configuration scripts (what you point `gem5.opt` at). `configs/common/` has
  shared option parsing (`Options.py`) and system-construction helpers used by both `se.py` and `fs.py`.
- SimObjects are declared via paired `.py`/`.cc`/`.hh` files (e.g. `GarnetNetwork.py` + `GarnetNetwork.hh/cc`);
  the `.py` file defines the parameters exposed to config scripts, the C++ files implement behavior. When
  adding a parameter to a Garnet/Ruby object, it must be added in both places.
- `ext/` — bundled third-party dependencies (googletest, dsent, mcpat, systemc, pybind11, ...), not
  gem5-authored code.
- `util/style` / `util/style.py` — gem5's C++/Python style checker, used by upstream's pre-commit hook; this
  fork does not appear to enforce it in CI, but match surrounding style when editing existing files regardless.

This fork tracks a gem5 version that still uses the legacy `garnet2.0` directory name (upstream later renamed
this to `garnet`) — don't be surprised the network code isn't at `src/mem/ruby/network/garnet`.
