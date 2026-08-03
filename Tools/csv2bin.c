#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define HEADER_SIZE 7
#define MAGIC_0 'T'
#define MAGIC_1 'C'

typedef struct {
	int* values;
	size_t count;
	size_t capacity;
	size_t width;
	size_t height;
} CsvGrid;

static void free_grid(CsvGrid* grid) {
	if (grid == NULL) {
		return;
	}

	free(grid->values);
	grid->values = NULL;
	grid->count = 0;
	grid->capacity = 0;
	grid->width = 0;
	grid->height = 0;
}

static char* trim(char* text) {
	while (*text != '\0' && isspace((unsigned char)*text)) {
		text++;
	}

	char* end = text + strlen(text);
	while (end > text && isspace((unsigned char)end[-1])) {
		end--;
	}
	*end = '\0';
	return text;
}

static bool reserve_values(CsvGrid* grid, size_t required) {
	if (grid->capacity >= required) {
		return true;
	}

	size_t new_capacity = grid->capacity == 0 ? 256 : grid->capacity;
	while (new_capacity < required) {
		if (new_capacity > SIZE_MAX / 2) {
			new_capacity = required;
			break;
		}
		new_capacity *= 2;
	}

	int* resized = realloc(grid->values, new_capacity * sizeof(*resized));
	if (resized == NULL) {
		return false;
	}

	grid->values = resized;
	grid->capacity = new_capacity;
	return true;
}

static bool push_value(CsvGrid* grid, int value) {
	if (!reserve_values(grid, grid->count + 1)) {
		return false;
	}

	grid->values[grid->count++] = value;
	return true;
}

static bool parse_csv_file(const char* path, CsvGrid* grid) {
	FILE* file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "Failed to open input %s: %s\n", path, strerror(errno));
		return false;
	}

	char* line = NULL;
	size_t line_capacity = 0;
	ssize_t line_length = 0;
	size_t line_number = 0;

	while ((line_length = getline(&line, &line_capacity, file)) >= 0) {
		line_number++;
		if (line_length == 0) {
			continue;
		}

		char* cursor = trim(line);
		if (*cursor == '\0') {
			continue;
		}

		size_t row_width = 0;
		while (*cursor != '\0') {
			char* token_start = cursor;
			char* delimiter = strchr(cursor, ',');
			if (delimiter != NULL) {
				*delimiter = '\0';
				cursor = delimiter + 1;
			} else {
				cursor += strlen(cursor);
			}

			char* token = trim(token_start);
			if (*token == '\0') {
				fprintf(stderr, "Empty value at line %zu\n", line_number);
				free(line);
				fclose(file);
				return false;
			}

			errno = 0;
			char* parse_end = NULL;
			long parsed = strtol(token, &parse_end, 10);
			if (errno != 0 || parse_end == token || *trim(parse_end) != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
				fprintf(stderr, "Invalid integer '%s' at line %zu\n", token, line_number);
				free(line);
				fclose(file);
				return false;
			}

			if (!push_value(grid, (int)parsed)) {
				fprintf(stderr, "Out of memory while parsing %s\n", path);
				free(line);
				fclose(file);
				return false;
			}

			row_width++;
		}

		if (row_width == 0) {
			continue;
		}

		if (grid->width == 0) {
			grid->width = row_width;
		} else if (grid->width != row_width) {
			fprintf(stderr,
					"Inconsistent row width at line %zu: expected %zu columns, got %zu\n",
					line_number,
					grid->width,
					row_width);
			free(line);
			fclose(file);
			return false;
		}

		grid->height++;
	}

	free(line);
	fclose(file);

	if (grid->width == 0 || grid->height == 0 || grid->count != grid->width * grid->height) {
		fprintf(stderr, "%s does not contain a valid CSV grid\n", path);
		return false;
	}

	return true;
}

static bool write_header(FILE* file, size_t width, size_t height, int bitwidth) {
	uint8_t header[HEADER_SIZE];

	if (width > UINT16_MAX || height > UINT16_MAX) {
		fprintf(stderr, "Grid too large for TC header\n");
		return false;
	}

	header[0] = MAGIC_0;
	header[1] = MAGIC_1;
	header[2] = (uint8_t)(width & 0xFFu);
	header[3] = (uint8_t)((width >> 8u) & 0xFFu);
	header[4] = (uint8_t)(height & 0xFFu);
	header[5] = (uint8_t)((height >> 8u) & 0xFFu);
	header[6] = (uint8_t)bitwidth;

	return fwrite(header, 1, sizeof(header), file) == sizeof(header);
}

static bool value_fits_bitwidth(int value, int bitwidth) {
	if (value == -1) {
		return true;
	}

	if (value < 0) {
		return false;
	}

	if (bitwidth == 32) {
		return true;
	}

	return (uint32_t)value <= ((1u << bitwidth) - 2u);
}

static bool write_packed_payload(FILE* file, const CsvGrid* grid, int bitwidth) {
	size_t total = grid->width * grid->height;
	uint8_t current_byte = 0;
	int bits_filled = 0;

	for (size_t index = 0; index < total; index++) {
		int value = grid->values[index];
		uint32_t encoded = 0;

		if (!value_fits_bitwidth(value, bitwidth)) {
			if (bitwidth == 32) {
				fprintf(stderr,
						"32-bit mode supports -1 or non-negative values, got %d at tile %zu\n",
						value,
						index);
			} else {
				fprintf(stderr,
						"%d-bit mode supports -1 or 0..%u, got %d at tile %zu\n",
						bitwidth,
						(1u << bitwidth) - 2u,
						value,
						index);
			}
			return false;
		}

		if (value == -1) {
			encoded = bitwidth == 32 ? UINT32_MAX : ((1u << bitwidth) - 1u);
		} else {
			encoded = (uint32_t)value;
		}

		for (int bit_index = bitwidth - 1; bit_index >= 0; bit_index--) {
			uint8_t bit = (uint8_t)((encoded >> bit_index) & 1u);
			current_byte = (uint8_t)((current_byte << 1) | bit);
			bits_filled++;

			if (bits_filled == 8) {
				if (fwrite(&current_byte, 1, 1, file) != 1) {
					return false;
				}
				current_byte = 0;
				bits_filled = 0;
			}
		}
	}

	if (bits_filled > 0) {
		current_byte = (uint8_t)(current_byte << (8 - bits_filled));
		if (fwrite(&current_byte, 1, 1, file) != 1) {
			return false;
		}
	}

	return true;
}

static bool write_binary_file(const char* path, const CsvGrid* grid, int bitwidth) {
	FILE* file = fopen(path, "wb");
	if (file == NULL) {
		fprintf(stderr, "Failed to open output %s: %s\n", path, strerror(errno));
		return false;
	}

	if (!write_header(file, grid->width, grid->height, bitwidth)) {
		fprintf(stderr, "Failed to write header to %s\n", path);
		fclose(file);
		return false;
	}

	if (!write_packed_payload(file, grid, bitwidth)) {
		fprintf(stderr, "Failed to write payload to %s\n", path);
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

static long get_file_size(const char* path) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return -1;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return -1;
	}

	long size = ftell(file);
	fclose(file);
	return size;
}

static void print_usage(const char* program_name) {
	fprintf(stderr, "Usage: %s <input.csv> <output.bin> <bitwidth>\n", program_name);
	fprintf(stderr, "Bitwidth modes: 1 through 32\n");
}

int main(int argc, char* argv[]) {
	if (argc != 4) {
		print_usage(argv[0]);
		return 1;
	}

	errno = 0;
	char* parse_end = NULL;
	long parsed_bitwidth = strtol(argv[3], &parse_end, 10);
	if (errno != 0 || parse_end == argv[3] || *trim(parse_end) != '\0' || parsed_bitwidth < 1 || parsed_bitwidth > 32) {
		print_usage(argv[0]);
		return 1;
	}

	CsvGrid grid = {0};
	if (!parse_csv_file(argv[1], &grid)) {
		free_grid(&grid);
		return 1;
	}

	if (!write_binary_file(argv[2], &grid, (int)parsed_bitwidth)) {
		free_grid(&grid);
		return 1;
	}

	long input_size = get_file_size(argv[1]);
	long output_size = get_file_size(argv[2]);
	printf("Converted %s -> %s as %ld-bit (%zux%zu)", argv[1], argv[2], parsed_bitwidth, grid.width, grid.height);
	if (input_size >= 0 && output_size >= 0) {
		printf("  %ldB -> %ldB", input_size, output_size);
	}
	printf("\n");

	free_grid(&grid);
	return 0;
}