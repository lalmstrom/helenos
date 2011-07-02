/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

/* Has to be first. */
#include "stdbool.h"

#include "internal/common.h"
#include "stdio.h"

#include "assert.h"
#include "errno.h"
#include "string.h"
#include "sys/types.h"

#include "libc/io/printf_core.h"
#include "libc/str.h"


/* not the best of solutions, but freopen and ungetc will eventually
 * need to be implemented in libc anyway
 */
#include "../c/generic/private/stdio.h"

/** Clears the stream's error and end-of-file indicators.
 *
 * @param stream Stream whose indicators shall be cleared.
 */
void posix_clearerr(FILE *stream)
{
	stream->error = 0;
	stream->eof = 0;
}

/**
 * Generate a pathname for the controlling terminal.
 *
 * @param s Allocated buffer to which the pathname shall be put.
 * @return Either s or static location filled with the requested pathname.
 */
char *posix_ctermid(char *s)
{
	/* Currently always returns an error value (empty string). */
	// TODO: return a real terminal path

	static char dummy_path[L_ctermid] = {'\0'};

	if (s == NULL) {
		return dummy_path;
	}

	s[0] = '\0';
	return s;
}

/**
 * Push byte back into input stream.
 * 
 * @param c Byte to be pushed back.
 * @param stream Stream to where the byte shall be pushed.
 * @return Provided byte on succes or EOF if not possible.
 */
int posix_ungetc(int c, FILE *stream)
{
	uint8_t b = (uint8_t) c;

	bool can_unget =
		/* Provided character is legal. */
	    c != EOF &&
		/* Stream is consistent. */
	    !stream->error &&
		/* Stream is buffered. */
	    stream->btype != _IONBF &&
		/* Last operation on the stream was a read operation. */
	    stream->buf_state == _bs_read &&
		/* Stream buffer is already allocated (i.e. there was already carried
		 * out either write or read operation on the stream). This is probably
		 * redundant check but let's be safe. */
	    stream->buf != NULL &&
		/* There is still space in the stream to retreat. POSIX demands the
		 * possibility to unget at least 1 character. It should be always
		 * possible, assuming the last operation on the stream read at least 1
		 * character, because the buffer is refilled in the lazily manner. */
	    stream->buf_tail > stream->buf;

	if (can_unget) {
		--stream->buf_tail;
		stream->buf_tail[0] = b;
		stream->eof = false;
		return (int) b;
	} else {
		return EOF;
	}
}

/**
 *
 * @param lineptr
 * @param n
 * @param delimiter
 * @param stream
 * @return
 */
ssize_t posix_getdelim(char **restrict lineptr, size_t *restrict n,
    int delimiter, FILE *restrict stream)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param lineptr
 * @param n
 * @param stream
 * @return
 */
ssize_t posix_getline(char **restrict lineptr, size_t *restrict n,
    FILE *restrict stream)
{
	// TODO
	not_implemented();
}

/**
 * Reopen a file stream.
 * 
 * @param filename Pathname of a file to be reopened or NULL for changing
 *     the mode of the stream.
 * @param mode Mode to be used for reopening the file or changing current
 *     mode of the stream.
 * @param stream Current stream associated with the opened file.
 * @return On success, either a stream of the reopened file or the provided
 *     stream with a changed mode. NULL otherwise.
 */
FILE *posix_freopen(
    const char *restrict filename,
    const char *restrict mode,
    FILE *restrict stream)
{
	assert(mode != NULL);
	assert(stream != NULL);

	if (filename == NULL) {
		// TODO
		
		/* print error to stderr as well, to avoid hard to find problems
		 * with buggy apps that expect this to work
		 */
		fprintf(stderr,
		    "ERROR: Application wants to use freopen() to change mode of opened stream.\n"
		    "       libposix does not support that yet, the application may function improperly.\n");
		errno = ENOTSUP;
		return NULL;
	}

	FILE* copy = malloc(sizeof(FILE));
	if (copy == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memcpy(copy, stream, sizeof(FILE));
	fclose(copy); /* copy is now freed */
	
	copy = fopen(filename, mode); /* open new stream */
	if (copy == NULL) {
		/* fopen() sets errno */
		return NULL;
	}
	
	/* move the new stream to the original location */
	memcpy(stream, copy, sizeof (FILE));
	free(copy);
	
	/* update references in the file list */
	stream->link.next->prev = &stream->link;
	stream->link.prev->next = &stream->link;
	
	return stream;
}

/**
 *
 * @param buf
 * @param size
 * @param mode
 * @return
 */
FILE *posix_fmemopen(void *restrict buf, size_t size,
    const char *restrict mode)
{
	// TODO
	not_implemented();
}

/**
 *
 * @param bufp
 * @param sizep
 * @return
 */
FILE *posix_open_memstream(char **bufp, size_t *sizep)
{
	// TODO
	not_implemented();
}

/**
 * Write error messages to standard error.
 *
 * @param s Error message.
 */
void posix_perror(const char *s)
{
	if (s == NULL || s[0] == '\0') {
		fprintf(stderr, "%s\n", posix_strerror(errno));
	} else {
		fprintf(stderr, "%s: %s\n", s, posix_strerror(errno));
	}
}

struct _posix_fpos {
	off64_t offset;
};

/** Restores stream a to position previously saved with fgetpos().
 *
 * @param stream Stream to restore
 * @param pos Position to restore
 * @return Zero on success, non-zero (with errno set) on failure
 */
int posix_fsetpos(FILE *stream, const posix_fpos_t *pos)
{
	return fseek(stream, pos->offset, SEEK_SET);
}

/** Saves the stream's position for later use by fsetpos().
 *
 * @param stream Stream to save
 * @param pos Place to store the position
 * @return Zero on success, non-zero (with errno set) on failure
 */
int posix_fgetpos(FILE *restrict stream, posix_fpos_t *restrict pos)
{
	off64_t ret = ftell(stream);
	if (ret == -1) {
		return errno;
	}
	pos->offset = ret;
	return 0;
}

/**
 * Reposition a file-position indicator in a stream.
 * 
 * @param stream Stream to seek in.
 * @param offset Direction and amount of bytes to seek.
 * @param whence From where to seek.
 * @return Zero on success, -1 otherwise.
 */
int posix_fseek(FILE *stream, long offset, int whence)
{
	return fseek(stream, (off64_t) offset, whence);
}

/**
 * Reposition a file-position indicator in a stream.
 * 
 * @param stream Stream to seek in.
 * @param offset Direction and amount of bytes to seek.
 * @param whence From where to seek.
 * @return Zero on success, -1 otherwise.
 */
int posix_fseeko(FILE *stream, posix_off_t offset, int whence)
{
	return fseek(stream, (off64_t) offset, whence);
}

/**
 * Discover current file offset in a stream.
 * 
 * @param stream Stream for which the offset shall be retrieved.
 * @return Current offset or -1 if not possible.
 */
long posix_ftell(FILE *stream)
{
	return (long) ftell(stream);
}

/**
 * Discover current file offset in a stream.
 * 
 * @param stream Stream for which the offset shall be retrieved.
 * @return Current offset or -1 if not possible.
 */
posix_off_t posix_ftello(FILE *stream)
{
	return (posix_off_t) ftell(stream);
}

/**
 * Print formatted output to the opened file.
 *
 * @param fildes File descriptor of the opened file.
 * @param format Format description.
 * @return Either the number of printed characters or negative value on error.
 */
int posix_dprintf(int fildes, const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vdprintf(fildes, format, list);
	va_end(list);
	return result;
}

/**
 * Write ordinary string to the opened file.
 *
 * @param str String to be written.
 * @param size Size of the string (in bytes)..
 * @param fd File descriptor of the opened file.
 * @return The number of written characters.
 */
static int _dprintf_str_write(const char *str, size_t size, void *fd)
{
	ssize_t wr = write(*(int *) fd, str, size);
	return str_nlength(str, wr);
}

/**
 * Write wide string to the opened file.
 * 
 * @param str String to be written.
 * @param size Size of the string (in bytes).
 * @param fd File descriptor of the opened file.
 * @return The number of written characters.
 */
static int _dprintf_wstr_write(const wchar_t *str, size_t size, void *fd)
{
	size_t offset = 0;
	size_t chars = 0;
	size_t sz;
	char buf[4];
	
	while (offset < size) {
		sz = 0;
		if (chr_encode(str[chars], buf, &sz, sizeof(buf)) != EOK) {
			break;
		}
		
		if (write(*(int *) fd, buf, sz) != (ssize_t) sz) {
			break;
		}
		
		chars++;
		offset += sizeof(wchar_t);
	}
	
	return chars;
}

/**
 * Print formatted output to the opened file.
 * 
 * @param fildes File descriptor of the opened file.
 * @param format Format description.
 * @param ap Print arguments.
 * @return Either the number of printed characters or negative value on error.
 */
int posix_vdprintf(int fildes, const char *restrict format, va_list ap)
{
	printf_spec_t spec = {
		.str_write = _dprintf_str_write,
		.wstr_write = _dprintf_wstr_write,
		.data = &fildes
	};
	
	return printf_core(format, &spec, ap);
}

/**
 * Print formatted output to the string.
 * 
 * @param s Output string.
 * @param format Format description.
 * @return Either the number of printed characters (excluding null byte) or
 *     negative value on error.
 */
int posix_sprintf(char *s, const char *format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vsprintf(s, format, list);
	va_end(list);
	return result;
}

/**
 * Print formatted output to the string.
 * 
 * @param s Output string.
 * @param format Format description.
 * @param ap Print arguments.
 * @return Either the number of printed characters (excluding null byte) or
 *     negative value on error.
 */
int posix_vsprintf(char *s, const char *format, va_list ap)
{
	return vsnprintf(s, STR_NO_LIMIT, format, ap);
}

/**
 * Convert formatted input from the stream.
 * 
 * @param stream Input stream.
 * @param format Format description.
 * @return The number of converted output items or EOF on failure.
 */
int posix_fscanf(FILE *restrict stream, const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vfscanf(stream, format, list);
	va_end(list);
	return result;
}

/**
 * Convert formatted input from the stream.
 * 
 * @param stream Input stream.
 * @param format Format description.
 * @param arg Output items.
 * @return The number of converted output items or EOF on failure.
 */
int posix_vfscanf(FILE *restrict stream, const char *restrict format, va_list arg)
{
	// TODO
	not_implemented();
}

/**
 * Convert formatted input from the standard input.
 * 
 * @param format Format description.
 * @return The number of converted output items or EOF on failure.
 */
int posix_scanf(const char *restrict format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vscanf(format, list);
	va_end(list);
	return result;
}

/**
 * Convert formatted input from the standard input.
 * 
 * @param format Format description.
 * @param arg Output items.
 * @return The number of converted output items or EOF on failure.
 */
int posix_vscanf(const char *restrict format, va_list arg)
{
	return posix_vfscanf(stdin, format, arg);
}

/**
 * Convert formatted input from the string.
 * 
 * @param s Input string.
 * @param format Format description.
 * @return The number of converted output items or EOF on failure.
 */
int posix_sscanf(const char *s, const char *format, ...)
{
	va_list list;
	va_start(list, format);
	int result = posix_vsscanf(s, format, list);
	va_end(list);
	return result;
}

/**
 * Convert formatted input from the string.
 * 
 * @param s Input string.
 * @param format Format description.
 * @param arg Output items.
 * @return The number of converted output items or EOF on failure.
 */
int posix_vsscanf(
    const char *restrict s, const char *restrict format, va_list arg)
{
	// TODO
	not_implemented();
}

/**
 * Acquire file stream for the thread.
 *
 * @param file File stream to lock.
 */
void posix_flockfile(FILE *file)
{
	/* dummy */
}

/**
 * Acquire file stream for the thread (non-blocking).
 *
 * @param file File stream to lock.
 * @return Zero for success and non-zero if the lock cannot be acquired.
 */
int posix_ftrylockfile(FILE *file)
{
	/* dummy */
	return 0;
}

/**
 * Relinquish the ownership of the locked file stream.
 *
 * @param file File stream to unlock.
 */
void posix_funlockfile(FILE *file)
{
	/* dummy */
}

/**
 * Get a byte from a stream (thread-unsafe).
 *
 * @param stream Input file stream.
 * @return Either read byte or EOF.
 */
int posix_getc_unlocked(FILE *stream)
{
	return getc(stream);
}

/**
 * Get a byte from the standard input stream (thread-unsafe).
 *
 * @return Either read byte or EOF.
 */
int posix_getchar_unlocked(void)
{
	return getchar();
}

/**
 * Put a byte on a stream (thread-unsafe).
 *
 * @param c Byte to output.
 * @param stream Output file stream.
 * @return Either written byte or EOF.
 */
int posix_putc_unlocked(int c, FILE *stream)
{
	return putc(c, stream);
}

/**
 * Put a byte on the standard output stream (thread-unsafe).
 * 
 * @param c Byte to output.
 * @return Either written byte or EOF.
 */
int posix_putchar_unlocked(int c)
{
	return putchar(c);
}

/**
 * Remove a file.
 *
 * @param path Pathname of the file that shall be removed.
 * @return Zero on success, -1 otherwise.
 */
int posix_remove(const char *path)
{
	// FIXME: unlink() and rmdir() seem to be equivalent at the moment,
	//        but that does not have to be true forever
	return unlink(path);
}

/**
 * 
 * @param s
 * @return
 */
char *posix_tmpnam(char *s)
{
	// TODO: low priority, just a compile-time dependency of binutils
	not_implemented();
}

/** @}
 */
