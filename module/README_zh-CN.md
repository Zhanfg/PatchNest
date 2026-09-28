# PatchNest Module

[English](README.md) | **中文**

PatchNest Module 为 Magisk、KernelSU、KernelSU-Next 和 APatch 提供 KPM 宿主、安装管理与 WebUI。项目基于 `KernelSU-Next/KPatch-Next-Module` 继续维护，但使用独立的发布与依赖链。

## 仓库架构

| 仓库 | 职责 |
|---|---|
| [`Zhanfg/PatchNest`](https://github.com/Zhanfg/PatchNest) | canonical monorepo：`cli/`、`module/`、WebUI 与 `kpms/` |
| [`Zhanfg/KernelPatch-Public`](https://github.com/Zhanfg/KernelPatch-Public) | 独立维护的 `kpimg`、`kptools` 源码与发布资产 |

## KPM 仓库

默认目录地址：

```text
https://raw.githubusercontent.com/Zhanfg/PatchNest/main/kpms/kpm_repo.json
```

WebUI 也支持添加其他 HTTPS 目录。系统级目录覆盖文件位于：

```text
/data/adb/patchnest/repos.json
```

KPM 源码与校验现在位于 monorepo 的 `kpms/` 组件；目录中的 KPM 二进制仍不会直接打包进 PatchNest Module。

## 构建完整性

依赖版本与可信 Release 摘要统一固定在 `version.properties`。本地构建和 CI 遇到缺失或不匹配的 SHA256 时都会直接失败。

版本规则：

```text
内部版本：0.4.1-rc2
Git 标签：v0.4.1-rc2
```

历史独立 Module Release 保留原有 `v<版本>` 标签；canonical 仓库后续 Module Release 使用 `module-v<版本>` 命名空间。`module.prop` 与 `update.json` 仍使用不带标签前缀的内部版本。

当前重启基线和后续任务记录在 [`docs/restart/BASELINE.md`](docs/restart/BASELINE.md)。

## 鸣谢

- 上游模块：[`KernelSU-Next/KPatch-Next-Module`](https://github.com/KernelSU-Next/KPatch-Next-Module)
- 补丁脚本来源：[`bmax121/APatch`](https://github.com/bmax121/APatch)
- `magiskboot` 来源：[`topjohnwu/Magisk`](https://github.com/topjohnwu/Magisk)

## 许可证

- PatchNest-Module：[GPL-3.0](LICENSE)
- PatchNest 与 KernelPatch 组件继续遵循对应上游 GPL 许可证。
- WebUI：[MIT](webui/LICENSE)
