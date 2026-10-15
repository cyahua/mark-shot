# 使用 GitHub CLI (gh) 提交 Issue 说明

GitHub CLI (命令为 `gh`) 是 GitHub 官方提供的命令行工具。本文件介绍如何在本项目中使用 `gh` 工具提交 Issue，以提高问题反馈与处理的效率。

## 准备工作

在提交 Issue 之前，需要安装并配置好 GitHub CLI 工具。

### 1. 安装 GitHub CLI
根据您的操作系统，选择对应的安装方式。详细安装指南请参阅 [GitHub CLI 官方安装说明](https://cli.github.com)。

### 2. 身份验证
安装完成后，在终端运行以下命令进行登录认证：
```bash
gh auth login
```
按照终端提示选择您的 GitHub 账号进行登录，并完成授权。

---

## 提交方式

本项目提供了中文与英文两个版本的 Issue 模板。由于本软件（mark-shot）作为截图标注工具，其运行表现与显示服务器（Wayland 或 X11）及窗口管理器密切相关，因此推荐使用“自动收集环境脚本”方式提交 Bug。

### 方式一：使用辅助脚本自动收集环境（推荐，支持中英双语）

本项目在 [scripts/submit-issue.sh](../scripts/submit-issue.sh) 中提供了一个自动化脚本。此脚本会自动收集当前主机的以下环境信息：
- 操作系统版本
- 当前窗口管理器或桌面环境
- 显示服务器类型（Wayland 或 X11）
- 安装的 Qt 版本
- mark-shot 软件版本

运行此脚本即可开始提交流程：
```bash
./scripts/submit-issue.sh
```

**执行流程**：
1. 运行后，脚本会首先提示您选择首选语言（中文或英文）。
2. 接着提示选择 Issue 类型（Bug 报告或功能请求）。
3. 若选择 Bug 报告，脚本将自动提取当前系统的各项参数，填入对应的模板中：
   - 中文版：[bug_report_zh.md](../.github/ISSUE_TEMPLATE/bug_report_zh.md)
   - 英文版：[bug_report_en.md](../.github/ISSUE_TEMPLATE/bug_report_en.md)
4. 随后脚本会调用 `gh issue create`，在终端引导您输入 Issue 标题，并提供进一步编辑描述内容或直接提交的选项。
5. 若选择功能请求，脚本会直接载入对应的模板供您填写：
   - 中文版：[feature_request_zh.md](../.github/ISSUE_TEMPLATE/feature_request_zh.md)
   - 英文版：[feature_request_en.md](../.github/ISSUE_TEMPLATE/feature_request_en.md)

---

### 方式二：直接使用 gh 命令（交互式）

如果您不想运行辅助脚本，也可以直接在项目根目录下使用 `gh` 的标准交互式命令，指定对应的模板。

#### 1. 提交中文 Issue
在终端执行以下命令：
```bash
# 提交中文 Bug 报告
gh issue create --template "bug_report_zh.md"

# 提交中文新功能请求
gh issue create --template "feature_request_zh.md"
```

#### 2. 提交英文 Issue (Submit English Issues)
在终端执行以下命令：
```bash
# Submit English Bug Report
gh issue create --template "bug_report_en.md"

# Submit English Feature Request
gh issue create --template "feature_request_en.md"
```
终端会载入相应语言的模板，按照提示输入标题并填写相关环境及描述信息即可。
