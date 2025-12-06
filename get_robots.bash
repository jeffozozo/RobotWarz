#!/usr/bin/env bash
set -euo pipefail

ORG="dsu-cit-cs3005"

# This should match the actual repo name prefix, e.g. "robot-staging-jeffozozo"
ASSIGNMENT_PREFIX="robot-staging"

DEST="${ASSIGNMENT_PREFIX}-repos"
ROBOTS_DIR="robots"

echo "Org:        $ORG"
echo "Prefix:     $ASSIGNMENT_PREFIX"
echo "Clone dir:  $DEST"
echo "Robots dir: $ROBOTS_DIR"

# Make sure directories exist
mkdir -p "$DEST"
mkdir -p "$ROBOTS_DIR"

# Clear out old Robot_*.cpp files from robots/
echo "Clearing old Robot_*.cpp files from $ROBOTS_DIR..."
rm -f "$ROBOTS_DIR"/Robot_*.cpp 2>/dev/null || true

echo
echo "=== Cloning / updating assignment repos ==="

# list repo names that START with the assignment prefix
gh repo list "$ORG" --limit 1000 --json name \
  --jq ".[] | select(.name | startswith(\"$ASSIGNMENT_PREFIX\")) | .name" \
| while read -r name; do
    [ -z "$name" ] && continue
    echo "→ $name"

    if [ -d "$DEST/$name/.git" ]; then
        echo "   already exists, pulling latest..."
        (cd "$DEST/$name" && gh repo sync "$ORG/$name" --force) || {
            echo "   sync failed, falling back to git pull…"
            (cd "$DEST/$name" && git pull --rebase || true)
        }
    else
        gh repo clone "$ORG/$name" "$DEST/$name" -- --depth=1 || {
          echo "   clone failed, sleeping 10s and retrying…"
          sleep 10
          gh repo clone "$ORG/$name" "$DEST/$name" -- --depth=1
        }
    fi
done

echo
echo "=== Copying Robot_*.cpp files into $ROBOTS_DIR ==="

shopt -s nullglob
for repo in "$DEST"/*; do
    [ -d "$repo" ] || continue

    echo "Processing repo: $repo"

    # Look for Robot_*.cpp in the repo root
    for robot_src in "$repo"/Robot_*.cpp; do
        base_src=$(basename "$robot_src")
        dest_path="$ROBOTS_DIR/$base_src"

        echo "  Copying $robot_src -> $dest_path"
        cp "$robot_src" "$dest_path"
    done
done
shopt -u nullglob

echo
echo "=== Cleaning up cloned repos in $DEST ==="
rm -rf "$DEST"

echo
echo "Done. Robot_*.cpp files are now in $ROBOTS_DIR/"
