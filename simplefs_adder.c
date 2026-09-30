#include "simplefs.h"

void set_bit(unsigned char *bitmap, int index) { bitmap[index / 8] |= (1u << (index % 8)); }
int is_bit_set(unsigned char *bitmap, int index) { return bitmap[index / 8] & (1u << (index % 8)); }
long inode_offset(int inode_number) { return ((long)INODE_TABLE_BLOCK * BLOCK_SIZE) + ((long)(inode_number - 1) * sizeof(inode_t)); }
int data_bitmap_index(int absolute_block_number) { return absolute_block_number - DATA_REGION_BLOCK; }

int find_free_inode(unsigned char *bitmap)
{
    /* TODO 1: Search bitmap indexes 1..31 and return INODE NUMBER. */
    /* TODO: STUDENT CODE START */
    /* Index 0 (inode 1) is always the root, so start scanning from index 1. */
    for (int index = 1; index < TOTAL_INODES; index++) {
        if (!is_bit_set(bitmap, index)) {
            return index + 1; /* inode number = index + 1 */
        }
    }
    /* TODO: STUDENT CODE END */
    return -1;
}

int find_free_data_block(unsigned char *bitmap)
{
    /* TODO 2: First-fit search; return ABSOLUTE data block number. */
    /* TODO: STUDENT CODE START */
    /* Index 0 (absolute block DATA_REGION_BLOCK) is always the root dir block. */
    for (int index = 1; index < DATA_BLOCKS; index++) {
        if (!is_bit_set(bitmap, index)) {
            return DATA_REGION_BLOCK + index;
        }
    }
    /* TODO: STUDENT CODE END */
    return -1;
}

int filename_exists(FILE *image, const char *filename)
{
    dirent_t entry;
    /* TODO 3: Search root directory entries for filename. */
    /* TODO: STUDENT CODE START */
    int max_entries = BLOCK_SIZE / DIRENT_SIZE; /* 64 entries in root block */
    for (int i = 0; i < max_entries; i++) {
        long pos = ((long)ROOT_DATA_BLOCK * BLOCK_SIZE) + ((long)i * sizeof(dirent_t));
        fseek(image, pos, SEEK_SET);
        if (fread(&entry, sizeof(entry), 1, image) != 1) {
            break;
        }
        if (entry.inode_no != 0 && strcmp(entry.name, filename) == 0) {
            return 1;
        }
    }
    /* TODO: STUDENT CODE END */
    return 0;
}

int find_free_directory_entry(FILE *image)
{
    dirent_t entry;
    /* TODO 4: Search entries 2..63; free entry has inode_no == 0. */
    /* TODO: STUDENT CODE START */
    int max_entries = BLOCK_SIZE / DIRENT_SIZE; /* 64 entries in root block */
    for (int i = 2; i < max_entries; i++) {
        long pos = ((long)ROOT_DATA_BLOCK * BLOCK_SIZE) + ((long)i * sizeof(dirent_t));
        fseek(image, pos, SEEK_SET);
        if (fread(&entry, sizeof(entry), 1, image) != 1) {
            break;
        }
        if (entry.inode_no == 0) {
            return i;
        }
    }
    /* TODO: STUDENT CODE END */
    return -1;
}

int main(int argc, char *argv[])
{
    char *image_name = NULL, *source_name = NULL;
    FILE *image, *source;
    superblock_t sb;
    unsigned char inode_bitmap[BLOCK_SIZE], data_bitmap[BLOCK_SIZE];
    inode_t new_inode, root_inode;
    dirent_t new_entry;
    long file_size;
    int required_blocks, free_inode;
    int allocated_blocks[MAX_DIRECT_BLOCKS] = {0};
    int directory_entry_index;

    if (argc != 5) { printf("Usage: %s --input <image> --file <file>\n", argv[0]); return 1; }
    if (strcmp(argv[1], "--input") != 0 || strcmp(argv[3], "--file") != 0) { printf("Error: invalid command-line arguments.\n"); return 1; }
    image_name = argv[2]; source_name = argv[4];

    image = fopen(image_name, "rb+");
    if (!image) { printf("Error: file-system image not found.\n"); return 1; }
    fseek(image, SUPERBLOCK_BLOCK * BLOCK_SIZE, SEEK_SET);
    if (fread(&sb, sizeof(sb), 1, image) != 1) { printf("Error: could not read superblock.\n"); fclose(image); return 1; }
    if (sb.magic != MAGIC_NUMBER) { printf("Error: invalid SimpleFS image.\n"); fclose(image); return 1; }

    source = fopen(source_name, "rb");
    if (!source) { printf("Error: source file not found.\n"); fclose(image); return 1; }
    fseek(source, 0, SEEK_END); file_size = ftell(source); rewind(source);
    if (file_size < 0) { printf("Error: could not determine source file size.\n"); fclose(source); fclose(image); return 1; }
    if (file_size > MAX_FILE_SIZE) { printf("Error: file is too large for SimpleFS.\n"); fclose(source); fclose(image); return 1; }
    if (strlen(source_name) > 58) { printf("Error: file name is too long for SimpleFS.\n"); fclose(source); fclose(image); return 1; }

    /* TODO 5: Calculate required_blocks. Zero-byte file uses zero blocks. */
    /* TODO: STUDENT CODE START */
    required_blocks = (int)((file_size + BLOCK_SIZE - 1) / BLOCK_SIZE);
    /* TODO: STUDENT CODE END */

    if (filename_exists(image, source_name)) { printf("Error: file already exists in SimpleFS.\n"); fclose(source); fclose(image); return 1; }

    fseek(image, INODE_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fread(inode_bitmap, BLOCK_SIZE, 1, image);
    free_inode = find_free_inode(inode_bitmap);
    if (free_inode == -1) { printf("Error: no free inode available.\n"); fclose(source); fclose(image); return 1; }

    fseek(image, DATA_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fread(data_bitmap, BLOCK_SIZE, 1, image);

    /* TODO 6: Allocate required data blocks and mark them in memory. */
    /* TODO: STUDENT CODE START */
    for (int i = 0; i < required_blocks; i++) {
        int block = find_free_data_block(data_bitmap);
        if (block == -1) {
            printf("Error: not enough free data blocks available.\n");
            fclose(source); fclose(image);
            return 1;
        }
        allocated_blocks[i] = block;
        set_bit(data_bitmap, data_bitmap_index(block));
    }
    /* TODO: STUDENT CODE END */

    directory_entry_index = find_free_directory_entry(image);
    if (directory_entry_index == -1) { printf("Error: root directory is full.\n"); fclose(source); fclose(image); return 1; }

    /* TODO 7: Copy source contents into allocated blocks using zero-filled buffers. */
    /* TODO: STUDENT CODE START */
    for (int i = 0; i < required_blocks; i++) {
        unsigned char buffer[BLOCK_SIZE];
        memset(buffer, 0, BLOCK_SIZE); /* ensures unused tail bytes stay zero */
        fread(buffer, 1, BLOCK_SIZE, source);
        fseek(image, (long)allocated_blocks[i] * BLOCK_SIZE, SEEK_SET);
        fwrite(buffer, BLOCK_SIZE, 1, image);
    }
    /* TODO: STUDENT CODE END */

    /* TODO 8: Initialize new file inode and its direct pointers. */
    memset(&new_inode, 0, sizeof(new_inode));
    /* TODO: STUDENT CODE START */
    new_inode.type = TYPE_FILE;
    new_inode.links = 1;
    new_inode.size = (uint32_t)file_size;
    for (int i = 0; i < required_blocks; i++) {
        new_inode.direct[i] = (uint32_t)allocated_blocks[i];
    }
    /* remaining direct[] entries stay 0 from memset above */
    /* TODO: STUDENT CODE END */
    fseek(image, inode_offset(free_inode), SEEK_SET);
    fwrite(&new_inode, sizeof(new_inode), 1, image);

    /* TODO 9: Mark allocated inode in inode bitmap. */
    /* TODO: STUDENT CODE START */
    set_bit(inode_bitmap, free_inode - 1); /* inode number -> bitmap index */
    /* TODO: STUDENT CODE END */
    fseek(image, INODE_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(inode_bitmap, BLOCK_SIZE, 1, image);
    fseek(image, DATA_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(data_bitmap, BLOCK_SIZE, 1, image);

    /* TODO 10: Create directory entry; ensure name is null-terminated. */
    memset(&new_entry, 0, sizeof(new_entry));
    /* TODO: STUDENT CODE START */
    new_entry.inode_no = (uint32_t)free_inode;
    new_entry.type = TYPE_FILE;
    strncpy(new_entry.name, source_name, sizeof(new_entry.name) - 1);
    new_entry.name[sizeof(new_entry.name) - 1] = '\0';
    /* TODO: STUDENT CODE END */
    {
        long pos = ((long)ROOT_DATA_BLOCK * BLOCK_SIZE) + ((long)directory_entry_index * sizeof(dirent_t));
        fseek(image, pos, SEEK_SET);
        fwrite(&new_entry, sizeof(new_entry), 1, image);
    }

    fseek(image, inode_offset(ROOT_INODE), SEEK_SET);
    fread(&root_inode, sizeof(root_inode), 1, image);

    /* TODO 11: Increase root_inode.size by sizeof(dirent_t). */
    /* TODO: STUDENT CODE START */
    root_inode.size += (uint32_t)sizeof(dirent_t);
    /* TODO: STUDENT CODE END */
    fseek(image, inode_offset(ROOT_INODE), SEEK_SET);
    fwrite(&root_inode, sizeof(root_inode), 1, image);

    fclose(source); fclose(image);
    printf("%s added successfully to %s\n", source_name, image_name);
    return 0;
}
