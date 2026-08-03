#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 7
#define MAGIC_0 'T'
#define MAGIC_1 'C'

typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t bitwidth;
	uint32_t* values;
} BinaryGrid;

static void free_grid(BinaryGrid* grid) {
	if (grid == NULL) {
		return;
	}

	free(grid->values);
	grid->values = NULL;
	grid->width = 0;
	grid->height = 0;
	grid->bitwidth = 0;
}

static uint16_t read_u16_le(const uint8_t* bytes) {
	return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8u);
}

static bool load_binary_file(const char* path, uint8_t** out_bytes, size_t* out_size) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Failed to open input %s: %s\n", path, strerror(errno));
		return false;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fprintf(stderr, "Failed to seek %s\n", path);
		fclose(file);
		return false;
	}

	long file_size = ftell(file);
	if (file_size < 0) {
		fprintf(stderr, "Failed to size %s\n", path);
		fclose(file);
		return false;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		fprintf(stderr, "Failed to rewind %s\n", path);
		fclose(file);
		return false;
	}

	uint8_t* bytes = malloc((size_t)file_size);
	if (bytes == NULL && file_size > 0) {
		fprintf(stderr, "Out of memory reading %s\n", path);
		fclose(file);
		return false;
	}

	if (file_size > 0 && fread(bytes, 1, (size_t)file_size, file) != (size_t)file_size) {
		fprintf(stderr, "Failed to read %s\n", path);
		free(bytes);
		fclose(file);
		return false;
	}

	fclose(file);
	*out_bytes = bytes;
	*out_size = (size_t)file_size;
	return true;
}

static bool parse_binary_grid(const uint8_t* bytes, size_t size, BinaryGrid* grid) {
	if (size < HEADER_SIZE) {
		fprintf(stderr, "Input is too small to be a TC binary\n");
		return false;
	}

	if (bytes[0] != MAGIC_0 || bytes[1] != MAGIC_1) {
		fprintf(stderr, "Input does not start with TC magic\n");
		return false;
	}

	grid->width = read_u16_le(&bytes[2]);
	grid->height = read_u16_le(&bytes[4]);
	grid->bitwidth = bytes[6];

	if (grid->width == 0 || grid->height == 0) {
		fprintf(stderr, "Binary grid dimensions must be non-zero\n");
		return false;
	}

	if (grid->bitwidth < 1 || grid->bitwidth > 32) {
		fprintf(stderr, "Binary bitwidth must be between 1 and 32\n");
		return false;
	}

	size_t value_count = (size_t)grid->width * (size_t)grid->height;
	if (value_count > SIZE_MAX / sizeof(*grid->values)) {
		fprintf(stderr, "Binary grid is too large\n");
		return false;
	}

	size_t payload_bits = value_count * (size_t)grid->bitwidth;
	size_t payload_bytes = (payload_bits + 7u) / 8u;
	if (size != HEADER_SIZE + payload_bytes) {
		fprintf(stderr,
				"Unexpected binary size: expected %zu bytes, got %zu\n",
				HEADER_SIZE + payload_bytes,
				size);
		return false;
	}

	grid->values = calloc(value_count, sizeof(*grid->values));
	if (grid->values == NULL) {
		fprintf(stderr, "Out of memory decoding binary grid\n");
		return false;
	}

	const uint8_t* payload = bytes + HEADER_SIZE;
	size_t bit_cursor = 0;
	uint32_t sentinel = grid->bitwidth == 32 ? UINT32_MAX : ((uint32_t)1u << grid->bitwidth) - 1u;

	for (size_t index = 0; index < value_count; index++) {
		uint32_t decoded = 0;
		for (uint8_t bit_index = 0; bit_index < grid->bitwidth; bit_index++) {
			size_t byte_index = bit_cursor / 8u;
			uint8_t bit_in_byte = (uint8_t)(7u - (bit_cursor % 8u));
			uint8_t bit = (payload[byte_index] >> bit_in_byte) & 1u;
			decoded = (decoded << 1u) | bit;
			bit_cursor++;
		}
		grid->values[index] = decoded == sentinel ? UINT32_MAX : decoded;
	}

	return true;
}

static bool write_csv_file(const char* path, const BinaryGrid* grid) {
	FILE* file = fopen(path, "w");
	if (file == NULL) {
		fprintf(stderr, "Failed to open output %s: %s\n", path, strerror(errno));
		return false;
	}

	for (uint32_t row = 0; row < grid->height; row++) {
		for (uint32_t col = 0; col < grid->width; col++) {
			size_t index = (size_t)row * (size_t)grid->width + (size_t)col;
			if (grid->values[index] == UINT32_MAX) {
				if (fputs("-1", file) == EOF) {
					fclose(file);
					return false;
				}
			} else if (fprintf(file, "%u", grid->values[index]) < 0) {
				fclose(file);
				return false;
			}

			if (col + 1 < grid->width && fputc(',', file) == EOF) {
				fclose(file);
				return false;
			}
		}

		if (fputc('\n', file) == EOF) {
			fclose(file);
			return false;
		}
	}

	fclose(file);
	return true;
}

static void print_usage(const char* program_name) {
	fprintf(stderr, "Usage: %s <input.bin> <output.csv>\n", program_name);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		print_usage(argv[0]);
		return 1;
	}

	uint8_t* bytes = NULL;
	size_t size = 0;
	if (!load_binary_file(argv[1], &bytes, &size)) {
		return 1;
	}

	BinaryGrid grid = {0};
	if (!parse_binary_grid(bytes, size, &grid)) {
		free(bytes);
		free_grid(&grid);
		return 1;
	}

	free(bytes);

	if (!write_csv_file(argv[2], &grid)) {
		free_grid(&grid);
		return 1;
	}

	printf("Converted %s -> %s (%ux%u, %u-bit)\n", argv[1], argv[2], grid.width, grid.height, grid.bitwidth);
	free_grid(&grid);
	return 0;
}