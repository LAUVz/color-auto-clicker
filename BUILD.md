# Building Color Auto-Clicker - Multi-Platform

This guide covers building the cross-platform Electron version for Windows, macOS, and Linux using **GitHub Actions**.

## 🚀 GitHub Actions - Automated Builds

**Builds all platforms automatically in the cloud - FREE on GitHub!**

### Quick Start

1. **Push your code to GitHub**
2. **Create a version tag**: `git tag v1.0.0 && git push --tags`
3. **GitHub builds all platforms automatically** (~20 minutes)
4. **Download from Releases page**

That's it! 🎉

---

## Setup (One-Time)

### Push to GitHub

```bash
# Initialize git (if not already done)
git init
git add .
git commit -m "Initial commit"

# Create repository on GitHub, then:
git remote add origin https://github.com/YOUR_USERNAME/color-clicker.git
git push -u origin main
```

The workflow is already configured at `.github/workflows/build.yml` ✅

---

## Creating Releases

### Automatic Release (Recommended)

```bash
# Bump version and create tag
npm version 1.0.0

# Push to GitHub
git push origin main --tags
```

**What happens:**
1. ✅ GitHub Actions builds Windows, macOS, Linux (parallel)
2. ✅ Creates GitHub Release with all binaries
3. ✅ Ready to download in ~20 minutes

### Manual Build (No Release)

1. Go to GitHub repository → **Actions** tab
2. Select "Build Multi-Platform Releases"
3. Click **"Run workflow"** → Select branch → Run
4. Download artifacts from the workflow run

---

## What Gets Built

| Platform | Files | Size |
|----------|-------|------|
| **Windows** | ColorClicker.exe | ~68 MB |
| **macOS** | ColorClicker.dmg + ColorClicker-mac.zip (Universal) | ~100 MB |
| **Linux** | ColorClicker.AppImage + .deb packages (x64 + arm64) | ~100 MB |

**Build time:** ~20 minutes (all platforms in parallel)

---

## Version Management

Quick version bumping:

```bash
# Patch: 1.0.0 → 1.0.1
npm version patch && git push --follow-tags

# Minor: 1.0.0 → 1.1.0
npm version minor && git push --follow-tags

# Major: 1.0.0 → 2.0.0
npm version major && git push --follow-tags
```

Each command triggers automatic builds! 🚀

---

## Monitoring Builds

### View Progress
- Repository → **Actions** tab
- Click on running workflow
- See real-time logs

### Download Results

**From Releases (tagged builds):**
- **Releases** tab → Latest release → Download files

**From Actions (manual trigger):**
- Actions tab → Workflow run → **Artifacts** section

---

## Troubleshooting

### Builds Not Triggering?

✅ Check: `.github/workflows/build.yml` exists
✅ Check: Actions enabled (Settings → Actions)
✅ Check: Tag format is `v*.*.*` (e.g., `v1.0.0`)
✅ Check: Tag was pushed (`git push --tags`)

### Build Failed?

1. Actions tab → Click failed job
2. Read error logs
3. Common fixes:
   - **Windows**: Python setuptools (handled automatically)
   - **macOS**: Code signing (disabled by default)
   - **Linux**: Dependencies (installed in workflow)

---

## Local Testing

Test builds locally before pushing:

```bash
# Windows
npm run dist:win

# macOS (requires Mac)
npm run dist:mac

# Linux (requires Linux)
npm run dist:linux
```

---

## GitHub Actions Limits

**Free tier:**
- ✅ **Unlimited** for public repos
- ✅ **2000 min/month** for private repos

**Your usage:**
- ~20 minutes per release
- = **100+ releases/month possible!**

---

## Workflow Details

The `.github/workflows/build.yml` includes:

✅ Parallel builds (Windows + macOS + Linux)
✅ Auto-rebuild native modules (robotjs)
✅ Python setup for Windows
✅ Linux dependencies (libx11, libxtst)
✅ Artifact retention (30 days)
✅ Auto-create GitHub Releases on tags

---

## Summary

**Simple Release Process:**

```bash
npm version 1.0.0
git push --follow-tags
# → Wait 20 minutes
# → Download from Releases page
```

**That's it! No Docker, no VMs, no hassle.** 🎉

---

## Resources

- **GitHub Actions**: https://docs.github.com/en/actions
- **electron-builder**: https://www.electron.build/
- **Workflow file**: [.github/workflows/build.yml](.github/workflows/build.yml)
