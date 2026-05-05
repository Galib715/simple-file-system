cat > README.md << 'EOF'
# Simple File System (SFS)

A simple file system built in C with a web-based GUI visualizer.
Created for CSE323 Operating Systems at North South University.

## What it does
Simulates how a real file system works, built completely from scratch in C.
Creates a virtual disk (disk.img) and manages everything manually including
storing files, folders, and tracking free space, just like a real OS does.

## Features
- Virtual disk image with superblock, inode table, bitmap and data blocks
- Create, read, write, delete files and directories
- Nested folder support
- Web GUI showing disk block map and directory tree in real time

## How to run
gcc src/sfs.c src/main.c -o sfs
./sfs format
python3 gui/server.py

Then open http://localhost:5000 in your browser.
EOF
git add README.md
git commit -m "Add README"
git push

YouTube-> https://youtu.be/sLUUxasCpj0
