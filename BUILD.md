# RawToHdr Android — 通过 GitHub Actions 一键构建 APK

你只需要做 **1 件事**：把代码推送到你的 GitHub 仓库，Actions 会自动编译出 APK，你直接下载安装。

---

## 操作步骤（5 分钟搞定）

### 1️⃣ 在 GitHub 上新建一个仓库

打开 https://github.com/new → 仓库名填 `RawToHdrAndroid` → 创建（不要勾选任何初始化选项）

### 2️⃣ 推送代码

打开终端（Windows 可以用 Git Bash），逐行执行：

```bash
cd /c/Users/Administrator/WorkBuddy/20260319101354/RawToHdrAndroid

git init
git add .
git commit -m "Initial commit: RawToHdr Android app"
git branch -M main
git remote add origin https://github.com/你的GitHub用户名/RawToHdrAndroid.git
git push -u origin main
```

> ⚠️ 把 `你的GitHub用户名` 换成你的实际用户名

### 3️⃣ 等待构建完成

- 打开你的 GitHub 仓库页面
- 点击顶部 **Actions** 标签
- 会看到一个正在运行的 `Build Android APK` 工作流
- **等待约 10-15 分钟**，变成绿色 ✔ 即构建成功

### 4️⃣ 下载 APK

构建变绿后：
- 点击该工作流进入详情
- 往下翻到 **Artifacts** 区域
- 点击 **RawToHdr-Debug-APK** 下载 zip
- 解压得到 `app-debug.apk`
- 传到手机安装即可使用

### 5️⃣ 以后每次修改代码

改完代码后执行：
```bash
git add .
git commit -m "修改说明"
git push
```
GitHub Actions 会自动重新构建，生成新的 APK。

---

## 常见问题

**Q: 构建失败怎么办？**
A: 进入 Actions → 点失败的构建 → 看日志中的报错信息，通常是 libraw 下载或 NDK 版本问题。

**Q: 需要安装 Android Studio 吗？**
A: 不需要。GitHub Actions 在云端自动完成编译，你只需要 Git 推送代码。

**Q: 怎么手动触发构建？**
A: 进入 Actions → 选择 "Build Android APK" → 点 "Run workflow" → "Run workflow"。