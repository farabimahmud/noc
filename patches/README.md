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
whatever copy already exists on the machine rather than re-downloading.

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
