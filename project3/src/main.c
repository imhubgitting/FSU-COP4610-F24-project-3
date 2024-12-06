#include "lexer.h"
#include "fat32.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Implement command functions
void handle_info_command(FAT32BootSector *bs) {
    print_boot_sector_info(bs);
}

void handle_exit_command(FILE *image) {
    fclose(image);
    printf("Exiting...\n");
}

void handle_ls_command(FILE *image, FAT32BootSector *bs, uint32_t cluster);

void handle_cd_command(FILE *image, FAT32BootSector *bs, uint32_t *current_cluster, uint32_t *parent_cluster, char *current_path, const char *dirname);

void handle_mkdir_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *dirname);

void handle_creat_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *filename);

void handle_open_command(FILE *image, FAT32BootSector *bs, uint32_t current_cluster, const char *filename, const char *mode);

void handle_close_command(const char *filename);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_file>\n", argv[0]);
        return 1;
    }

    FILE *image = fopen(argv[1], "rb");
    if (!image) {
        perror("Error opening image file");
        return 1;
    }

    FAT32BootSector bs;
    fseek(image, 0, SEEK_SET);
    fread(&bs, sizeof(FAT32BootSector), 1, image);

    uint32_t current_cluster = bs.root_cluster;
    uint32_t parent_cluster = bs.root_cluster; // Root is its own parent initially
    char current_path[256] = "";  // Initialize the current path with the image name

    snprintf(current_path, sizeof(current_path), "%s", argv[1]);

    // Initialize open files array
    memset(open_files, 0, sizeof(open_files));

    char *input;
    tokenlist *tokens;
    while (1) {
        printf("[%s]/>", current_path);
        input = get_input();
        tokens = get_tokens(input);

        if (tokens->size > 0) {
            if (strcmp(tokens->items[0], "info") == 0) {
                handle_info_command(&bs);
            } else if (strcmp(tokens->items[0], "ls") == 0) {
                handle_ls_command(image, &bs, current_cluster);
            } else if (strcmp(tokens->items[0], "cd") == 0) {
                if (tokens->size == 2) {
                    handle_cd_command(image, &bs, &current_cluster, &parent_cluster, current_path, tokens->items[1]);
                } else {
                    printf("Error: Incorrect number of arguments for 'cd' command.\n");
                }
            } else if (strcmp(tokens->items[0], "mkdir") == 0) {
                if (tokens->size == 2) {
                    handle_mkdir_command(image, &bs, current_cluster, tokens->items[1]);
                } else {
                    printf("Error: Incorrect number of arguments for 'mkdir' command.\n");
                }
            } else if (strcmp(tokens->items[0], "creat") == 0) {
                if (tokens->size == 2) {
                    handle_creat_command(image, &bs, current_cluster, tokens->items[1]);
                } else {
                    printf("Error: Incorrect number of arguments for 'creat' command.\n");
                }
            } else if (strcmp(tokens->items[0], "open") == 0) {
                if (tokens->size == 3) {
                    handle_open_command(image, &bs, current_cluster, tokens->items[1], tokens->items[2]);
                } else {
                    printf("Error: Incorrect number of arguments for 'open' command.\n");
                }
            } else if (strcmp(tokens->items[0], "close") == 0) {
                if (tokens->size == 2) {
                    handle_close_command(tokens->items[1]);
                } else {
                    printf("Error: Incorrect number of arguments for 'close' command.\n");
                }
            } else if (strcmp(tokens->items[0], "exit") == 0) {
                handle_exit_command(image);
                free_tokens(tokens);
                free(input);
                break;
            } else {
                printf("Unknown command\n");
            }
        }

        free_tokens(tokens);
        free(input);
    }

    return 0;
}
