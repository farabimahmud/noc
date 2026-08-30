# Guest-benchmark patches and build recipes

Benchmark source lives outside this repo (`~/benchmarks/` on the machine these
results were produced on) and is not under git, so fixes made there can't be
tracked as part of this repo's own history. This directory holds any patches
against that external source, plus the exact build recipe needed to reproduce
each guest binary. See `EXPERIMENTS.md` at the repo root for the exact gem5
invocation used for every benchmark x policy run.

## Why guest binaries are built with musl, not glibc

Running real SE-mode (syscall-emulation) benchmarks under this gem5 fork on a
modern host (Ubuntu 24.04, glibc 2.39) hits three back-to-back ABI mismatches:
1. the `rseq` syscall (334) is unimplemented in this gem5 fork -> fatal
2. `libc.so.6` requires a CPU-ISA level gem5's simulated CPUID doesn't report
3. a TLS/FS-base setup crash before `main()`, even under static glibc linking

All three disappear when the guest binary is built with musl instead, static
and non-PIE:

```
MUSL_CXX=~/opt/x86_64-linux-musl-native/bin/x86_64-linux-musl-g++
# CXXFLAGS: -static -no-pie   (and whatever the benchmark needs on top)
```

The toolchain is a standalone download from musl.cc (no root required); reuse
whatever copy already exists on the machine rather than re-downloading. If
setting up fresh: download the `x86_64-linux-musl-native` toolchain tarball
from `musl.cc`, extract anywhere (e.g. `~/opt/`), and use
`<extract-dir>/bin/x86_64-linux-musl-gcc` / `-g++` in place of the host
compiler for every guest binary below (including facesim).

## Benchmark source

- **Rodinia 3.0** (OpenMP variants, `~/benchmarks/rodinia_3.0/openmp/`):
  cloned from the `yuhc/gpu-rodinia` GitHub mirror. That mirror's own `data/`
  directory is empty upstream (just a `.gitkeep`) for several benchmarks --
  where noted below, the input file was synthetically generated instead of
  using Rodinia's original dataset (fine for this project's purposes, since
  only the network timing side-channel is being measured, not solution
  correctness).
- **PARSEC 3.0** (`~/benchmarks/parsec-3.0/`): cloned from the
  `cirosantilli/parsec-benchmark` GitHub mirror, tag `3.0` (same source used
  for facesim above). Its release assets include prebuilt input tarballs
  (`parsec-3.0-input-sim.tar.gz`, ~490MB for every benchmark) -- extract only
  the specific benchmark's `inputs/` subdirectory needed, not the whole
  archive.

None of the 15 benchmarks below use PARSEC's `parsecmgmt` build harness or
`m4`-macro-expanded portability wrappers (that pattern applies to some other
PARSEC apps not used here, e.g. `blackscholes`) -- every one is built by
invoking `make`/the compiler directly against the musl toolchain. Where a
benchmark's own Makefile hardcodes `gcc`/`g++` in its recipes rather than
using a `$(CC)`/`$(CXX)` variable, the command below is a direct compiler
invocation (bypassing `make`) rather than a `make CC=...` override, since the
latter would silently do nothing for those Makefiles.

**Two data-provenance caveats**, flagged rather than guessed at: the exact
source of `heartwall`'s `test.avi`, `leukocyte`'s `testfile.avi`, and
`b+tree`'s `mil.txt` isn't captured in this project's own notes (they exist
on the machine these results were produced on but predate the record-keeping
below) -- likely Rodinia's separate official data release
(`rodinia.cs.virginia.edu` / the `rodinia_3.1` data tarball) rather than the
source-only GitHub mirror, but not verified. If reproducing from scratch and
these aren't available, regenerate synthetic equivalents following the same
approach used for `cfd`/`bfs`/`hotspot` below (matching each program's own
input-parsing format).

## facesim (PARSEC)

Source: `github.com/cirosantilli/parsec-benchmark`, tag `3.0`,
`pkgs/apps/facesim/src/`. This is a PhysBAM-based physics simulation --
substantially bigger and older (~2008-era template-heavy C++) than the PARSEC
kernels (canneal, freqmine, fluidanimate) built elsewhere in this project, and
needed four distinct fixes to build and run correctly under gem5 SE-mode.

**1. Apply `facesim-gem5-se-mode.patch`** (in this directory) -- fixes a
`system("mkdir -p ...")` call that gem5 SE-mode can't emulate (no real
fork()+exec() support). See the patch file's own header for the full
root-cause explanation.

**2. Set `PHYSBAM` to the source directory itself.**
`Public_Library/Makefile.common` does `cd $(PHYSBAM)/Public_Library && make
...`. Upstream PARSEC's own build harness (`parsecmgmt`) normally sets this to
a *copied* build directory (`build_env` in `parsec/gcc-pthreads.bldconf`) --
since we build directly from the source tree without that harness, point it
there instead:

```
cd parsec-3.0/pkgs/apps/facesim/src
export PHYSBAM=$(pwd)
```

**3. Build with `-fpermissive`, `-O0`, and `-DENABLE_PTHREADS`:**

```
make version=pthreads \
  CXX="$MUSL_CXX" \
  CXXFLAGS="-fexceptions -fpermissive -static -no-pie -O0 -g -DENABLE_PTHREADS" \
  LDFLAGS="-static -no-pie"
```

- `-fpermissive`: the codebase relies on implicit two-phase-lookup relaxation
  for calling a dependent template base class's member without `this->` --
  legal under older GCC, a hard error under GCC 11+ without this flag.
- `-O0`: **required, not just a nicety.** At `-O2` the binary segfaults inside
  `PARSE_ARGS::Add_Integer_Argument` / `LIST_ARRAY::Ensure_Enough_Space`,
  before any real simulation work runs -- `-O2` exposes latent undefined
  behavior in this 2008-era array-growth code that happens not to manifest at
  `-O0`. Confirmed by isolating the flag: identical source, only the
  optimization level differs.
- `-DENABLE_PTHREADS`: `version=pthreads` alone does *not* reliably propagate
  this define -- the top-level Makefile's `CXXFLAGS += -DENABLE_PTHREADS`
  can be silently dropped when `CXXFLAGS` is also set on the `make` command
  line (GNU Make variable-origin precedence). Pass it explicitly. Without it,
  the binary builds and links fine but refuses to run with more than 1 thread
  ("Error: Number of threads cannot be greater than 1 for serial runs"), and
  the *library* (`Thread_Utilities/THREAD_POOL.cpp` etc.) must be rebuilt
  with the flag too, not just the driver -- a partial rebuild after adding
  the flag late will still fail the same way. Do a clean rebuild
  (`find . -name '*.o' -delete && rm -f lib/libPhysBAM.a`) whenever this flag
  changes.
- `LDFLAGS="-static -no-pie"` **must be passed separately from `CXXFLAGS`.**
  `Public_Library/Makefile.common`'s final link rule does not forward
  `CXXFLAGS` to the link step, only `LDFLAGS` (via `LINK_FLAGS += ... $(LDFLAGS)
  $(LIBS)`) -- passing `-static -no-pie` only in `CXXFLAGS` produces a
  binary that compiles fine but links dynamically against musl's loader.

**4. Input data**: `simsmall` size, from the `parsec-3.0-input-sim.tar.gz`
release asset (`github.com/cirosantilli/parsec-benchmark` releases, tag
`3.0`) -- extract only `pkgs/apps/facesim/inputs/input_simsmall.tar`, then
extract *that* tar into a working "run" directory (e.g.
`pkgs/apps/facesim/run/`); it expands into a `Face_Data/` tree that facesim
expects to find via relative path in its current working directory at
runtime. gem5 SE-mode's simulated process inherits gem5's own host cwd
(`process.cwd = os.getcwd()` in `configs/example/se.py`), so gem5 itself must
be launched with that "run" directory as its cwd -- see `EXPERIMENTS.md` for
the exact invocation.

The guest program also picks a pre-partitioned mesh file matching the thread
count automatically (`face_simulation_<N>.tet` for `-threads N`); the
`simsmall` input ships partitions for `N` in `{1,2,3,4,6,8,16,32,64,128}`. We
run at `-threads 64` to match the 64-node mesh, which the `simsmall` tarball
covers (`face_simulation_64.tet`).

## The other 15 benchmarks

All built the same way: musl toolchain, `-static -no-pie`, plus `-fopenmp`
for the OpenMP ones. Commands below bypass `make` for benchmarks whose
Makefile hardcodes the host compiler (noted per-benchmark); for the rest,
`make <VAR>=<musl-compiler> ...` works directly. `$MUSL_CC` /
`$MUSL_CXX` below stand for
`~/opt/x86_64-linux-musl-native/bin/x86_64-linux-musl-{gcc,g++}`.

### Rodinia (openmp/)

**leukocyte** -- has a nested dependency, the Meschach matrix library, built
via its own `configure`/`make`. Build that first with the musl toolchain so
the final static link is ABI-consistent, then the main binary (its Makefile
uses `$(CC)`, override works):
```
cd openmp/leukocyte/meschach_lib
CC=$MUSL_CC ./configure --with-all && make all && make clean
cd ../OpenMP
make CC=$MUSL_CC CC_FLAGS="-g -O3 -Wall -fopenmp -I../meschach_lib -static -no-pie"
```
Input: `testfile.avi` (see data-provenance caveat above), args
`20 64 <path-to-testfile.avi>`.

**heartwall** -- Makefile hardcodes `gcc`; also has a nested `AVI/` static
lib with its own Makefile (uses `$(CC)`, override works there). Build AVI
first, then compile+link main directly:
```
cd openmp/heartwall/AVI && make CC=$MUSL_CC
cd ..
$MUSL_CC -DOUTPUT main.c -I./AVI -c -O3 -fopenmp -static -no-pie
$MUSL_CC main.o ./AVI/avilib.o ./AVI/avimod.o -lm -fopenmp -static -no-pie -o heartwall.out
```
Input: `test.avi` (see data-provenance caveat above), args
`<path-to-test.avi> 20 64`.

**lud** -- Makefile uses `$(CC)`/`$(CXX)`, override works:
```
cd openmp/lud/omp
make CC=$MUSL_CC CXX=$MUSL_CXX COMMON_CFLAGS="-fopenmp -static -no-pie" COMMON_LDFLAGS="-fopenmp -static -no-pie"
```
Self-contained (`-s <size>` generates a synthetic matrix internally, `-n
<threads>` sets thread count) -- no input file. GOTCHA: matrix size must be
a multiple of the internal block size (`BS=16` in `lud.c`) or it segfaults;
`4096` works, `1000` does not. Args used: `-n 64 -s 4096`. By far the most
network-heavy workload in this set -- ran at `--maxinsts=10000000`, not
100M, since even 10M produces more attack samples than any other benchmark
gets at 100M (see `EXPERIMENTS_COMMANDS.txt`).

**srad** -- Makefile (`openmp/srad/srad_v1/makefile`) hardcodes `gcc`:
```
cd openmp/srad/srad_v1
$MUSL_CC main.c -c -O3 -fopenmp -static -no-pie
$MUSL_CC main.o -lm -fopenmp -static -no-pie -o srad
```
Self-contained (`100 0.5 502 458 64` = iterations, lambda, rows, cols,
threads via CLI) -- no input file.

**backprop** -- Makefile uses `$(CC)`, override works:
```
cd openmp/backprop
make CC=$MUSL_CC CC_FLAGS="-g -fopenmp -O2 -static -no-pie"
```
Self-contained (`65536` = layer size via CLI, synthetic training data
generated internally). GOTCHA: takes no thread-count CLI argument -- relies
on OpenMP's default, which must be forced with an `OMP_NUM_THREADS=64`
environment variable passed to gem5 (`--env=<path-to-env-file>` in
`se.py`), or it under-parallelizes.

**nn** (nearest-neighbor) -- Makefile uses `$(CC)`, override works. Needs a
synthetic dataset generated first with `hurricane_gen.c`, built with the
**host's normal gcc** (native one-shot data-prep tool, never runs under
gem5, no musl needed):
```
cd rodinia_3.0/data/nn
gcc -O3 -Wall -o hurricane_gen hurricane_gen.c -lm
mkdir -p data && ./hurricane_gen 42760 4    # writes data/cane4_{0..3}.db
cd ../../openmp/nn
make CC=$MUSL_CC CFLAGS="-lm -fopenmp -Wall -static -no-pie"
```
Args: `<absolute-path-to-filelist_4> 5 30 90` (`filelist_4` must list
absolute paths to the generated `cane4_*.db` files -- results dir uses key
`nn2` to avoid confusion with the unrelated `nw` benchmark). No CLI
thread-count arg -- needs `OMP_NUM_THREADS` via `--env` like backprop, and
the env value must match `--num-cpus` exactly (gem5 SE-mode can't multiplex
more OS threads than simulated cores).

**b+tree** -- Makefile uses `$(C_C)`, override works:
```
cd openmp/b+tree
make C_C=$MUSL_CC OMP_FLAG="-fopenmp -static -no-pie"
```
Input: `mil.txt` (data file, see provenance caveat above) + `command.txt`
(single line, `k60000` -- a find-key command). Args:
`core 64 file <path-to-mil.txt> command <path-to-command.txt>`.

**particlefilter** -- Makefile hardcodes `gcc`:
```
cd openmp/particlefilter
$MUSL_CC -O3 -ffast-math -fopenmp ex_particle_OPENMP_seq.c -o particle_filter -lm -static -no-pie
```
Self-contained (`-x 128 -y 128 -z 10 -np 10000` = frame width/height, frame
count, particle count, all via CLI synthetic generation) -- no input file,
no `--env` needed (plain `#pragma omp parallel for`, no explicit
thread-count argument, works fine without one).

**lavaMD** -- Makefile uses `$(C_C)`, override works:
```
cd openmp/lavaMD
make C_C=$MUSL_CC OMP_FLAG="-fopenmp -static -no-pie"
```
GOTCHA: `util/timer/timer.c` is missing `#include <sys/time.h>` in this
source tree -- fix by adding `-include sys/time.h` to the compile line
rather than patching the file (e.g. append to `CC_FLAGS`/the `main.o` rule).
Note upstream's own Makefile deliberately compiles `main.c` *without*
`-fopenmp` (only `kernel_cpu.c` gets it) -- keep that asymmetry. Self-
contained (`-cores 64 -boxes1d 20` sets threads and generates the
neighbor-box grid internally) -- no input file.

**nw** (Needleman-Wunsch) -- Makefile uses `$(CC)`, override works:
```
cd openmp/nw
make CC=$MUSL_CXX CC_FLAGS="-g -O3 -fopenmp -static -no-pie"
```
Self-contained (`4096 10 64` = matrix dimension, penalty, thread count, all
via CLI, synthetic scoring matrix generated internally) -- no input file,
no `--env` needed (explicit thread-count CLI arg).

**cfd** (`euler3d_cpu`) -- Makefile hardcodes `g++`, and thread count is
compile-time (`-Dblock_length=N`):
```
cd openmp/cfd
$MUSL_CXX -O3 -Dblock_length=64 -fopenmp euler3d_cpu.cpp -o euler3d_cpu -static -no-pie
```
Needs an external mesh file; the original Rodinia dataset
(`fvcorr.domn.193K`) isn't available from the source-only mirror, so a
synthetic one was generated instead (`synth.domn`, 50,000 elements) matching
the exact token format `euler3d_cpu.cpp`'s `main()` reads via `ifstream >>`:
first token `nel` (element count), then per element `area`(float) followed
by 4 repetitions of `{neighbor_id(int, 1-based, -1 sentinel) normal_x
normal_y normal_z(floats)}`. 2 "local" neighbors (`i-1`/`i+1`) + 2 random
long-range neighbors per element, chosen specifically to maximize cross-node
traffic (not physically meaningful as a real CFD mesh -- fine, since only
the timing side-channel is measured). GOTCHA: despite the compile-time
thread count, runtime parallelism is still gated by `OMP_NUM_THREADS` --
needs `--env` set to 64 like backprop. Args: `<path-to-synth.domn>`.

**bfs** -- Makefile hardcodes `g++`:
```
cd openmp/bfs
$MUSL_CXX -g -fopenmp -O2 bfs.cpp -o bfs -static -no-pie
```
Needs an external CSR graph file; the original (`graph1MW_6.txt`) isn't
available, so a synthetic one was generated instead (`synth_graph.txt`,
100,000 nodes, average degree 6, random edge targets) matching `bfs.cpp`'s
`fscanf` format: `no_of_nodes`, then per-node `"start_edge_index
no_of_edges"` pairs, then `source_node`, then `edge_list_size`, then
per-edge `"id cost"` pairs. Args: `64 <path-to-synth_graph.txt>` (explicit
thread-count CLI arg, no `--env` needed).

### PARSEC (pkgs/.../src/)

**canneal** (`pkgs/kernels/canneal`) -- self-contained Makefile (no
PARSEC-harness dependency, no `m4` step -- unlike some other PARSEC apps),
uses `$(CXX)`:
```
cd pkgs/kernels/canneal/src
make CXX=$MUSL_CXX version=pthreads CXXFLAGS="-DENABLE_THREADS -pthread -static -no-pie"
```
Input: `.nets` netlist files from the PARSEC input-sim release tarball
(`inputs/400000.nets`, extracted per "Benchmark source" above). Args:
`64 15000 2000 <path-to-400000.nets> 128` (threads, swaps/temp-step,
temp-steps, netlist, max-temp-scale).

**fluidanimate** (`pkgs/apps/fluidanimate`) -- use the `pthreads` variant
Makefile (no TBB needed), uses `$(CXX)`:
```
cd pkgs/apps/fluidanimate/src
make -f Makefile.pthreads CXX=$MUSL_CXX pthreads CXXFLAGS="-pthread -D_GNU_SOURCE -D__XOPEN_SOURCE=600 -static -no-pie"
```
Input: `in_100K.fluid` (simmedium-size, from the PARSEC input-sim release
tarball). Args: `64 5 <path-to-in_100K.fluid> <output-path>` (threads,
sim-steps, infile, outfile).

**freqmine** (`pkgs/apps/freqmine`) -- uses `$(CXX)`, but the Makefile
itself never adds `-fopenmp` even though `fp_tree.cpp` uses `#pragma omp
parallel for` -- must be added manually to both compile and link:
```
cd pkgs/apps/freqmine/src
make CXX=$MUSL_CXX CXXFLAGS="-Wno-deprecated -O2 -fopenmp -static -no-pie" LDFLAGS="-fopenmp -static -no-pie"
```
Needs an external transaction-database file; the real PARSEC input
(`kosarak_500k.dat`) isn't available, so a synthetic one was generated
instead (`synth_trans.dat`: 100,000 transactions, item vocabulary 10,000,
4-15 items/transaction, one whitespace-separated line of integer item IDs
per transaction -- format confirmed via `data.cpp`'s character-at-a-time
parser; no header needed, `ITEM_NO` grows dynamically). Args:
`<path-to-synth_trans.dat> 82` (datafile, `MINSUP` -- scaled down
proportionally from the real `kosarak_500k` config's `MINSUP=410` at 500K
transactions).

## Attack-signal density note (why facesim is a thin-signal benchmark)

Even with all four fixes above, facesim generates a much lower attack-packet
density per instruction than the Rodinia/PARSEC kernels evaluated elsewhere
in this project (~22 attack samples per million instructions at the 64-node,
all-nodes-attack configuration, vs. orders of magnitude more for e.g.
`srad`/`canneal`) -- it spends the overwhelming majority of its instructions
on dense floating-point tetrahedron-mesh math with comparatively little
cross-core cache/directory sharing. Reaching the same ~150K-sample rigor used
for the "solid" tier benchmarks would need on the order of 7 billion
instructions per policy (~15h at the observed instruction rate) -- judged not
worth the wall-clock cost; facesim is included as a thin-signal benchmark
(~434 samples/policy at `--maxinsts=20000000`, a few minutes per run) instead.
This is a data-density property of the workload, not a bug.
