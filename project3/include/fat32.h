#pragma once
#include <stdint.h>
#include <stdio.h>  // Include stdio.h for FILE type

#ifndef FAT32_H
#define FAT32_H

// Define FAT32 Boot Sector structure
typedef struct {
    uint8_t jump_boot[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} __attribute__((packed)) FAT32BootSector;

// Define Directory Entry structure
typedef struct {
    uint8_t name[11];
    uint8_t attr;
    uint8_t ntres;
    uint8_t crttimetenth;
    uint16_t crttime;
    uint16_t crtdate;
    uint16_t lastaccesdate;
    uint16_t firstclusthi;
    uint16_t wrttime;
    uint16_t wrtdate;
    uint16_t firstclustlo;
    uint32_t filesize;
} __attribute__((packed)) DirectoryEntry;

typedef struct {
    char filename[12];
    uint32_t cluster;
    char mode[3];
    uint32_t offset;
} OpenFile;

#define MAX_OPEN_FILES 10
extern OpenFile open_files[MAX_OPEN_FILES];


// Function declarations
void handle_ls_command(FILE *image, FAT32BootSector *bs, uint32_t cluster);
void print_boot_sector_info(FAT32BootSector *bs);
uint32_t cluster_to_sector(FAT32BootSector *bs, uint32_t cluster);
void read_cluster(FILE *image, FAT32BootSector *bs, uint32_t cluster, void *buffer);
void write_cluster(FILE *image, FAT32BootSector *bs, uint32_t cluster, const void *buffer);
uint32_t find_directory_cluster(FILE *image, FAT32BootSector *bs, uint32_t cluster, const char *dirname);
void handle_cd_command(FILE *image, FAT32BootSector *bs, uint32_t *current_cluster, uint32_t *parent_cluster, char *current_path, const char *dirname);
void handle_mkdir_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *dirname);
uint32_t find_free_cluster(FILE *image, FAT32BootSector *bs);
void write_fat_entry(FILE *image, FAT32BootSector *bs, uint32_t cluster, uint32_t value);
void populate_dir(FILE *image, FAT32BootSector *bs, DirectoryEntry *entries, uint32_t cluster);
void create_directory_entry(DirectoryEntry *entry, const char *name, uint32_t cluster);
void create_special_entries(DirectoryEntry *entries, uint32_t new_cluster, uint32_t parent_cluster);

void read_directory_entry(FILE *fp, DirectoryEntry *entry);
void write_directory_entry(FILE *fp);
void handle_creat_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *filename);
void handle_open_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *filename, const char *mode);
void handle_close_command(const char *filename);




#endif // FAT32_H
