# Spec Coding Usage

本文件记录本项目的 Spec Coding 协作模式、对话历史与反馈。与 Claude Code memory 双向同步，可随时手动编辑。

---

## 会话恢复

新会话中粘贴以下命令即可恢复完整上下文：

```
/init
读 Spec_Coding_Usage.md 恢复上次会话状态，继续工作。
上次会话: ef386477-fa70-4667-a3c8-bd55f33be9a2
项目: MiniDB (2026_CPP_DB)，C++20/23，zh-Hans
```

> **Session ID**: `ef386477-fa70-4667-a3c8-bd55f33be9a2`

---

## 工作流（5 步法）

| 步骤 | 动作 | 触发示例 |
|------|------|----------|
| 1. 初始化 | `/init` → 生成 CLAUDE.md 项目上下文 | `/init` |
| 2. 审计 | 全面检查（测试/平台/覆盖），先报告再动手 | `check if all modules' test are implemented properly, and check linux_amd64 compatibility` |
| 3. 批量修复 | 一次性修复所有发现的问题 → 构建验证 → 测试通过 | `fix all that encountered and generate a fix commit` |
| 4. 提交 | 规范化 commit + `Co-Authored-By` 尾注 | — |
| 5. 文档 | 生成完整中文项目文档 / README | `generate a better manual and change the major locale to zh-hans` |

---

## 提交规范

- 格式：`类型: 英文简述`
- 尾注：`Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>`

---

## 项目信息

- **仓库**：`2026_CPP_DB` — MiniDB 微型关系型数据库
- **课程**：《现代 C++ 程序设计》，电子科技大学，2025–2026 第二学期
- **截止**：2026-06-21
- **语言**：C++20/23，文档主语言 zh-Hans
- **硬性约束**：禁用 STL 容器，须在 Linux 下构建通过

---

## Session 对话历史 (2026-05-30)

### 对话 1 — 初始化

> **用户**：`/init`

**Claude**：探索代码库结构（CMake、include/、src/、tests/），读取所有头文件和关键实现，生成 `CLAUDE.md`，包含构建命令、四层架构（parser→storage→execution→network）、编码规范（无 STL、命名空间、头文件保护、测试框架）。

**产出**：`CLAUDE.md`（首次创建）

---

### 对话 2 — 审计

> **用户**：`check if all modules' test are implemented properly, and check linux_amd64 compatibility`

**Claude**：读取全部 5 个测试文件和 10+ 源文件，逐行审查平台相关代码。

**审计报告**：

| 级别 | 问题 | 位置 |
|------|------|------|
| 🔴 编译失败 | `rmdir()` 缺少 `<unistd.h>`（Linux） | `test_storage.cpp`, `test_executor.cpp` |
| 🟡 弃用警告 | `gethostbyname` 在 Linux 已弃用 | `client.cpp:39` |
| 🟢 已验证 | 网络层 `#ifdef _WIN32`/POSIX 分支正确 | `server.cpp`, `client.cpp` |
| 🟢 已验证 | `mkdir`/`_mkdir`、`rmdir`/`_rmdir` 宏正确 | `executor.cpp`, `database.cpp` |
| 🟢 已验证 | 路径分隔符统一使用 `/`，跨平台兼容 | 全部文件 |
| ℹ️ 质量 | `test_executor.cpp` — 静态共享状态导致测试不隔离 | `exec()` helper |
| ℹ️ 质量 | 两套不同测试框架风格混用 | lexer/parser vs storage/executor |

---

### 对话 3 — 批量修复

> **用户**：`fix all that encountered and generate a fix commit`

**Claude**：三处修改 + 构建 + 测试 + 提交。

| 文件 | 修改 |
|------|------|
| `tests/test_storage.cpp` | `#else` 分支添加 `#include <unistd.h>` |
| `tests/test_executor.cpp` | 同上 |
| `src/network/client.cpp` | `gethostbyname` → `getaddrinfo`/`freeaddrinfo`，添加 `#include <netdb.h>` |

**构建**：MSVC clean build 全目标通过（仅 C4819 编码页警告，预存在）  
**测试**：5/5 passed  
**提交**：`6b7957e fix: Linux compatibility — missing <unistd.h>, deprecated gethostbyname`

---

### 对话 4 — 文档

> **用户**：`generate a better manual and change the major locale to zh-hans`

**Claude**：创建完整中文 `README.md`，包含环境要求、构建/测试/启动命令、SQL 使用示例、语法表、ASCII 架构图（数据流 + 库依赖）、存储文件格式、JSON 通信协议、项目文件树、编码规范。更新 `CLAUDE.md` 标注 zh-Hans 为主语言。

**提交**：`707a857 docs: add Chinese README and CLAUDE.md, set locale to zh-Hans`

---

### 对话 5 — 记忆持久化

> **用户**：`update the session history into Spec_Coding_Usage (this should be done automatically)`

**Claude**：创建 memory 条目 `spec-coding-usage`（`MEMORY.md` 索引 + `spec_coding_usage.md`），记录工作流、提交规范、项目上下文。

> **用户**：`Also User-touchable .md file, sync`

**Claude**：在项目根目录创建 `Spec_Coding_Usage.md`，与 memory 双向同步，用户可随时手动编辑。

> **用户**：`Include the dialogs and feedbacks also`

**Claude**：将完整对话历史与反馈写入本文档。

---

## 用户反馈记录

| 反馈 | 对应的调整 |
|------|-----------|
| "fix all that encountered and generate a fix commit" | 审计后直接批量修复→构建→测试→提交，一步到位 |
| "generate a better manual and change the major locale to zh-hans" | 文档以中文为第一语言，内容覆盖全貌 |
| "this should be done automatically" | 记忆持久化应作为每次会话的收尾步骤 |
| "Also User-touchable .md file, sync" | memory 和项目文件保持双向镜像 |
| "Include the dialogs and feedbacks also" | 记忆需包含原始对话与反馈，而非仅结论 |
