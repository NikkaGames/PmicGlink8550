---
page_type: sample
languages:
- cpp
tools:
- windows-driver-kit
products:
- windows-hardware
description: "Qualcomm PMIC GLink KMDF driver workspace"
---

# Qualcomm PMIC GLink Driver

This repository now contains a PMIC GLink focused KMDF driver for ZTE RedMagic 8 Pro.

## Build

Use the provided script from repository root:

```bat
build_arm64.cmd
```

The current project outputs:

- `pmicglink8550.sys`
- `pmicglink8550.inf`
- `pmicglink8550.cat`
