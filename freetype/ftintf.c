/***********************************************************************/
/*                                                                     */
/*                           Objective Caml                            */
/*                                                                     */
/*            Jun Furuse, projet Cristal, INRIA Rocquencourt           */
/*                                                                     */
/*  Copyright 1999,2000                                                */
/*  Institut National de Recherche en Informatique et en Automatique.  */
/*  Distributed only by permission.                                    */
/*                                                                     */
/***********************************************************************/

#include "../config/config.h"
#include "compat.h"

#ifdef HAS_FREETYPE

#include <string.h>

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/callback.h>
#include <caml/fail.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define FT_Library_val(x) ((FT_Library*)Ptr_val(x))

CAMLprim value init_FreeType()
{
    CAMLparam0();

    FT_Library *library;

    if( (library = stat_alloc( sizeof(FT_Library) )) == NULL ){
        failwith( "init_FreeType: Memory over" );
    }
    if( FT_Init_FreeType( library ) ){
        stat_free(library);
        failwith( "FT_Init_FreeType" );
    }

    CAMLreturn( Val_ptr(library) );
}

CAMLprim value done_FreeType( lib )
    value lib;
{
    CAMLparam1(lib);

    FT_Library* library = FT_Library_val(lib);

    if ( FT_Done_FreeType( *library ) ){
        failwith( "FT_Done_FreeType" );
    }

    stat_free( (void *) library );

    CAMLreturn(Val_unit);
}

#include <stdio.h>
CAMLprim value new_Face( lib, fontpath, idx )
     value lib;
     value fontpath;
     value idx;
{
    CAMLparam3(lib, fontpath, idx );

    FT_Library *library = FT_Library_val(lib);
    FT_Face *face;

    if( (face = stat_alloc( sizeof(FT_Face) )) == NULL ){
        failwith( "new_Face: Memory over" );
    }
    if( FT_New_Face( *library, String_val( fontpath ), Int_val( idx ), face ) ){
        stat_free(face);
        failwith( "new_Face: Could not open face" );
    }

    CAMLreturn( Val_ptr(face) );
}

CAMLprim value face_info( vface )
     value vface;
{
    CAMLparam1(vface);
    CAMLlocal1(res);

    FT_Face face = *(FT_Face*)Ptr_val(vface);
  
    res = alloc_tuple(14);
    Store_field(res, 0, Val_int( face->num_faces ));
    Store_field(res, 1, Val_int( face->num_glyphs ));
    Store_field(res, 2, copy_string( face->family_name == NULL ? "" : face->family_name ));
    Store_field(res, 3, copy_string( face->style_name == NULL ? "" : face->style_name ));
    Store_field(res, 4, Val_bool( FT_HAS_HORIZONTAL( face ) ));
    Store_field(res, 5, Val_bool( FT_HAS_VERTICAL( face ) ));
    Store_field(res, 6, Val_bool( FT_HAS_KERNING( face ) ));
    Store_field(res, 7, Val_bool( FT_IS_SCALABLE( face ) ));
    Store_field(res, 8, Val_bool( FT_IS_SFNT( face ) ));
    Store_field(res, 9, Val_bool( FT_IS_FIXED_WIDTH( face ) ));
    Store_field(res,10, Val_bool( FT_HAS_FIXED_SIZES( face ) ));
    Store_field(res,11, Val_bool( FT_HAS_FAST_GLYPHS( face ) ));
    Store_field(res,12, Val_bool( FT_HAS_GLYPH_NAMES( face ) ));
    Store_field(res,13, Val_bool( FT_HAS_MULTIPLE_MASTERS( face ) ));

    CAMLreturn(res);
}

CAMLprim value done_Face( vface )
     value vface;
{
    CAMLparam1(vface);

    FT_Face* face = Ptr_val(vface);

    if ( FT_Done_Face( *face ) ){
        failwith("FT_Done_Face");
    }

    CAMLreturn( Val_unit );
}

CAMLprim value get_num_glyphs( vface )
     value vface;
{
    CAMLparam1(vface);

    FT_Face* face = Ptr_val(vface);

    CAMLreturn( Val_int ((*face)->num_glyphs) );
}


CAMLprim value set_Char_Size( vface, char_w, char_h, res_h, res_v )
     value vface;
     value char_w, char_h; /* 26.6 1 = 1/64pt */
     value res_h, res_v; /* dpi */
{
    CAMLparam5( vface, char_w, char_h, res_h, res_v );

    FT_Face *face = Ptr_val(vface);

    if ( FT_Set_Char_Size( *face,
                           Int_val(char_w), Int_val(char_h),
                           Int_val(res_h), Int_val(res_v) ) ){
        failwith("FT_Set_Char_Size");
    }

    CAMLreturn(Val_unit);
}

/* to be done: query at face->fixed_sizes
 */

CAMLprim value set_Pixel_Sizes( vface, pixel_w, pixel_h )
    value vface;
value pixel_w, pixel_h; /* dot */
{
    CAMLparam3(vface,pixel_w,pixel_h);

    FT_Face *face = Ptr_val(vface);

    if ( FT_Set_Pixel_Sizes( *face, Int_val(pixel_w), Int_val(pixel_h) ) ){
        failwith("FT_Set_Pixel_Sizes");
    }

    CAMLreturn(Val_unit);
}

CAMLprim value val_CharMap( charmapp )
     FT_CharMap *charmapp;
{
    CAMLparam0();
    CAMLlocal1(res);

    res = alloc_tuple(2);
    Store_field(res,0, Val_int((*charmapp)->platform_id));
    Store_field(res,1, Val_int((*charmapp)->encoding_id));

    CAMLreturn(res);
}

CAMLprim value get_CharMaps( vface )
     value vface;
{
    CAMLparam1(vface);
    CAMLlocal3(list,last_cell,new_cell);

    int i = 0;
    FT_Face face;

    face = *(FT_Face *)(Ptr_val(vface));

    list = last_cell = Val_unit;

    while( i < face->num_charmaps ){
        new_cell = alloc_tuple(2);
        Store_field(new_cell,0, val_CharMap( face->charmaps + i ));
        Store_field(new_cell,1, Val_unit);
        if( i == 0 ){
            list = new_cell;
        } else {
            Store_field(last_cell,1, new_cell);
        }
        last_cell = new_cell;
        i++;
    }

    CAMLreturn(list);
}

CAMLprim value set_CharMap( vface, charmapv )
     value vface;
     value charmapv;
{
    CAMLparam2(vface,charmapv);

    int i = 0;
    FT_Face face;
    FT_CharMap charmap;
    int my_pid, my_eid;

    face = *(FT_Face *) (Ptr_val(vface));
    my_pid = Int_val(Field(charmapv, 0));
    my_eid = Int_val(Field(charmapv, 1));

    while( i < face->num_charmaps ){
        charmap = face->charmaps[i];
        if ( charmap->platform_id == my_pid &&
             charmap->encoding_id == my_eid ){
            if ( FT_Set_Charmap( face, charmap ) ){
                failwith("FT_Set_Charmap");
            }
            CAMLreturn(Val_unit);
        } else {
            i++;
        }
    }

    failwith("freetype:set_charmaps: selected pid+eid do not exist");
}

CAMLprim value get_Char_Index( vface, code )
     value vface, code;
{
    CAMLparam2(vface,code);

    FT_Face *face = Ptr_val(vface);

    CAMLreturn(Val_int(FT_Get_Char_Index( *face, Int_val(code))));
}

CAMLprim value load_Glyph( vface, index, flags )
     value vface, index, flags;
{
    CAMLparam3(vface,index,flags);
    CAMLlocal1(res);

    FT_Face *face = Ptr_val(vface);

    if( FT_Load_Glyph( *face, Int_val(index), FT_LOAD_DEFAULT | Int_val(flags)) ){
        failwith("FT_Load_Glyph");
    }

    res = alloc_tuple(2);
    Store_field(res,0, Val_int( (*face)->glyph->advance.x ));
    Store_field(res,1, Val_int( (*face)->glyph->advance.y ));

    CAMLreturn(res);
}

CAMLprim value load_Char( vface, code, flags )
     value vface, code, flags;
{
    CAMLparam3(vface,code,flags);
    CAMLlocal1(res);

    FT_Face* face = Ptr_val(vface);

    /* FT_Load_Glyph(face, FT_Get_Char_Index( face, code )) */
    if( FT_Load_Char( *face, Int_val(code), FT_LOAD_DEFAULT | Int_val(flags)) ){
        failwith("FT_Load_Char");
    }

    res = alloc_tuple(2);
    Store_field(res,0, Val_int( (*(FT_Face*)face)->glyph->advance.x ));
    Store_field(res,1, Val_int( (*(FT_Face*)face)->glyph->advance.y ));

    CAMLreturn(res);
}

CAMLprim value render_Glyph_of_Face( vface, mode )
     value vface;
     value mode;
{
    CAMLparam2(vface,mode);

    FT_Face *face = (FT_Face*)Ptr_val(vface);

    if (FT_Render_Glyph( (*face)->glyph , Int_val(mode) )){
        failwith("FT_Render_Glyph");
    }
    CAMLreturn(Val_unit);
}

CAMLprim value render_Char( vface, code, flags, mode )
     value vface, code, flags, mode;
{
    CAMLparam4(vface,code,flags,mode);
    CAMLlocal1(res);

    FT_Face *face = Ptr_val(vface);

    /* FT_Load_Glyph(face, FT_Get_Char_Index( face, code ), FT_LOAD_RENDER) */
    if( FT_Load_Char( *face, Int_val(code),
                      FT_LOAD_RENDER |
                      Int_val(flags) |
                      (Int_val(mode) ? FT_LOAD_MONOCHROME : 0)) ){
        failwith("FT_Load_Char");
    }

    res = alloc_tuple(2);
    Store_field(res,0, Val_int( (*(FT_Face*)face)->glyph->advance.x ));
    Store_field(res,1, Val_int( (*(FT_Face*)face)->glyph->advance.y ));

    CAMLreturn(res);
}

CAMLprim value set_Transform( vface, vmatrix, vpen )
     value vface, vmatrix, vpen;
{
    CAMLparam3(vface, vmatrix, vpen);

    FT_Matrix matrix;
    FT_Vector pen;
    FT_Face *face = Ptr_val(vface);

    matrix.xx = (FT_Fixed)( Int_val(Field(vmatrix,0)) );
    matrix.xy = (FT_Fixed)( Int_val(Field(vmatrix,1)) );
    matrix.yx = (FT_Fixed)( Int_val(Field(vmatrix,2)) );
    matrix.yy = (FT_Fixed)( Int_val(Field(vmatrix,3)) );
    pen.x = (FT_Fixed)( Int_val(Field(vpen,0)) );
    pen.y = (FT_Fixed)( Int_val(Field(vpen,1)) );

    FT_Set_Transform( *face, &matrix, &pen );

    CAMLreturn(Val_unit);
}

CAMLprim value get_Bitmap_Info( vface )
     value vface;
{
    CAMLparam1(vface);
    CAMLlocal1(res);

    FT_Face *face = Ptr_val(vface);

    FT_GlyphSlot glyph = (*face)->glyph;
    FT_Bitmap bitmap = glyph->bitmap;

    switch ( bitmap.pixel_mode ) {
    case ft_pixel_mode_grays:
        if ( bitmap.num_grays != 256 ){
            failwith("get_Bitmap_Info: unknown num_grays");
        }
        break;
    case ft_pixel_mode_mono:
        break;
    default:
        failwith("get_Bitmap_Info: unknown pixel mode");
    }

    res = alloc_tuple(5);
    Store_field(res,0, Val_int(glyph->bitmap_left));
    Store_field(res,1, Val_int(glyph->bitmap_top));
    Store_field(res,2, Val_int(bitmap.width));
    Store_field(res,3, Val_int(bitmap.rows));

    CAMLreturn(res);
}

CAMLprim value read_Bitmap( vface, vx, vy ) /* This "y" is in Y upwards convention */
     value vface, vx, vy;
{
    CAMLparam3(vface, vx, vy);

    /* no boundary check !!! */
    int x, y;

    FT_Face *face = Ptr_val(vface);
    FT_Bitmap bitmap = (*face)->glyph->bitmap;

    unsigned char *row;

    x = Int_val(vx);
    y = Int_val(vy);

    switch ( bitmap.pixel_mode ) {
    case ft_pixel_mode_grays:
        if (bitmap.pitch > 0){
            row = bitmap.buffer + (bitmap.rows - 1 - y) * bitmap.pitch;
        } else {
            row = bitmap.buffer - y * bitmap.pitch;
        }
        CAMLreturn (Val_int(row[x]));

    case ft_pixel_mode_mono:
        if (bitmap.pitch > 0){
            row = bitmap.buffer + (bitmap.rows - 1 - y) * bitmap.pitch;
        } else {
            row = bitmap.buffer - y * bitmap.pitch;
        }
        CAMLreturn (Val_int(row[x >> 3] & (128 >> (x & 7)) ? 255 : 0));
        break;

    default:
        failwith("read_Bitmap: unknown pixel mode");
    }

}

CAMLprim value get_Glyph_Metrics( vface )
     value vface;
{
    CAMLparam1(vface);
    CAMLlocal3(res1,res2,res);

    FT_Face *face = Ptr_val(vface);

    /* no soundness check ! */
    FT_Glyph_Metrics *metrics = &((*face)->glyph->metrics);

    res1 = alloc_tuple(3);
    Store_field(res1,0, Val_int(metrics->horiBearingX));
    Store_field(res1,1, Val_int(metrics->horiBearingY));
    Store_field(res1,2, Val_int(metrics->horiAdvance));

    res2 = alloc_tuple(3);
    Store_field(res2,0, Val_int(metrics->vertBearingX));
    Store_field(res2,1, Val_int(metrics->vertBearingY));
    Store_field(res2,2, Val_int(metrics->vertAdvance));

    res = alloc_tuple(4);
    Store_field(res,0, Val_int(metrics->width));
    Store_field(res,1, Val_int(metrics->height));
    Store_field(res,2, res1);
    Store_field(res,3, res2);

    CAMLreturn(res);
}

CAMLprim value get_Size_Metrics( vface )
     value vface;
{
    CAMLparam1(vface);
    CAMLlocal1(res);

    FT_Face *face = Ptr_val(vface);
    FT_Size_Metrics *imetrics = &((*face)->size->metrics);

    res = alloc_tuple(4);
    Store_field(res,0, Val_int(imetrics->x_ppem));
    Store_field(res,1, Val_int(imetrics->y_ppem));
    Store_field(res,2, Val_int(imetrics->x_scale));
    Store_field(res,3, Val_int(imetrics->y_scale));

    CAMLreturn(res);
}

CAMLprim value get_Outline_Contents(value vface)
{
    /* *****************************************************************

       Concrete definitions of TT_Outline might vary from version to
       version.

       This definition assumes freetype 2.0.1

       ( anyway, this function is wrong...)

       ***************************************************************** */
    CAMLparam1(vface);
    CAMLlocal5(points, tags, contours, res, tmp);

    int i;

    FT_Face *face = Ptr_val(vface);
    FT_Outline* outline = &((*face)->glyph->outline);

    int n_contours = outline->n_contours;
    int n_points   = outline->n_points;

    points   = alloc_tuple(n_points);
    tags     = alloc_tuple(n_points);
    contours = alloc_tuple(n_contours);

    for( i=0; i<n_points; i++ ) {
        FT_Vector* raw_points = outline->points;
        char* raw_flags  = outline->tags;
        tmp = alloc_tuple(2);
        /* caution: 26.6 fixed into 31 bit */
        Store_field(tmp, 0, Val_int(raw_points[i].x));
        Store_field(tmp, 1, Val_int(raw_points[i].y));
        Store_field(points, i, tmp);
        if ( raw_flags[i] & FT_Curve_Tag_On ) {
            Store_field(tags, i, Val_int(0)); /* On point */
        } else if ( raw_flags[i] & FT_Curve_Tag_Cubic ) {
            Store_field(tags, i, Val_int(2)); /* Off point, cubic */
        } else {
            Store_field(tags, i, Val_int(1)); /* Off point, conic */
        }
    }

    for( i=0; i<n_contours; i++ ) {
        short* raw_contours = outline->contours;
        Store_field(contours, i, Val_int(raw_contours[i]));
    }

    res = alloc_tuple(5);
    Store_field(res, 0, Val_int(n_contours));
    Store_field(res, 1, Val_int(n_points));
    Store_field(res, 2, points);
    Store_field(res, 3, tags);
    Store_field(res, 4, contours);

    CAMLreturn(res);
}

#else // HAS_FREETYPE

#include <caml/mlvalues.h>
#include <caml/alloc.h>
#include <caml/memory.h>
#include <caml/fail.h>

value init_FreeType(){ failwith("unsupported"); }
value done_FreeType(){ failwith("unsupported"); }
value new_Face(){ failwith("unsupported"); }
value face_info(){ failwith("unsupported"); }
value done_Face(){ failwith("unsupported"); }
value get_num_glyphs(){ failwith("unsupported"); }
value set_Char_Size(){ failwith("unsupported"); }
value set_Pixel_Sizes(){ failwith("unsupported"); }
value val_CharMap(){ failwith("unsupported"); }
value get_CharMaps(){ failwith("unsupported"); }
value set_CharMap(){ failwith("unsupported"); }
value get_Char_Index(){ failwith("unsupported"); }
value load_Glyph(){ failwith("unsupported"); }
value load_Char(){ failwith("unsupported"); }
value render_Glyph_of_Face(){ failwith("unsupported"); }
value render_Char(){ failwith("unsupported"); }
value set_Transform(){ failwith("unsupported"); }
value get_Bitmap_Info(){ failwith("unsupported"); }
value read_Bitmap(){ failwith("unsupported"); }
value get_Glyph_Metrics(){ failwith("unsupported"); }
value get_Size_Metrics(){ failwith("unsupported"); }
value get_Outline_Contents(){ failwith("unsupported"); }

#endif // HAS_FREETYPE
