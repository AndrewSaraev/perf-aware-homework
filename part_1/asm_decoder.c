#include "allocator.c"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

char *registers[] = {
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
    // Effective address registers
    "bx + si",
    "bx + di",
    "bp + si",
    "bp + di",
    "si",
    "di",
    "bp",
    "bx",
};

size_t read_byte(uint8_t *buffer, FILE *input)
{
    return fread(buffer, sizeof(uint8_t), 1, input);
}

int16_t read_sign_extended(FILE *input, bool word)
{
    if (word)
    {
        int16_t output;
        fread(&output, sizeof(int16_t), 1, input);
        return output;
    }
    else
    {
        int8_t output;
        fread(&output, sizeof(int8_t), 1, input);
        return output;
    }
}

int16_t read_disp(FILE *input, uint8_t mod)
{
    if (mod)
    {
        return read_sign_extended(input, mod == 0b10);
    }
    else
    {
        return 0;
    }
}

void output_effective_address(FILE *input, FILE *output, uint8_t mod, uint8_t r_m)
{
    if (r_m == 0b110 && mod == 0b00)
    {
        int16_t disp;
        fread(&disp, sizeof(int16_t), 1, input);
        fprintf(output, "[%d]", disp);
    }
    else
    {
        char *reg = registers[16 + r_m];
        int16_t disp = read_disp(input, mod);
        if (disp)
        {
            if (disp > 0)
            {
                fprintf(output, "[%s + %d]", reg, disp);
            }
            else
            {
                fprintf(output, "[%s - %d]", reg, -disp);
            }
        }
        else
        {
            fprintf(output, "[%s]", reg);
        }
    }
}

void decode_asm(FILE *input, FILE *output)
{
    fprintf(output, "bits 16\n\n");

    uint8_t read;
    while (read_byte(&read, input))
    {
        if (read >> 2 == 0b100010)
        {
            bool d = read >> 1 & 1;
            bool w = read & 1;

            read_byte(&read, input);
            uint8_t mod = read >> 6;
            uint8_t reg = read >> 3 & 0b111;
            uint8_t r_m = read & 0b111;

            uint8_t reg_mask = w << 3;
            char *reg_name = registers[reg_mask | reg];

            if (mod == 0b11) // Register mode
            {
                char *r_m_name = registers[reg_mask | r_m];
                if (d)
                {
                    fprintf(output, "mov %s, %s\n", reg_name, r_m_name);
                }
                else
                {
                    fprintf(output, "mov %s, %s\n", r_m_name, reg_name);
                }
            }
            else // Memory mode
            {
                if (d)
                {
                    fprintf(output, "mov %s, ", reg_name);
                    output_effective_address(input, output, mod, r_m);
                    fprintf(output, "\n");
                }
                else
                {
                    fprintf(output, "mov ");
                    output_effective_address(input, output, mod, r_m);
                    fprintf(output, ", %s\n", reg_name);
                }
            }
        }
        /* else if (read >> 1 == 0b1100011)
        {
            bool w = read & 1;

            read_byte(&read, input);
            uint8_t mod = read >> 6;
            uint8_t r_m = read & 0b111;
        } */
        else if (read >> 4 == 0b1011)
        {
            bool w = read >> 3 & 1;
            uint8_t reg = read & 0b111;

            char *reg_name = registers[w << 3 | reg];
            int16_t data = read_sign_extended(input, w);
            fprintf(output, "mov %s, %d\n", reg_name, data);
        }
        else
        {
            fprintf(output, "; unknown op code\n");
        }
    }
}

void main(int argc, char **argv)
{
    clock_t timer_start = clock();

    if (argc < 2)
    {
        printf("A file name is expected as an argument.");
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

    clock_t timer_end = clock();
    double timer_duration = (double)(timer_end - timer_start) / CLOCKS_PER_SEC;
    printf("Decoded instructions from %s in %f seconds.", input_file_name, timer_duration);
}
