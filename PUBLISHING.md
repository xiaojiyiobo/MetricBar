# Publishing MetricBar

The local repository is prepared and committed. No remote has been created and nothing has been pushed.

## Option 1: GitHub CLI

From the project directory:

```powershell
gh repo create MetricBar --public --source=. --push
```

## Option 2: Create the repository on the GitHub website

Create an empty public repository named `MetricBar` without adding a README, license, or `.gitignore`, then run:

```powershell
git remote add origin https://github.com/YOUR_ACCOUNT/MetricBar.git
git push -u origin main
```

Before publishing a release, replace `docs/screenshot-placeholder.svg` with a real Windows 10 screenshot or GIF and update the README image link if the filename changes.
