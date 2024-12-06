#include "fat32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DIR_ENTRIES 16
OpenFile open_files[MAX_OPEN_FILES];

uint32_t cluster_to_sector(FAT32BootSector *bs, uint32_t cluster) {
    uint32_t first_data_sector = bs->reserved_sector_count + (bs->num_fats * bs->fat_size_32);
    return ((cluster - 2) * bs->sectors_per_cluster) + first_data_sector;
}

void read_cluster(FILE *image, FAT32BootSector *bs, uint32_t cluster, void *buffer) {
    uint32_t sector = cluster_to_sector(bs, cluster);
    fseek(image, sector * bs->bytes_per_sector, SEEK_SET);
    fread(buffer, bs->bytes_per_sector, bs->sectors_per_cluster, image);
}

void write_cluster(FILE *image, FAT32BootSector *bs, uint32_t cluster, const void *buffer) {
    uint32_t sector = cluster_to_sector(bs, cluster);
    fseek(image, sector * bs->bytes_per_sector, SEEK_SET);
    fwrite(buffer, bs->bytes_per_sector, bs->sectors_per_cluster, image);
}

void print_boot_sector_info(FAT32BootSector *bs) {
    uint32_t total_data_clusters = (bs->total_sectors_32 - bs->reserved_sector_count - (bs->num_fats * bs->fat_size_32)) / bs->sectors_per_cluster;

    printf("Position of root cluster (in cluster #): %u\n", bs->root_cluster);
    printf("Bytes per sector: %u\n", bs->bytes_per_sector);
    printf("Sectors per cluster: %u\n", bs->sectors_per_cluster);
    printf("Total number of clusters in data region: %u\n", total_data_clusters);
    printf("Number of entries in one FAT: %u\n", bs->fat_size_32 * bs->bytes_per_sector / 4);
    printf("Size of image (in bytes): %u\n", bs->total_sectors_32 * bs->bytes_per_sector);
}

void handle_ls_command(FILE *image, FAT32BootSector *bs, uint32_t cluster) {
    DirectoryEntry entries[MAX_DIR_ENTRIES];
    read_cluster(image, bs, cluster, entries);

    printf("Listing directory contents:\n");
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            // No more entries
            break;
        }
        if ((entries[i].attr & 0x0F) == 0x0F) {
            // Long file name entry, skip it
            continue;
        }
        if (entries[i].name[0] == 0xE5) {
            // Deleted entry, skip it
            continue;
        }

        char name[12];
        memcpy(name, entries[i].name, 11);
        name[11] = '\0';

        // Remove trailing spaces from the name
        for (int j = 10; j >= 0; j--) {
            if (name[j] != ' ') {
                name[j + 1] = '\0';
                break;
            }
        }

        if ((entries[i].attr & 0x10) == 0x10) {
            printf("[DIR] %s\n", name);
        } else {
            printf("[FILE] %s\n", name);
        }
    }
}

uint32_t find_directory_cluster(FILE *image, FAT32BootSector *bs, uint32_t cluster, const char *dirname) {
    DirectoryEntry entries[MAX_DIR_ENTRIES];
    read_cluster(image, bs, cluster, entries);

    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            // No more entries
            break;
        }
        if ((entries[i].attr & 0x0F) == 0x0F || entries[i].name[0] == 0xE5) {
            // Long file name entry or deleted entry, skip it
            continue;
        }

        char name[12];
        memcpy(name, entries[i].name, 11);
        name[11] = '\0';

        // Remove trailing spaces from the name
        for (int j = 10; j >= 0; j--) {
            if (name[j] != ' ') {
                name[j + 1] = '\0';
                break;
            }
        }

        if (strcmp(name, dirname) == 0) {
            if ((entries[i].attr & 0x10) == 0x10) {
                // Directory found, return its cluster number
                return (entries[i].firstclusthi << 16) | entries[i].firstclustlo;
            } else {
                // Found entry with matching name but it is not a directory
                return 0;
            }
        }
    }

    // Directory not found
    return 0;
}

void handle_cd_command(FILE *image, FAT32BootSector *bs, uint32_t *current_cluster, uint32_t *parent_cluster, char *current_path, const char *dirname) {
    if (strcmp(dirname, ".") == 0) {
        // Stay in the current directory
        return;
    } else if (strcmp(dirname, "..") == 0) {
        if (*current_cluster != bs->root_cluster) {
            // Move to the parent directory
            *current_cluster = *parent_cluster;

            // Update current path
            char *last_slash = strrchr(current_path, '/');
            if (last_slash != NULL) {
                *last_slash = '\0';
            }
        } else {
            printf("Error: Already at the root directory.\n");
        }
        return;
    }

    uint32_t new_cluster = find_directory_cluster(image, bs, *current_cluster, dirname);
    if (new_cluster != 0) {
        // Update parent cluster before changing current directory
        *parent_cluster = *current_cluster;
        *current_cluster = new_cluster;

        // Update current path
        strcat(current_path, "/");
        strcat(current_path, dirname);
    } else {
        printf("Error: Directory '%s' not found or is not a directory.\n", dirname);
    }
}

uint32_t find_free_cluster(FILE *image, FAT32BootSector *bs) {
    uint32_t fat_start = bs->reserved_sector_count * bs->bytes_per_sector;
    uint32_t total_clusters = (bs->total_sectors_32 - fat_start / bs->bytes_per_sector) / bs->sectors_per_cluster;
    uint32_t *fat = (uint32_t *)malloc(total_clusters * sizeof(uint32_t));

    fseek(image, fat_start, SEEK_SET);
    fread(fat, sizeof(uint32_t), total_clusters, image);

    for (uint32_t i = 2; i < total_clusters; i++) {
        if (fat[i] == 0) {
            free(fat);
            return i;
        }
    }

    free(fat);
    return 0;  // No free cluster found
}

void handle_mkdir_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *dirname) {
    DirectoryEntry entries[MAX_DIR_ENTRIES];
    read_cluster(image, bs, current_cluster, entries);

    // Check if DIRNAME already exists
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            break;
        }
        char name[12];
        memcpy(name, entries[i].name, 11);
        name[11] = '\0';

        // Remove trailing spaces from the name
        for (int j = 10; j >= 0; j--) {
            if (name[j] != ' ') {
                name[j + 1] = '\0';
                break;
            }
        }

        if (strcmp(name, dirname) == 0) {
            printf("Error: Directory or file with the name '%s' already exists.\n", dirname);
            return;
        }
    }

    // Find a free cluster for the new directory
    uint32_t new_cluster = find_free_cluster(image, bs);
    if (new_cluster == 0) {
        printf("Error: No free cluster available.\n");
        return;
    }

    // Mark the new cluster as allocated in the FAT
    uint32_t fat_start = bs->reserved_sector_count * bs->bytes_per_sector;
    fseek(image, fat_start + new_cluster * sizeof(uint32_t), SEEK_SET);
    uint32_t end_of_chain = 0xFFFFFFFF;
    fwrite(&end_of_chain, sizeof(uint32_t), 1, image);

    // Create the new directory entry in the current directory
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            memset(entries[i].name, ' ', 11);
            strncpy((char *)entries[i].name, dirname, strlen(dirname));  // Cast to char *
            entries[i].attr = 0x10;  // Directory attribute
            entries[i].firstclusthi = (new_cluster >> 16) & 0xFFFF;
            entries[i].firstclustlo = new_cluster & 0xFFFF;
            entries[i].filesize = 0;
            break;
        }
    }

    // Write updated directory entries back to the current cluster
    write_cluster(image, bs, current_cluster, entries);

    // Initialize the new directory cluster
    DirectoryEntry new_entries[MAX_DIR_ENTRIES] = {0};
    memset(new_entries, 0, sizeof(new_entries));

    // Create '.' entry
    memset(new_entries[0].name, ' ', 11);
    new_entries[0].name[0] = '.';
    new_entries[0].attr = 0x10;
    new_entries[0].firstclusthi = (new_cluster >> 16) & 0xFFFF;
    new_entries[0].firstclustlo = new_cluster & 0xFFFF;
    new_entries[0].filesize = 0;

    // Create '..' entry
    memset(new_entries[1].name, ' ', 11);
    new_entries[1].name[0] = '.';
    new_entries[1].name[1] = '.';
    new_entries[1].attr = 0x10;
    new_entries[1].firstclusthi = (current_cluster >> 16) & 0xFFFF;
    new_entries[1].firstclustlo = current_cluster & 0xFFFF;
    new_entries[1].filesize = 0;

    // Write the new directory entries to the new cluster
    write_cluster(image, bs, new_cluster, new_entries);

    printf("Directory '%s' created successfully.\n", dirname);

    // Verify by reading back the new cluster
    populate_dir(image, bs, new_entries, new_cluster);
    printf("Verifying new directory contents:\n");
    for (int j = 0; j < MAX_DIR_ENTRIES; j++) {
        if (new_entries[j].name[0] == 0x00) {
            break;
        }
        char name[12];
        memcpy(name, new_entries[j].name, 11);
        name[11] = '\0';
        // Remove trailing spaces from the name
        for (int k = 10; k >= 0; k--) {
            if (name[k] != ' ') {
                name[k + 1] = '\0';
                break;
            }
        }
        printf("Entry %d: %s\n", j, name);
    }
}

// Add any missing helper functions
void create_directory_entry(DirectoryEntry *entry, const char *name, uint32_t cluster) {
    memset(entry->name, ' ', 11);
    strncpy((char *)entry->name, name, strlen(name));  // Cast to char *
    for (int i = strlen(name); i < 11; i++) {
        entry->name[i] = ' ';
    }
    entry->attr = 0x10;  // Directory attribute
    entry->firstclusthi = (cluster >> 16) & 0xFFFF;
    entry->firstclustlo = cluster & 0xFFFF;
    entry->filesize = 0;
}

void create_special_entries(DirectoryEntry *entries, uint32_t new_cluster, uint32_t parent_cluster) {
    memset(entries[0].name, ' ', 11);
    entries[0].name[0] = '.';
    entries[0].attr = 0x10;
    entries[0].firstclusthi = (new_cluster >> 16) & 0xFFFF;
    entries[0].firstclustlo = new_cluster & 0xFFFF;
    entries[0].filesize = 0;

    memset(entries[1].name, ' ', 11);
    entries[1].name[0] = '.';
    entries[1].name[1] = '.';
    entries[1].attr = 0x10;
    entries[1].firstclusthi = (parent_cluster >> 16) & 0xFFFF;
    entries[1].firstclustlo = parent_cluster & 0xFFFF;
    entries[1].filesize = 0;
}

void write_fat_entry(FILE *image, FAT32BootSector *bs, uint32_t cluster, uint32_t value) {
    uint32_t fat_start = bs->reserved_sector_count * bs->bytes_per_sector;
    fseek(image, fat_start + cluster * sizeof(uint32_t), SEEK_SET);
    fwrite(&value, sizeof(uint32_t), 1, image);
}

void populate_dir(FILE *image, FAT32BootSector *bs, DirectoryEntry *entries, uint32_t cluster) {
    read_cluster(image, bs, cluster, entries);
}


void handle_creat_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *filename) {
    DirectoryEntry entries[MAX_DIR_ENTRIES];
    read_cluster(image, bs, current_cluster, entries);

    // Check if FILENAME already exists
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            break;
        }
        char name[12];
        memcpy(name, entries[i].name, 11);
        name[11] = '\0';

        // Remove trailing spaces from the name
        for (int j = 10; j >= 0; j--) {
            if (name[j] != ' ') {
                name[j + 1] = '\0';
                break;
            }
        }

        if (strcmp(name, filename) == 0) {
            printf("Error: Directory or file with the name '%s' already exists.\n", filename);
            return;
        }
    }

    // Find a free cluster for the new file
    uint32_t new_cluster = find_free_cluster(image, bs);
    if (new_cluster == 0) {
        printf("Error: No free cluster available.\n");
        return;
    }

    // Mark the new cluster as allocated in the FAT
    write_fat_entry(image, bs, new_cluster, 0xFFFFFFFF);

    // Create the new file entry in the current directory
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            memset(entries[i].name, ' ', 11);
            strncpy((char *)entries[i].name, filename, strlen(filename));  // Cast to char *
            entries[i].attr = 0x20;  // Archive attribute (regular file)
            entries[i].firstclusthi = (new_cluster >> 16) & 0xFFFF;
            entries[i].firstclustlo = new_cluster & 0xFFFF;
            entries[i].filesize = 0;
            break;
        }
    }

    // Write updated directory entries back to the current cluster
    write_cluster(image, bs, current_cluster, entries);

    printf("File '%s' created successfully.\n", filename);
}


void handle_open_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *filename, const char *mode) {
    DirectoryEntry entries[MAX_DIR_ENTRIES];
    read_cluster(image, bs, current_cluster, entries);

    // Check if the file is already open
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (strcmp(open_files[i].filename, filename) == 0) {
            printf("Error: File '%s' is already open.\n", filename);
            return;
        }
    }

    // Check if the file exists and get its cluster number
    uint32_t file_cluster = 0;
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            break;
        }
        char name[12];
        memcpy(name, entries[i].name, 11);
        name[11] = '\0';

        // Remove trailing spaces from the name
        for (int j = 10; j >= 0; j--) {
            if (name[j] != ' ') {
                name[j + 1] = '\0';
                break;
            }
        }

        if (strcmp(name, filename) == 0) {
            if ((entries[i].attr & 0x10) == 0x10) {
                printf("Error: '%s' is a directory.\n", filename);
                return;
            }
            file_cluster = (entries[i].firstclusthi << 16) | entries[i].firstclustlo;
            break;
        }
    }

    if (file_cluster == 0) {
        printf("Error: File '%s' not found.\n", filename);
        return;
    }

    // Check if the mode is valid
    if (strcmp(mode, "-r") != 0 && strcmp(mode, "-w") != 0 && strcmp(mode, "-rw") != 0 && strcmp(mode, "-wr") != 0) {
        printf("Error: Invalid mode '%s'.\n", mode);
        return;
    }

    // Find a free slot in the open files array
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (open_files[i].filename[0] == 0) {
            strncpy(open_files[i].filename, filename, 11);
            open_files[i].filename[11] = '\0'; // Ensure null termination
            open_files[i].cluster = file_cluster;
            strncpy(open_files[i].mode, mode + 1, 2); // Skip the '-' character
            open_files[i].mode[2] = '\0'; // Ensure null termination
            open_files[i].offset = 0;
            printf("File '%s' opened in mode '%s'.\n", filename, mode);
            return;
        }
    }

    printf("Error: Maximum number of open files reached.\n");
}

void handle_close_command(const char *filename) {
    // Check if the file is open
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (strcmp(open_files[i].filename, filename) == 0) {
            // Close the file by resetting its entry
            memset(&open_files[i], 0, sizeof(OpenFile));
            printf("File '%s' closed successfully.\n", filename);
            return;
        }
    }

    // If the file was not found in the open files array, print an error
    printf("Error: File '%s' is not open or does not exist.\n", filename);
}
