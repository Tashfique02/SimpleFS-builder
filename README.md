CSE 321: Operating Systems
SimpleFS Lab Term Project - Summer 2026

------------------------------------------------------------
FILES SUBMITTED
------------------------------------------------------------
simplefs.h          - fixed constants and structures (unmodified)
simplefs_builder.c  - creates and initializes an empty SimpleFS image
simplefs_adder.c    - adds a regular file to an existing SimpleFS image
README.txt          - this file

------------------------------------------------------------
COMPILATION
------------------------------------------------------------
gcc -Wall -Wextra -std=c11 simplefs_builder.c -o simplefs_builder
gcc -Wall -Wextra -std=c11 simplefs_adder.c -o simplefs_adder

Both programs compile cleanly with no warnings under -Wall -Wextra -std=c11.

------------------------------------------------------------
EXECUTION EXAMPLES
------------------------------------------------------------
./simplefs_builder --image disk.img
./simplefs_adder --input disk.img --file test1.txt
./simplefs_adder --input disk.img --file test2.txt
./simplefs_adder --input disk.img --file test3.txt

Inspecting the image afterward:
od -An -tx1 -N128 disk.img          (or: xxd -l 128 disk.img)

------------------------------------------------------------
IMPLEMENTATION DESCRIPTION
------------------------------------------------------------
simplefs_builder.c
- Creates a 262144-byte (64 x 4096) image and zero-fills it.
- Fills the superblock (magic 0x53465331, block size 4096, 64 total
  blocks, 32 inodes, and the fixed block numbers for the inode bitmap,
  data bitmap, inode table, and data region) and writes it to Block 0.
- Sets bit 0 of the inode bitmap (Block 1) to mark inode 1 (root)
  allocated, and bit 0 of the data bitmap (Block 2) to mark Block 4
  (the root directory's data block) allocated.
- Initializes the root inode (type = directory, links = 2, size = 128,
  direct[0] = 4) and writes it to the first inode slot in Block 3.
- Writes the "." and ".." directory entries (both pointing to inode 1)
  at the start of Block 4.

simplefs_adder.c
- Opens the image, validates the superblock magic number, and opens
  the source file from the current working directory.
- Rejects source files larger than 12288 bytes (3 direct blocks) and
  file names longer than 58 characters.
- find_free_inode() / find_free_data_block() implement first-fit
  allocation by scanning the in-memory bitmap copies starting after
  the entries reserved for the root (inode 1 / data block 4).
- filename_exists() and find_free_directory_entry() scan the 64
  possible root-directory entry slots (entries 0 and 1 are always
  "." and ".."), rejecting duplicate names and reporting a full
  directory when no free slot remains.
- required_blocks is computed as ceil(file_size / BLOCK_SIZE); a
  0-byte file correctly requires 0 blocks.
- File contents are copied block-by-block through a zero-filled
  4096-byte buffer, so any unused tail bytes in the final block stay
  zero.
- The new inode is written with type = file, links = 1, the exact
  source-file size, and direct[] pointing at the allocated absolute
  block numbers (remaining entries left at 0).
- The inode bitmap, data bitmap, new directory entry, and the root
  inode's size (increased by sizeof(dirent_t) = 64 bytes per file
  added) are all updated and written back to the image.

------------------------------------------------------------
TESTING PERFORMED
------------------------------------------------------------
All test cases from the project specification (Section 19) were run
and verified with od/hexdump against the expected byte values:
- Empty image is exactly 262144 bytes; superblock, inode bitmap
  (0x01), data bitmap (0x01), and root directory ("." / "..") are
  correct immediately after simplefs_builder.
- Adding a one-block file (test1.txt) correctly sets inode bitmap to
  0x03 and data bitmap to 0x03, and the file's text is verified at
  Block 5.
- Adding a two-block ~5000-byte file allocates Blocks 5 and 6
  (data bitmap 0x07) with direct[0]=5, direct[1]=6.
- A 12288-byte file is accepted; a 12289-byte file is rejected with
  an error message and no crash.
- Adding three files in sequence allocates inodes 2, 3, 4 in order
  (inode bitmap 0x0f).
- Re-adding an existing file name is rejected ("file already exists
  in SimpleFS").
- Adding a non-existent source file and running the adder against a
  non-existent image both print a clear error and exit normally
  (no segmentation fault).
- A 0-byte file is accepted, uses 0 data blocks, and leaves the data
  bitmap unchanged aside from the root block.

------------------------------------------------------------
KNOWN LIMITATIONS
------------------------------------------------------------
- As specified, SimpleFS supports only a single flat root directory,
  no subdirectories, no file deletion/renaming, no indirect blocks,
  and a maximum of 31 regular files (32 inodes minus the root).
- File names are taken exactly as passed on the command line; no
  path handling is performed, per the project specification.
