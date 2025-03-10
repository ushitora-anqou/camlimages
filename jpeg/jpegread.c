/***********************************************************************/
/*                                                                     */
/*                           Objective Caml                            */
/*                                                                     */
/*            Fran輟is Pessaux, projet Cristal, INRIA Rocquencourt     */
/*            Pierre Weis, projet Cristal, INRIA Rocquencourt          */
/*            Jun Furuse, projet Cristal, INRIA Rocquencourt           */
/*                                                                     */
/*  Copyright 1999,2000                                                */
/*  Institut National de Recherche en Informatique et en Automatique.  */
/*  Distributed only by permission.                                    */
/*                                                                     */
/***********************************************************************/

/* $Id: jpegread.c,v 1.5 2009/10/16 16:08:33 weis Exp $ */

#include "../config/config.h"
#include "compat.h"

#ifdef HAS_JPEG

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>

#include "../include/oversized.h"

#include <stdio.h>
#include <string.h>

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */

#include "jpeglib.h"

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>

#ifdef DEBUG_JPEG
#  define DEBUGF(...) do{fprintf(stderr, __VA_ARGS__); fflush(stderr); }while(0)
#else
#  define DEBUGF(...) do{/*do nothing*/}while(0)
#endif

char jpg_error_message[JMSG_LENGTH_MAX];
 
struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfop)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfop->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfop->err->format_message) (cinfop, jpg_error_message );

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

CAMLprim value jpeg_set_scale_denom( jpegh, denom )
     value jpegh;
     value denom;
{
    CAMLparam2(jpegh, denom);

    struct jpeg_decompress_struct *cinfop;
    cinfop = (struct jpeg_decompress_struct *) Ptr_val(Field ( jpegh, 0 ));
    cinfop->scale_num = 1;
    cinfop->scale_denom = Int_val( denom );
    CAMLreturn(Val_unit);
}

static value caml_val_jpeg_marker( jpeg_saved_marker_ptr p )
{
    CAMLparam0();
    CAMLlocal2(res, data);

    if(!p) { caml_failwith("caml_val_jpeg_marker: bug"); }

    if( p->data ) {
        data = caml_alloc_string(p->data_length);
        memcpy( Bytes_val(data), p->data, p->data_length);
    } else {
        data = caml_alloc_string(0);
    }

    res = caml_alloc_tuple(2);
    Store_field(res, 0, Val_int(p->marker));
    Store_field(res, 1, data);

    CAMLreturn(res);
}

static value caml_val_jpeg_rev_markers( jpeg_saved_marker_ptr p )
{
    CAMLparam0();
    CAMLlocal3(res, tmp, hd);

    res = Val_int(0); // null

    while(p){
        hd = caml_val_jpeg_marker(p);

        tmp = caml_alloc(2, 0); 
        Store_field(tmp, 0, hd);
        Store_field(tmp, 1, res);

        res = tmp;
        p = p->next;
    }

    CAMLreturn(res);
}

CAMLprim value open_jpeg_file_for_read( name )
     value name;
{
    CAMLparam1(name);
    CAMLlocal3(res, hdl, markers);

    const char *filename;
    /* This struct contains the JPEG decompression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     */
    struct jpeg_decompress_struct* cinfop;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct my_error_mgr *myerrp;
    /* More stuff */
    FILE * infile;		/* source file */
    int i;

    filename= String_val( name );

    if ((infile = fopen(filename, "rb")) == NULL) {
        failwith("failed to open jpeg file");
    }

    cinfop = malloc(sizeof (struct jpeg_decompress_struct));
    myerrp = malloc(sizeof (struct my_error_mgr));

    /* In this example we want to open the input file before doing anything else,
     * so that the setjmp() error recovery below can assume the file is open.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to read binary files.
     */


    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfop->err = jpeg_std_error(&myerrp->pub);
    myerrp->pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(myerrp->setjmp_buffer)) {
        /* If we get here, the JPEG code has signaled an error.
         * We need to clean up the JPEG object, close the input file, and return.
         */
        jpeg_destroy_decompress(cinfop);
        free(cinfop);
        free(myerrp);
        fclose(infile);
        failwith(jpg_error_message);
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(cinfop);

    /* Step 2: specify data source (eg, a file) */

    jpeg_stdio_src(cinfop, infile);

    /* EXIF and other markers */

    jpeg_save_markers(cinfop, JPEG_COM, 0xFFFF /* all */);
    for(i=0; i<16; i++)
        jpeg_save_markers(cinfop, JPEG_APP0+i, 0xFFFF /* all */);

    /* Step 3: read file parameters with jpeg_read_header() */

    (void) jpeg_read_header(cinfop, TRUE);

    /* (width, height, (cinfop, infile, myerrp), markers) */

    // https://v2.ocaml.org/manual/intfc.html#ss:c-outside-head
    // In earlier versions of OCaml, it was possible to use word-aligned pointers to addresses
    // outside the heap as OCaml values, just by casting the pointer to type value. This usage
    // is no longer supported since OCaml 5.0.
    
    hdl = caml_alloc_tuple(3);
    Store_field(hdl, 0, Val_ptr(cinfop));
    Store_field(hdl, 1, Val_ptr(infile));
    Store_field(hdl, 2, Val_ptr(myerrp));

    // must comes earlier than caml_alloc(3, 0).... I do not know why
    markers = caml_val_jpeg_rev_markers( cinfop->marker_list );

    res = caml_alloc_tuple(4);
    Store_field (res, 0, Val_int(cinfop->image_width));
    Store_field (res, 1, Val_int(cinfop->image_height));
    Store_field (res, 2, hdl);
    Store_field (res, 3, markers);

    CAMLreturn(res);
}

CAMLprim value open_jpeg_file_for_read_start( jpegh )
     value jpegh;
{
  CAMLparam1(jpegh);
  CAMLlocal1(res);

  struct jpeg_decompress_struct* cinfop;
  struct my_error_mgr *myerrp;
  FILE *infile;
  int i;

  cinfop = (struct jpeg_decompress_struct *) Ptr_val(Field( jpegh, 0 ));
  infile = (FILE *) Ptr_val(Field( jpegh, 1 ));
  myerrp = (struct my_error_mgr *) Ptr_val(Field( jpegh, 2 ));

  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  cinfop->out_color_space = JCS_RGB;

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(cinfop);
  DEBUGF("jpeg_start_decompress");

  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */

  /* row_stride = cinfop->output_width * cinfop->output_components; */

  res = caml_alloc_tuple(2);
  Store_field(res, 0, Val_int(cinfop->output_width));
  Store_field(res, 1, Val_int(cinfop->output_height));

  DEBUGF("cinfop= %d infile= %d %d %d \n", (int)cinfop, (int)infile, cinfop->output_scanline, cinfop->output_height); 
  CAMLreturn(res);
}

CAMLprim value read_jpeg_scanline( jpegh, buf, offset )
value jpegh, offset, buf;
{
  CAMLparam3(jpegh,offset,buf);
  struct jpeg_decompress_struct *cinfop;
  JSAMPROW row[1];

  cinfop = (struct jpeg_decompress_struct *) Ptr_val(Field( jpegh, 0 ));
  row[0] = Bytes_val( buf ) + Int_val( offset );
  jpeg_read_scanlines( cinfop, row, 1 );

  CAMLreturn(Val_unit);
}

/* no boundary checks */
CAMLprim void read_jpeg_scanlines( value jpegh, value buf, value offset, value lines )
{
  CAMLparam4(jpegh,offset,buf,lines);
  struct jpeg_decompress_struct *cinfop;
  JSAMPROW row[1];
  int clines = Int_val(lines);
  int i;
  row[0] = Bytes_val(buf) + Int_val(offset);
  cinfop = (struct jpeg_decompress_struct *) Ptr_val(Field( jpegh, 0 ));
  // width is NOT image_width since we may have scale_denom <> 1
  int scanline_bytes = cinfop->output_width * 3; // no padding (size 3 is fixed? )
  for(i=0; i<clines; i++){
    jpeg_read_scanlines( cinfop, row, 1 );
    row[0] += scanline_bytes;
  }
  CAMLreturn0;
}

CAMLprim value close_jpeg_file_for_read( jpegh )
     value jpegh;
{
  CAMLparam1(jpegh);

  struct jpeg_decompress_struct *cinfop;
  struct my_error_mgr *myerrp;
  FILE *infile;

  DEBUGF("closing\n");

  cinfop = (struct jpeg_decompress_struct *) Ptr_val(Field( jpegh, 0 ));
  infile = (FILE *) Ptr_val(Field( jpegh, 1 ));
  myerrp = (struct my_error_mgr *) Ptr_val(Field( jpegh, 2 ));

  DEBUGF("cinfop= %d infile= %d %d %d \n", (int)cinfop, (int)infile, cinfop->output_scanline, cinfop->output_height); 

  if( cinfop->output_height > 0 ){ // == 0 means jpeg_start_decompress is never called
      if( cinfop->output_scanline >= cinfop->output_height ){ 
          DEBUGF("finish_decompress\n");
          jpeg_finish_decompress( cinfop );
      }
  }
  DEBUGF("destroy\n");
  jpeg_destroy_decompress( cinfop ); 
  
  free(cinfop);
  free(myerrp);
  DEBUGF("file close\n");
  fclose(infile);

  CAMLreturn(Val_unit);
}

#else // HAS_JPEG

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>

CAMLprim value jpeg_set_scale_denom(){ failwith("unsupported"); }
CAMLprim value open_jpeg_file_for_read(){ failwith("unsupported"); }
CAMLprim value open_jpeg_file_for_read_start(){ failwith("unsupported"); }
CAMLprim value read_jpeg_scanline(){ failwith("unsupported"); }
CAMLprim value read_jpeg_scanlines(){ failwith("unsupported"); }
CAMLprim value close_jpeg_file_for_read(){ failwith("unsupported"); }

#endif // HAS_JPEG
