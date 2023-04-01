#include "memory.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

char *registers[] =
{
    // Byte registers
    "al",
    "cl",
    "dl",
    "bl",
    "ah",
    "ch",
    "dh",
    "bh",
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

char *address_calculations[] =
{
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
        char *reg = address_calculations[r_m];
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

void decode_mod_reg_r_m_disp(FILE *input, FILE *output, uint8_t byte0)
{
    bool d = byte0 >> 1 & 1;
    bool w = byte0 & 1;

    uint8_t byte1;
    read_byte(&byte1, input);
    uint8_t mod = byte1 >> 6;
    uint8_t reg = byte1 >> 3 & 0b111;
    uint8_t r_m = byte1 & 0b111;

    uint8_t reg_mask = w << 3;
    char *reg_name = registers[reg_mask | reg];
    if (mod == 0b11) // Register mode
    {
        char *r_m_name = registers[reg_mask | r_m];
        if (d)
        {
            fprintf(output, "%s, %s\n", reg_name, r_m_name);
        }
        else
        {
            fprintf(output, "%s, %s\n", r_m_name, reg_name);
        }
    }
    else // Memory mode
    {
        if (d)
        {
            fprintf(output, "%s, ", reg_name);
            output_effective_address(input, output, mod, r_m);
            fprintf(output, "\n");
        }
        else
        {
            output_effective_address(input, output, mod, r_m);
            fprintf(output, ", %s\n", reg_name);
        }
    }
}

void decode_immediate(FILE *input, FILE *output, bool word_reg, bool word_data, uint8_t byte1)
{
    uint8_t mod = byte1 >> 6;
    uint8_t r_m = byte1 & 0b111;

    uint8_t reg_mask = word_reg << 3;
    char *r_m_name = registers[reg_mask | r_m];

    if (mod == 0b11) // Register mode
    {
        int16_t data = read_sign_extended(input, word_data);
        fprintf(output, "%s, %d\n", r_m_name, data);
    }
    else // Memory mode
    {
        if (word_reg)
        {
            fprintf(output, "word ");
        }
        else
        {
            fprintf(output, "byte ");
        }
        output_effective_address(input, output, mod, r_m);
        int16_t data = read_sign_extended(input, word_data);
        fprintf(output, ", %d\n", data);
    }
}

void write_unrecognized_byte(FILE *output, uint8_t byte)
{
    fprintf(output, "; %02x\n", byte);
}

void decode_asm(FILE *input, FILE *output)
{
    fprintf(output, "bits 16\n\n");

    uint8_t byte0;
    while (read_byte(&byte0, input))
    {
        // Reg/memory and register to either
        if (byte0 >> 2 == 0b100010)
        {
            fprintf(output, "mov ");
            decode_mod_reg_r_m_disp(input, output, byte0);
        }
        else if (byte0 >> 2 == 0b000000)
        {
            fprintf(output, "add ");
            decode_mod_reg_r_m_disp(input, output, byte0);
        }
        else if (byte0 >> 2 == 0b001010)
        {
            fprintf(output, "sub ");
            decode_mod_reg_r_m_disp(input, output, byte0);
        }
        else if (byte0 >> 2 == 0b001110)
        {
            fprintf(output, "cmp ");
            decode_mod_reg_r_m_disp(input, output, byte0);
        }

        // Immediate from register/memory
        else if (byte0 >> 1 == 0b1100011)
        {
            bool word = byte0 & 1;

            uint8_t byte1;
            read_byte(&byte1, input);
            uint8_t reg = byte1 >> 3 & 0b111;

            if (reg == 0b000)
            {
                fprintf(output, "mov ");
                decode_immediate(input, output, word, word, byte1);
            }
            else
            {
                write_unrecognized_byte(output, byte0);
                write_unrecognized_byte(output, byte1);
            }
        }
        else if (byte0 >> 2 == 0b100000)
        {
            bool word_reg = byte0 & 1;
            bool word_data = byte0 & 0b11 == 0b01;

            uint8_t byte1;

            read_byte(&byte1, input);
            uint8_t reg = byte1 >> 3 & 0b111;

            if (reg == 0b000)
            {
                fprintf(output, "add ");
                decode_immediate(input, output, word_reg, word_data, byte1);
            }
            else if (reg == 0b101)
            {
                fprintf(output, "sub ");
                decode_immediate(input, output, word_reg, word_data, byte1);
            }
            else if (reg == 0b111)
            {
                fprintf(output, "cmp ");
                decode_immediate(input, output, word_reg, word_data, byte1);
            }
            else
            {
                write_unrecognized_byte(output, byte0);
                write_unrecognized_byte(output, byte1);
            }
        }
        else if (byte0 >> 4 == 0b1011)
        {
            bool w = byte0 >> 3 & 1;
            uint8_t reg = byte0 & 0b111;

            char *reg_name = registers[w << 3 | reg];
            int16_t data = read_sign_extended(input, w);
            fprintf(output, "mov %s, %d\n", reg_name, data);
        }
        else if (byte0 >> 2 == 0b101000)
        {
            bool d = byte0 >> 1 & 1;
            bool w = byte0 & 1;

            char *reg_name = registers[w << 3];
            uint16_t addr;
            fread(&addr, sizeof(uint16_t), 1, input);
            if (d)
            {
                fprintf(output, "mov [%d], %s\n", addr, reg_name);
            }
            else
            {
                fprintf(output, "mov %s, [%d]\n", reg_name, addr);
            }
        }
        else
        {
            write_unrecognized_byte(output, byte0);
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

    memory memory = alloc_memory(1024);

    size_t output_file_name_length = strlen(input_file_name) + 9;
    LET(char, output_file_name, output_file_name_length, &memory);
    strcpy(output_file_name, input_file_name);
    strcat(output_file_name, "_dec.asm");

    FILE *output_file = fopen(output_file_name, "w");

    decode_asm(input_file, output_file);

    fclose(input_file);
    fclose(output_file);

    clock_t timer_end = clock();
    double timer_duration = (double)(timer_end - timer_start) / CLOCKS_PER_SEC;
    printf("Decoded instructions from %s in %f seconds.", input_file_name, timer_duration);
}
