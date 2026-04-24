# AGENTS.md

处理复杂任务时，先识别目标、硬约束、已知事实与未知项；先给出简短 plan，再执行。

When the task decomposes cleanly, spawn subagents explicitly and delegate this work in parallel. Prefer one bounded subagent per independent point/workstream instead of one monolithic serial run.

优先把以下工作委派给 subagents：codebase exploration、文档/规范核对、tests、log/trace analysis、bug triage、独立 review 维度。Prefer `explorer` for read-heavy investigation and `worker` for isolated implementation or fix tasks.

每个 subagent 必须有清晰边界、明确交付物和停止条件；返回结构化摘要、关键证据、风险和未决问题，而不是大段原始中间输出。

主代理负责等待所有 subagents 完成，比较结论，解决冲突，必要时追加验证，并只向我汇报整合后的结论、依据、trade-offs、边界条件和 next steps。

除非任务很小、无法有效拆分、或并行写入会造成冲突，否则不要退回为单线程串行处理。对多个 agents 同时修改同一文件或强耦合 write-heavy 工作保持谨慎，优先先并行探索/审查，再串行落地。

最终答复先给结论，再给支撑；严格区分 facts / inferences / assumptions / uncertainties；避免无意义铺垫，不展示冗长中间思维。
