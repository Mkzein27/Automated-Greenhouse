# Greenhouse Automation System

ESP32-based automated greenhouse control system using ESP-IDF framework.

## Team Division

### Mohamed Khaled
**Responsible for:**
Sensor modules and data filtration
 - 

### Abdelrahman Yasser
**Responsible for:**
Main Code 
Control Logic modules
 - 
### Ibrahim Nour
**Responsible for:**
 - 
### Abdelrahman-el-Gamal
**Responsible for:**
Actuator control modules
-


## Building
```bash
# Build
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio device monitor

# Or use PlatformIO buttons in VS Code
```
### 🔄 Complete Daily Workflow
```bash
# Get latest code
git pull

# Create branch for your work
git checkout -b feature/my-feature

# Work on code...
# (edit files in VS Code)

# Check what changed
git status

# Add your changes
git add .

# Commit with descriptive message
git commit -m "Implemented soil moisture sensor filtering"

# Push your branch
git push origin feature/my-feature

# Create Pull Request on GitHub for review
# (done in web browser)
```

---

### 🔍 Checking History and Changes
```bash
# View commit history
git log

# View last 5 commits (concise)
git log --oneline -5

# See what changed in a file
git diff filename.c

# See what changed in last commit
git show

# See who changed what line (blame)
git blame filename.c
```

---

### ↩️ Undoing Changes
```bash
# Undo changes in a file (before adding)
git checkout -- filename.c

# Unstage file (after git add, before commit)
git reset HEAD filename.c

# Undo last commit (keep changes)
git reset --soft HEAD~1

# Undo last commit (discard changes) ⚠️ DANGEROUS
git reset --hard HEAD~1

# Revert a commit (safe - creates new commit)
git revert COMMIT_HASH
```

**⚠️ Warning:** Be careful with `--hard` - it permanently deletes changes!

---



## Pin Assignments

## Hardware

- 
