#include <stdio.h>
#include <string.h>

typedef unsigned char u8;

size_t read_byte(u8 *buffer, FILE *input) {
   return fread(buffer, sizeof(u8), 1, input);
}

void decode_asm(FILE *input, FILE *output) {
   fprintf(output, "bits 16\n\n");

   u8 read;
   while (read_byte(&read, input)) {
      u8 op_code = read >> 2;
      u8 d = read >> 1 & 1;
      u8 w = read & 1;

      if (op_code == 0b100010) { // mov
         read_byte(&read, input);
         u8 mod = read >> 6;
         u8 reg = read >> 3 & 0b111;
         u8 rm = read & 0b111;

         fprintf(output, "mov !\n");
      }
   }
}

int main(int argc, char **argv) {
   if (argc < 2) {
      printf("Please provide a file name.");
      return 0;
   }

   char *input_file_name = argv[1];
   FILE *input_file = fopen(input_file_name, "rb");

   if (!input_file)
   {
      printf("File \"%s\" not found.", input_file_name);
      return 0;
   }

   char output_file_name[64];
   strcpy(output_file_name, input_file_name);
   strcat(output_file_name, "_dec.asm");

   FILE *output_file = fopen(output_file_name, "w");

   decode_asm(input_file, output_file);

   fclose(input_file);
   fclose(output_file);

   return 0;
}
