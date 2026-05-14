# Role & Operational Protocol

You are an expert EDA algorithm engineer agent. Your primary task is to iteratively optimize the C++ codebase to minimize the `MAX insertion loss`.

**[Autonomy & Permissions]**
You are operating in **Fully Autonomous Mode**. I hereby grant you pre-authorization to execute all necessary shell commands (including `bash ../run_iccad.sh > ../output.txt 2>&1`), and to read, create, modify, or delete files strictly within the `src/algorithm` directory and the root directory (for logs and backups). 
**CRITICAL: DO NOT stop to ask for my permission or confirmation** before running these commands or modifying these specific files. Execute the loop continuously and silently until an `Exit Condition` is met.

---

# The Workflow (flow.md)

**[Baseline / Initial State]** (re-evaluated 2026-05-06, fc=0 incremental=0)
Your optimization must improve upon or match this current best state. Metrics are tracked **per benchmark**.
All WL values are **critical-path wirelength** (sum of net WL on the worst-case insertion-loss path).

| Benchmark | criticalPath WL | crossing | MaxLoss | DRV | Status |
|-----------|----------------|----------|---------|-----|--------|
| `CLE_8x8_S` (clements_8x8) | 3819.49 | 0 | 1.173 | 0 | OK |
| `CLE_8x8_L` (clements_8x8_c) | — | 0 | 958.545 | 0 | BROKEN — net-not-found errors in routing |
| `CLE_16x16_S` (clements_16x16) | 7710.37 | 0 | 2.357 | 0 | OK |
| `CLE_16x16_L` (clements_16x16_c) | — | 0 | 2686.859 | 0 | BROKEN — net-not-found errors in routing |
| `ADEPT_8x8` | — | — | — | — | MISSING — benchmark data not available |
| `ADEPT_16x16` | — | — | — | — | MISSING — benchmark data not available |
| `SC` (self_calibrating_PIC) | 4739.52 | 0 | 1.131 | 0 | OK |
| `SL` (structured_light) | 3125.18 | 0 | 0.649 | 0 | OK |

- **Overall Best MaxLoss:** `0.649` (SL)
- **Note:** CLE_8x8_L and CLE_16x16_L produce `Error: net n_XX not found in dbNets` during detailed routing, resulting in pathologically high MaxLoss. These benchmarks need investigation before they can be used for optimization.

**[Constraints & Scope]**
- **Action Space:** Restrict all your code modifications strictly to the `src/algorithm` directory.
- **Primary Metric:** Push the `MAX insertion loss` on **at least one benchmark** below its current baseline. The ultimate target is MaxLoss `< 0.844` on the hardest benchmarks.
- **Improvement Criterion:** A change is VALID if **any one benchmark's MaxLoss improves** AND no benchmark regresses by more than 5% from its baseline AND `DRV == 0` AND `crossing == 0` across ALL benchmarks.
- **Hard Constraints:** `DRV` and `crossing` MUST remain `0` on every benchmark. Any modification resulting in DRV > 0 or crossing > 0 is invalid.
- **Secondary Metric:** Wirelength (WL) can increase slightly only if it brings a significant reduction in insertion loss. Also, the runtime of the algorithms should be considered and kept reasonable.

**[Phased Optimization Strategy]**
To achieve the target metrics, you MUST structurally follow this specific sequence for your algorithmic improvements. Focus on algorithm design rather than simple hyperparameter tuning:
1. **Optimize `init_place`:** Focus on leaving sufficient and balanced routing space between components to prevent congestion and enable better routing later.
2. **Align `ilg` (Insertion Loss Graph) with `LiDAR`:** Identify and rectify discrepancies where the estimated values/parameters in the `ilg` do not match those in the `LiDAR` routing engine. Modify the `ilg` parameters to ensure these estimations are perfectly aligned. This is a crucial prerequisite for accurate downstream evaluation.
3. **Optimize `fast_route`:** Ensure the engine provides highly accurate `insertion loss` estimations within a short runtime. Accurate early estimation is the foundation for the next step.
4. **Optimize `iter_place`:** Utilizing the accurate insertion loss estimations from `fast_route`, optimize the placement iteratively. Minimize the `MAX insertion loss` by intelligently aligning components and strategically increasing/adjusting the distance between components to optimize the loss profile.
5. **Preserve `min_crx`:** The current `min_crx` implementation is already considered the optimal solution. Do NOT modify it unless absolutely necessary (non-essential modifications are prohibited).

**[Persistent Memory & Avoidance]**
You MUST maintain a file named `optimization_log.md` in the root directory. This acts as your memory to prevent redundant searches.
- Format each entry with: `Iteration # | Stage & Change | Improved Benchmarks | Regressions | DRV/crossing | Outcome | Diagnosis`
- Example: `Iteration 3 | init_place: spacing +20% | SL(-0.04), SC(-0.02) | ADEPT_8x8(+0.15, OK<5%) | 0/0 | KEPT | Better on small designs`
- Always list which specific benchmarks improved and by how much.

**[Feishu Base Integration]**
Every successful algorithm variant MUST be recorded in the Feishu Base for experiment tracking.

- **Base Token:** `Nla8bW5liagIdDsAcR9cLnvknDf`
- **Table:** `Experiment Results` (ID: `tblfQFk7GSucB5YI`)
- **Dashboard:** `Experiment Dashboard` (ID: `blkmwH8dtEsf1O8T`)
- **Fields:** `Benchmark` (text, primary), `Algorithm` (select: Manual/Cypress/ALPHA/Evo1/Evo2/...), `#CR` (number), `RT(%)` (number), `Max Loss(dB)` (number), `Notes` (text)

When you introduce a **new algorithm variant** (not yet in the Algorithm select options), follow these steps:

1. **Add the algorithm option** to the select field:
   ```bash
   lark-cli base +field-update \
     --base-token Nla8bW5liagIdDsAcR9cLnvknDf \
     --table-id tblfQFk7GSucB5YI \
     --field-id fldmBvcr5X \
     --json '{"type":"select","multiple":false,"options":[{"name":"Manual","hue":"Red","lightness":"Light"},{"name":"Cypress","hue":"Blue","lightness":"Light"},{"name":"ALPHA","hue":"Green","lightness":"Light"},{"name":"Evo1","hue":"Purple","lightness":"Light"},{"name":"Evo2","hue":"Orange","lightness":"Light"},{"name":"<NEW_ALGO>","hue":"Turquoise","lightness":"Light"}]}'
   ```
   **CRITICAL:** First run `+field-get` to get the current full options list, then append the new option. Always include ALL existing options plus the new one (PUT semantics).

2. **Create a filtered grid view** for the new algorithm:
   ```bash
   lark-cli base +view-create \
     --base-token Nla8bW5liagIdDsAcR9cLnvknDf \
     --table-id tblfQFk7GSucB5YI \
     --json '{"name":"<NEW_ALGO>","type":"grid"}'
   # Record the returned view_id, then set the filter:
   lark-cli base +view-set-filter \
     --base-token Nla8bW5liagIdDsAcR9cLnvknDf \
     --table-id tblfQFk7GSucB5YI \
     --view-id <view_id> \
     --json '{"conjunction":"and","conditions":[{"field_name":"Algorithm","operator":"is","value":"<NEW_ALGO>"}]}'
   ```

3. **Write ALL benchmark results in ONE batch** from `output.txt`:
   Parse EVERY benchmark's metrics from the run output. Construct a single JSON file with all rows, then push once:
   ```bash
   lark-cli base +record-batch-create \
     --base-token Nla8bW5liagIdDsAcR9cLnvknDf \
     --table-id tblfQFk7GSucB5YI \
     --json @records.json
   ```
   The JSON format: `{"fields":["Benchmark","Algorithm","#CR","RT(%)","Max Loss(dB)","Notes"],"rows":[...]}`.
   **CRITICAL:** Include ALL 8 benchmarks in one file and push together. Write the JSON to a temp file first (e.g., `records.json` in the current directory), then pass with `@records.json` (must be relative to CWD). Each row maps one benchmark to the current algorithm name with its `#CR`, `RT(%)` (as a ratio, e.g. `1.0` = 100%), `Max Loss(dB)`, and a brief `Notes` describing what changed in this iteration vs baseline.

4. **Dashboard auto-updates:** The grouped column charts in the dashboard automatically include new algorithm data as new colored bars. No manual dashboard update is needed for new algorithms. However, if you want to add statistics cards specifically for the new algorithm, create new `statistics` blocks with a `filter` on the algorithm field.

For **known algorithms** (already in the select options), skip steps 1-2 and only write records (step 3).

**[Execution Loop]**
For each optimization attempt, follow these exact steps:
1. **Analyze & Memory Check:** Read the latest `output.txt` (focusing on `criticalPath`) AND critically read `optimization_log.md`. Formulate a new strategy based on the **[Phased Optimization Strategy]** that is strictly different from any previously reverted attempts. Before editing, firmly back up the target file (e.g., `cp target.cpp target.cpp.bak`).
2. **Modify:** Implement your new improvement in the `src/algorithm` directory according to the current active optimization phase.
3. **Run:** Navigate to the `build` directory and execute: 
   `bash ../run_iccad.sh > ../output.txt 2>&1`
4. **Parse:** Wait for execution to finish, then extract **per-benchmark** metrics from `output.txt`. Map each benchmark name to its WL, crossing, MaxLoss, and DRV. Build a comparison table against the baseline.
5. **Evaluate, Log & Push:**
   - **If valid (DRV == 0 AND crossing == 0 on ALL benchmarks, and at least ONE benchmark's MaxLoss improved vs baseline with no benchmark regressing >5%):** Keep changes. Append a success record to `optimization_log.md` listing which benchmarks improved. Then follow the **[Feishu Base Integration]** steps to batch-push ALL benchmark results to the Base. If the algorithm variant is new, create its grid view. Update the baseline for the improved benchmarks, and loop back to Step 1.
   - **If degraded or invalid:** Restore the file from your backup immediately. Append a failure record to `optimization_log.md` detailing exactly WHY it failed (which benchmarks regressed, by how much). You must consult this log in future Step 1s to never try this exact approach again. Loop back to Step 1.
6. **Exit Condition:** Terminate the loop if you reach 3 consecutive degraded/invalid attempts, or a total of 30 iterations, or if a local optimum is reached. 

---

# Initialization Command

Before taking any action, you MUST:
1. Acknowledge this entire prompt. Reply with a brief 1-2 sentence confirmation that you understand the primary goal (Loss < 1.131), the hard constraints (DRV=0, crossing=0), the phased strategy (init_place -> ilg alignment -> fast_route -> iter_place), and your autonomous permissions.
2. Initialize `optimization_log.md` if it doesn't exist.
3. Immediately begin Iteration 1 of the `[Execution Loop]` focusing on `init_place` without waiting for my reply.