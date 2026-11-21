# Tools used

AI assistance is declared here by the project role whose module or deliverable it supported. Work coordinated through Piyush's session is identified as such; the module owner should review and be able to explain every submitted change.

| Tool and version | Used by / role | Used for | Files affected | What was generated, changed, or checked |
|---|---|---|---|---|
| Claude (Anthropic; version/date not recorded) | Piyush — Architect | Early interface and project scaffolding | `INTERFACE.md`, shared header stubs, `Makefile`, initial `README.md` | Drafts were subsequently integrated and revised by the team. The original prompts and detailed change history were not retained. |
| Codex GPT-6 (OpenAI; Nov 2026) | Piyush — Architect | Application integration, README and architect-owned report sections | `app/main.cpp`, `src/factory.cpp`, `scripts/run_all.sh`, `README.md`, `REPORT.md` §§1–3, 6, 12–13 | Integrated the four real union-find variants into the application, added the Naive size cap and workflow documentation, and drafted the README and the listed report sections from project files and recorded results. The README still identifies personal and instructor-dependent details that must be supplied. |
| ChatGPT (OpenAI; model/version not recorded) | Hemanth — Structure A (D1) | Understanding Union-Find variants, Newman–Ziff sweeps, the union-by-rank height bound, and development/debugging guidance | `src/structA/percolation.cpp`, `src/structA/sweep.cpp`, Structure A presentation/derivation materials (if applicable) | Used for algorithm explanations, design discussion, suggested checks, and debugging guidance. Confirm the exact files affected and independently verify the submitted code and proofs. |
| Codex GPT-6 (OpenAI; Nov 2026) | Hemanth — Structure A | Structure A report write-up | `REPORT.md` §4 | Adapted the owner's supplied write-up and checked it against the current source, interface and saved Oracle run. Updated the verification note to reflect that `tests/test_structA.cpp` is absent in this copy and the real factory is already wired. Structure A owner should verify the derivation and final wording. |
| GCC/G++ 15.2.0 (MSYS2) | Hemanth — Structure A (D1) | Compiling the C++17 implementation | `src/structA/` and test/build targets | Reported as used to compile the project through the Make workflow. |
| GNU Make (MSYS2; version not recorded) | Hemanth — Structure A (D1) | Building and running tests | `Makefile` and build/test outputs | Reported use of `make test`; confirm the exact test commands and results against the submitted repository. |
| MSYS2 UCRT64 (version not recorded) | Hemanth — Structure A (D1) | Windows command-line build environment | No direct source files | Reported use for compiler, Make, and Git commands. |
| Git / GitHub (versions not recorded) | Hemanth — Structure A (D1) | Version control and team collaboration | Structure A project files | Reported use for local changes, commits, and team repository workflow. |
| Visual Studio Code (version not recorded; confirm use) | Hemanth — Structure A (D1) | Editing and reviewing code | Structure A source files | Included in Hemanth's supplied declaration; confirm it was used before final submission. |
| Codex GPT-6 (OpenAI; Nov 2026) | Datthatreya — Structure B | Structure B report write-up | `REPORT.md` §5 | Drafted the comparison of the three Structure B variants, their complexity, invariants and design rationale from the implementation and interface contract. The Structure B owner should review the description. |
| Codex GPT-6 (OpenAI; Nov 2026) | Dhanvanth — Analyst | Benchmark seed, plots, and results/report analysis | `bench/main.cpp`, `scripts/plot_results.py`, `README.md`, `REPORT.md` §§10–11 | Set the benchmark seed default to 171226; adjusted the plots to keep the requested figures and show the OLS fit line only for Rank + compression; reviewed benchmark CSV values and drafted the experimental setup/results comparison. The full workflow was run with the recorded seed. |
| Codex GPT-6 (OpenAI; Nov 2026) | Manirathna (Mani) — Oracle | Oracle seed configuration and correctness/report write-up | `oracle/differential.hpp`, `oracle/oracle.mk`, `oracle/diff_main.cpp`, `oracle/sweep_main.cpp`, `REPORT.md` §§1, 3, 7, 12–13 | Changed Oracle defaults/examples to seed 171226 and supplied the `oracle-real` command. Reviewed the Oracle owner's supplied implementation notes and saved run output; documented the oracle proof, checker map, differential/sweep results, limitations and references. The owner should verify the write-up and fill personal/PR details. |

## Representative prompts

Prompts below are representative of the requests that guided Codex assistance; wording is shortened for readability.

- **Architect:** “Can you wire the real union-find variants into the application?”
- **Analyst:** “Change the plotting code so only Rank + compression has a fitted line, while keeping the four variants' (p_c) values in the legend.”
- **Oracle:** “Give me the command to run the real Oracle test with seed 171226 and save its output.”



1. “Explain what a Newman–Ziff sweep means in percolation and what an incremental sweep is.”
2. “Explain Union-Find variants: naive, union by rank, path compression, and rank plus compression.”
3. “Prove that under union by rank a root of rank $r$ has at least $2^r$ nodes, so tree height is at most $\lfloor\log_2 n\rfloor$.”
4. “Explain the Structure A implementation and what to include in the project presentation.”
5. “How should I build, test, and commit the Structure A project using MSYS2, Make, and Git?”
