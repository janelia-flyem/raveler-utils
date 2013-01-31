/*
 * BASED ON libpng-short-example.c
 *
 * Base work has following license:
 *   Copyright 2002-2008 Guillaume Cottenceau.
 *   This software may be freely redistributed under the terms
 *   of the X11 license.
 *
 * Please see Janelia license in LICENSE.txt for modifications.
 */

#include "PngImage.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

void abort_(const char * s, ...)
{
    va_list args;
    va_start(args, s);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    va_end(args);
    abort();
}


PngImage::PngImage(const char* filename) :
    m_rgba(false),
    m_colsize(0)
{
    png_byte header[8]; // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(filename, "rb");
    if (!fp)
        abort_("[read_png_file] File %s could not be opened for reading", filename);
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
        abort_("[read_png_file] File %s is not recognized as a PNG file", filename);


    /* initialize stuff */
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr)
        abort_("[read_png_file] png_create_read_struct failed");

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);


    m_width = png_get_image_width(png_ptr, info_ptr);
    m_height = png_get_image_height(png_ptr, info_ptr);
    m_color_type = png_get_color_type(png_ptr, info_ptr);
    m_bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    png_read_update_info(png_ptr, info_ptr);


    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[read_png_file] Error during read_image");

    m_row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * m_height);
    for (int y=0; y<m_height; y++)
        m_row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));

    png_read_image(png_ptr, m_row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    
    // We only support 8-bit RGBA or 16-bit Grayscale but we could
    // easily add support for more if needed, libpng supports 
    // them all.
    if (m_color_type == PNG_COLOR_TYPE_RGBA && m_bit_depth == 8)
    {
        m_rgba = true;
        m_colsize = 4;
    }
    else if (m_color_type == PNG_COLOR_TYPE_GRAY && m_bit_depth == 16)
    {
        m_rgba = false;
        m_colsize = 2;
    }
    else
    {
        abort_("UNSUPPORTED PNG FORMAT: %s\n", filename);
    }
}

PngImage::~PngImage()
{
    for (int y=0; y<m_height; y++)
        free(m_row_pointers[y]);
    free(m_row_pointers);
}

void PngImage::write(const char* filename)
{
    /* create file */
    FILE *fp = fopen(filename, "wb");
    if (!fp)
        abort_("[write_png_file] File %s could not be opened for writing", filename);


    /* initialize stuff */
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!png_ptr)
        abort_("[write_png_file] png_create_write_struct failed");

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
        abort_("[write_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during init_io");

    png_init_io(png_ptr, fp);


    /* write header */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during writing header");

    png_set_IHDR(png_ptr, info_ptr, m_width, m_height,
             m_bit_depth, m_color_type, PNG_INTERLACE_NONE,
             PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    png_write_info(png_ptr, info_ptr);


    /* write bytes */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during writing bytes");

    png_write_image(png_ptr, m_row_pointers);


    /* end write */
    if (setjmp(png_jmpbuf(png_ptr)))
        abort_("[write_png_file] Error during end of write");

    png_write_end(png_ptr, NULL);

    fclose(fp);
    png_destroy_write_struct(&png_ptr, &info_ptr);
}


