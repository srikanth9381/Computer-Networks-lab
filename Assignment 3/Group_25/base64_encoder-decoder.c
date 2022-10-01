#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const int SIZE = 1800;

char *encoder(char *input_string)
{
    // The character set for base64 encoding scheme
    char base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int len = strlen(input_string); // Length of the given string as input.
    int padding = 0;                // Used to find the number of '=' to be appended at the end of encoded string.
    int value = 0;                  // Used to store the temporary binary form while encoding.
    int index = 0;                  // Used to keep the track of number of characters in the encoded string.
    int bits_count = 0;             // Used to keep track of number of bits of the variable value.

    // Allocation of memory for the encoded string
    char *encoded_string = (char *)malloc(SIZE * sizeof(char));

    // Here we take 3 characters of the input string at a time
    // and store their value in variable value.

    for (int i = 0; i < len; i += 3)
    {
        value = 0;
        bits_count = 0;

        // Loop to take chunks of 3 characters starting at position i
        for (int j = i; j <= i + 2 && j < len; j++)
        {
            value = value << 8;              // Shift value by 8 bits as each character in the input string takes up 8 bits in binary form.
            value = value | input_string[j]; // Set the appropriate bits of the variable value.
            bits_count += 8;                 // Increment the bits_count by 8.
        }

        if (bits_count == 8)
        {
            // Append 4 zeros in the binary representation of value
            value = value << 4;
            bits_count += 4;
            padding = 2;
        }
        else if (bits_count == 16)
        {
            // Append 2 zeros in the binary representation of value
            value = value << 2;
            bits_count += 2;
            padding = 1;
        }

        for (int j = 6; j <= bits_count; j += 6)
        {
            // Convert the chunks of 6 bits into a base64 encoded character.

            int temp = value >> (bits_count - j);      // Consider the first j bits.
            int six_bits = temp % 64;                  // Of the above number consider the 6 Lowest significant bits
            char ch = base64[six_bits];                // Find appropriate character for the 6 bit code in base64 system.
            encoded_string[index++] = ch;              // Assign the character in the encoded string.
        }
    }

    for (int j = 0; j < padding; j++)
    {
        encoded_string[index++] = '=';
    }
    encoded_string[index] = '\0';

    return encoded_string;
}

char *decoder(char *encoded_string)
{
    int len = strlen(encoded_string); // Length of the given encoded string as input.
    int padding = 0;                  // The padding in the encoded decoded_string
    int value = 0;                    // Used to store the temporary binary form while decoding.
    int index = 0;                    // Used to keep the track of number of characters in the decoded string.
    int bits_count = 0;               // Used to keep track of number of bits of the variable value.

    // Allocation of memory for the decoded string
    char *decoded_string = (char *)malloc(SIZE * sizeof(char));

    // Here we take 4 characters of the encoded string at a time
    // and store their value in variable value.

    for (int i = 0; i < len; i += 4)
    {
        value = 0;
        bits_count = 24;
        padding = 0;

          // Loop to take chunks of 4 characters starting at position i
        for (int j = i; j <= i + 3 && j < len; j++)
        {
            int base64_character_index; // Used to store the index of base64 character

            if (isupper(encoded_string[j]))
            {
                base64_character_index = encoded_string[j] - 'A';
            }
            else if (islower(encoded_string[j]))
            {
                base64_character_index = encoded_string[j] - 'a' + 26;
            }
            else if (isdigit(encoded_string[j]))
            {
                base64_character_index = encoded_string[j] - '0' + 52;
            }
            else if (encoded_string[j] == '+')
            {
                base64_character_index = 62;
            }
            else if (encoded_string[j] == '/')
            {
                base64_character_index = 63;
            }
            else if (encoded_string[j] == '=')
            {
                padding++;
                continue;
            }

            // Shift value by 6 bits as each character in the encoded string takes up 6 bits in binary form.
            value = value << 6;
            value = value | base64_character_index;
        }

        if (padding == 2)
        {
            // Remove the 4 lowest significant bits in the binary representation of value
            value = value >> 4;
            bits_count = 8;
        }
        else if (padding == 1)
        {
            // Remove the 4 lowest significant bits in the binary representation of value
            value = value >> 2;
            bits_count = 16;
        }

        for (int j = 8; j <= bits_count; j += 8)
        {
            // Convert the chunks of 8 bits into a ASCII character.

            int temp = value >> (bits_count - j);        // Consider the first j bits.
            int eight_bits = temp % 256;                 // Of the above number consider the 8 Lowest significant bits
            decoded_string[index++] = (char)eight_bits;  // Assign the ASCII character in the decoded string.
        }
    }

    decoded_string[index] = '\0';
    
    return decoded_string;
}
