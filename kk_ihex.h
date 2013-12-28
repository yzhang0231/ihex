/*
 * kk_ihex.h: A simple library for reading and writing the Intel HEX 
 * or IHEX format. Intended mainly for embedded systems, and thus
 * somewhat optimised for size at the expense of error handling and
 * generality.
 *
 *      USAGE
 *      -----
 *
 * The library has been split into read and write parts, which use a
 * common data structure (`struct ihex_state`), but each can be used
 * independently. Include the header `kk_ihex_read.h` for reading, and/or
 * the header `kk_ihex_write.h` for writing (and link with their respective
 * object files). Both can be used simultaneously - this header defines
 * the shared data structures and definitions.
 *
 *
 *      READING INTEL HEX DATA
 *      ----------------------
 *
 * To read data in the Intel HEX format, you must perform the actual reading
 * of bytes using other means (e.g., stdio). The bytes read must then be
 * passed to `ihex_read_byte` and/or `ihex_read_bytes`. The reading functions
 * will then call `ihex_data_read`, at which stage the `struct ihex_state`
 * structure will contain the data along with its address. See below for
 * details and example implementation of `ihex_data_read`.
 *
 * The sequence to read data in IHEX format is:
 *      struct ihex_state ihex;
 *      ihex_begin_read(&ihex);
 *      ihex_read_bytes(&ihex, my_input_bytes, length_of_my_input_bytes);
 *      ihex_end_read(&ihex);
 *
 *
 *      WRITING BINARY DATA AS INTEL HEX
 *      --------------------------------
 *
 * In order to write out data, the `ihex_write_at_address` or
 * `ihex_write_at_segment` functions are used to set the data location,
 * and then the binary bytes are written with `ihex_write_byte` and/or
 * `ihex_write_bytes`. The writing functions will then call the function
 * `ihex_flush_buffer` whenever the internal write buffer needs to be
 * cleared - it is up to the caller to provide an implementation of
 * `ihex_flush_buffer` to do the actual writing. See below for details
 * and an example implementation.
 *
 * See the declaration further down for an example implementation.
 *
 * The sequence to write data in IHEX format is:
 *      struct ihex_state ihex;
 *      ihex_init(&ihex);
 *      ihex_write_at_address(&ihex, 0);
 *      ihex_write_bytes(&ihex, my_data, length_of_my_data);
 *      ihex_end_write(&ihex);
 *
 * For outputs larger than 64KiB, 32-bit linear addresses are output. Normally
 * the initial linear extended address record of zero is NOT written - it can
 * be forced by setting `ihex->flags |= IHEX_FLAG_ADDRESS_OVERFLOW` before
 * writing the first byte.
 *
 * Gaps in the data may be created by calling `ihex_write_at_address` with the
 * new starting address without calling `ihex_end_write` in between.
 *
 *
 * The same `struct ihex_state` may be used either for reading or writing,
 * but NOT both at the same time. Furthermore, a global output buffer is
 * used for writing, i.e., multiple threads must not write simultaneously
 * (but multiple writes may be interleaved).
 *
 *
 *      CONSERVING MEMORY
 *      -----------------
 *
 * For memory-critical use, you can save additional memory by defining
 * `IHEX_LINE_MAX_LENGTH` as something less than 255. Note, however, that
 * this limit affects both reading and writing, so the resulting library
 * will be unable to read lines with more than this number of data bytes.
 * That said, I haven't encountered any IHEX files with more than 32
 * data bytes per line. For write only there is no reason to define the
 * maximum as greater than the line length you'll actually be writing,
 * e.g., 32 or 16.
 *
 * If you are doing both reading and writing, you can define the maximum
 * output length separately as `IHEX_MAX_OUTPUT_LINE_LENGTH` - this will
 * decrease the write buffer size, but `struct ihex_state` will still
 * use the larger `IHEX_LINE_MAX_LENGTH` for its data storage.
 *
 * You can also save a few additional bytes by disabling support for
 * segmented addresses, by defining `IHEX_DISABLE_SEGMENTS`. Both the
 * read and write modules need to be build with the same option, as the
 * resulting data structures will not be compatible otherwise. To be honest,
 * this is a fairly pointless optimisation.
 *
 *
 * Copyright (c) 2013 Kimmo Kulovesi, http://arkku.com/
 * Provided with absolutely no warranty, use at your own risk only.
 * Use and distribute freely, mark modified copies as such.
 */

#ifndef KK_IHEX_H
#define KK_IHEX_H

#include <stdint.h>
#include <stdbool.h>

typedef uint_least32_t ihex_address_t;
typedef uint_least16_t ihex_segment_t;

// Maximum number of data bytes per line (applies to both reading and
// writing!); specify 255 to support reading all possible lengths. Less
// can be used to limit memory footprint on embedded systems, e.g.,
// most programs with IHEX output use 32.
#ifndef IHEX_LINE_MAX_LENGTH
#define IHEX_LINE_MAX_LENGTH 255
#endif

typedef struct ihex_state {
    ihex_address_t address;
#ifndef IHEX_DISABLE_SEGMENTS
    ihex_segment_t segment;
#endif
    uint8_t flags;
    uint8_t line_length;
    uint8_t length;
    uint8_t data[IHEX_LINE_MAX_LENGTH + 1];
} kk_ihex_t;

#define IHEX_FLAG_ADDRESS_OVERFLOW  0x80    // 16-bit address overflow

enum ihex_record_type {
    IHEX_DATA_RECORD,
    IHEX_END_OF_FILE_RECORD,
    IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD,
    IHEX_START_SEGMENT_ADDRESS_RECORD,
    IHEX_EXTENDED_LINEAR_ADDRESS_RECORD,
    IHEX_START_LINEAR_ADDRESS_RECORD
};

// Resolve segmented address (if any)
#ifndef IHEX_DISABLE_SEGMENTS
#define IHEX_LINEAR_ADDRESS(ihex) ((ihex)->address + (((ihex_address_t)((ihex)->segment)) << 4))
#else
#define IHEX_LINEAR_ADDRESS(ihex) ((ihex)->address)
#endif

// See kk_ihex_read.h and kk_ihex_write.h for function declarations!

#endif // !KK_IHEX_H
