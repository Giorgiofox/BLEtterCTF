#!/usr/bin/env bash
#
# Publish the wiki/ folder to the GitHub wiki repo.
#
# GitHub only creates the wiki git repo AFTER the first page is created in the
# web UI. So the one-time manual step is:
#
#   1. Open https://github.com/Giorgiofox/BLEtterCTF/wiki
#   2. Click "Create the first page", type anything, Save.
#
# After that, this script mirrors wiki/ into the wiki repo on every run.
#
set -euo pipefail

REPO_SSH="git@github.com:Giorgiofox/BLEtterCTF.wiki.git"
HERE="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$HERE/wiki"
WORK="$(mktemp -d)"

echo "cloning wiki repo ..."
if ! git clone "$REPO_SSH" "$WORK" 2>/dev/null; then
  echo "ERROR: wiki repo not found. Create the first page in the web UI first:" >&2
  echo "  https://github.com/Giorgiofox/BLEtterCTF/wiki" >&2
  rm -rf "$WORK"
  exit 1
fi

echo "syncing pages ..."
# remove old markdown, copy fresh (keep .git)
find "$WORK" -maxdepth 1 -name '*.md' -delete
cp "$SRC"/*.md "$WORK"/

cd "$WORK"
git add -A
if git diff --cached --quiet; then
  echo "no changes."
else
  git -c user.name="Giorgiofox" -c user.email="giorgiofox@gmail.com" \
      commit -m "wiki: sync from repo wiki/ folder"
  git push origin HEAD
  echo "published."
fi

rm -rf "$WORK"
