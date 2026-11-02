# 手动提交到 GitHub

目标仓库：https://github.com/wwwfirstsleep/os-2026-lab2

## 使用 Git

在解压出的 `os-2026-lab2` 目录中打开终端：

```bash
git init -b main
git add .
git commit -m "Complete OS synchronization lab in C++"
git remote add origin https://github.com/wwwfirstsleep/os-2026-lab2.git
git push -u origin main
```

如 Git 提示没有作者信息，先按自己 GitHub 账户设置 user.name 和 user.email。
若目标仓库已有提交，请先 clone 仓库，再将本包文件复制进去提交；不要强制覆盖原有历史。

`report/` 含个人姓名和学号，是否随源码公开上传由你决定；代码输出也按指导书要求包含个人标记。

## 网页上传

在仓库页面选择 Add file → Upload files，上传源码目录、Makefile、README、scripts、results 等文件，填写提交说明后提交。需要保留 `.github/workflows/linux.yml` 时建议使用 Git，以免网页选文件时遗漏隐藏目录。

## 本地复现

```bash
make
bash scripts/run_all.sh local
```

Linux 验证：

```bash
bash scripts/run_all.sh linux
```

GitHub 工作流如被执行，会在 Actions 生成 `linux-experiment-results` 下载产物。本包内不包含已经执行过的 Linux 结果。
