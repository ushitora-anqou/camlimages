(***********************************************************************)
(*                                                                     *)
(*                           Objective Caml                            *)
(*                                                                     *)
(*            Fran�ois Pessaux, projet Cristal, INRIA Rocquencourt     *)
(*            Pierre Weis, projet Cristal, INRIA Rocquencourt          *)
(*            Jun Furuse, projet Cristal, INRIA Rocquencourt           *)
(*                                                                     *)
(*  Copyright 1999-2004                                                *)
(*  Institut National de Recherche en Informatique et en Automatique.  *)
(*  Distributed only by permission.                                    *)
(*                                                                     *)
(***********************************************************************)

(* $Id: jpeg.mli,v 1.2 2008/05/17 19:55:50 furuse Exp $ *)

module Marker : sig

  (** Jpeg Marker such as EXIF *)
  type t = 
    | Comment of string
    | App of int * string

  val format : Format.formatter -> t -> unit

end

val load : string -> Images.load_option list -> Images.t
  (** Loads a jpeg image. *)

val load_thumbnail : string -> Images.load_option list -> Geometry.spec -> 
  int * int * Images.t;;

val save : string -> Images.save_option list -> Images.t -> unit
  (** Save a full-color image in jpeg format file.
     Raises [Invalid_argument] if the image is not a full-color image. *)

val save_with_markers : string -> Images.save_option list -> Images.t -> Marker.t list -> unit
  (** Same as [save] but it also writes markers *)

val save_as_cmyk : string -> Images.save_option list -> 
  (Images.rgb -> int * int * int * int) -> Images.t -> unit
  (** This converts RGB images to CMYK, but the color conversion is not
     something fancy. *)

val save_cmyk_sample : string -> Images.save_option list -> unit
  (** Create CMYK jpeg image sample. Just for developpers. *)

(** Scanline based I/O functions *)
type in_handle;;

val open_in : string -> int * int * in_handle * Marker.t list;;
val open_in_thumbnail :
  string -> Geometry.spec -> int * int * (int * int * in_handle) * Marker.t list;;
val read_scanline : in_handle -> bytes -> int -> unit;;
val close_in : in_handle -> unit;;
  
type out_handle;;

val open_out : string -> int -> int -> int -> out_handle;;
val write_marker : out_handle -> Marker.t -> unit;;
val write_scanline : out_handle -> bytes -> unit;;
val close_out : out_handle -> unit;;

val check_header : string -> Images.header
  (** Checks the file header *)

val read_markers : string -> Marker.t list
  (** Open the file, read the markers, then close it immediately. *)
