#include "allocator.c"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const char registers[][3] = {
    // Byte registers
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bl",
    // Word registers
    "ax",
    "cx",
    "dx",
    "bx",
    "sp",
    "bp",
    "si",
    "di",
};

size_t read_byte(uint8_t *buffer, FILE *input)
{
    return fread(buffer, sizeof(uint8_t), 1, input);
}

void decode_asm(FILE *input, FILE *output)
{
    fprintf(output, "bits 16\n\n");

    uint8_t read;
    while (read_byte(&read, input))
    {
        uint8_t op_code = read >> 2;
        bool d = read >> 1 & 1;
        bool w = read & 1;

        if (op_code == 0b100010)
        {
            read_byte(&read, input);
            uint8_t mod = read >> 6;
            uint8_t reg = read >> 3 & 0b111;
            uint8_t rm = read & 0b111;

            if (mod == 0b11)
            {
                uint8_t reg_start = w << 3;
                bool not_d = !d;

                uint8_t dest = reg_start + reg * d + rm * not_d;
                uint8_t src = reg_start + reg * not_d + rm * d;

                fprintf(output, "mov %s, %s\n", registers[dest], registers[src]);
            }
            else
            {
                fprintf(output, "; unsupported mov op (mod other than 11)\n");
            }
        }
        else
        {
            fprintf(output, "; unknown op code\n");
        }
    }
}

void main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Please provide a file name.");
        return;
    }

    char *input_file_name = argv[1];
    FILE *input_file = fopen(input_file_name, "rb");

    if (!input_file)
    {
        printf("File %s not found.", input_file_name);
        return;
    }

    bump_allocator allocator = create_bump_allocator(1024);

    size_t output_file_name_length = strlen(input_file_name) + 9;
    char *output_file_name;
    ALLOC_BUMP(output_file_name, output_file_name_length, &allocator);
    strcpy(output_file_name, input_file_name);
    strcat(output_file_name, "_dec.asm");

    FILE *output_file = fopen(output_file_name, "w");
    clear_bump_allocator(&allocator);

    decode_asm(input_file, output_file);

    fclose(input_file);
    fclose(output_file);
}
