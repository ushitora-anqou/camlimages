open Camlon
open Ocaml_conv.Default

module C = struct
  include Configurator
  include XConfigurator
end

external (&) : ('a -> 'b) -> 'a -> 'b = "%apply"
(** Haskell's [($)]. *)

type package_conf 
  = C.Pkg_config.package_conf
  = { libs   : string list
    ; cflags : string list
    } [@@deriving conv{ocaml}]

let options_of_package_conf {libs; cflags} = String.concat " " (libs @ cflags)
  
let t = C.create "camlimages"

type conf = 
  | Library of package_conf
  | Pkg_config
  | Program of string
  | OCamlLibrary of string
[@@deriving conv{ocaml}]

type pack = 
  { conf : conf
  ; info : string option
  } [@@deriving conv{ocaml}]

type item = 
  { name : string
  ; res : (pack, string) result
  } [@@deriving conv{ocaml}]

let (!%) fmt = Printf.sprintf fmt
let (!!%) fmt = Format.eprintf fmt

let log fmt = Format.eprintf fmt

let format_item = Ocaml.format_with [%derive.ocaml_of: item]

let log_item i = 
  match i.res with
  | Ok pack -> 
      begin match pack.info with
      | None -> Format.eprintf "%s:\t(found)@." i.name
      | Some info -> Format.eprintf "%s:\t(found : %s)@." i.name info
      end
  | Error e -> Format.eprintf "%s:\t(not found : %s)@." i.name e

let pkg_config, item_pkg_config = 
  log "Checking pkg-config in $PATH... ";
  let pkg_config = C.Pkg_config.get t in
  let res = match pkg_config with
    | None -> 
        log "(not found)";
        Error "not found"
    | Some p -> 
        log "(found)";
        Ok { conf= Pkg_config; info = None }
  in
  log "@.";
  let item = { name= "pkg-config"; res } in
  pkg_config, item

let find_program n = 
  log "Checking program %s in $PATH... " n;
  let res =
    match C.find_program n with
    | None -> 
        log "(not found)";
        Error [!% "not found in PATH"]
    | Some n -> 
        log "(found : %s)" n;
        Ok { conf= Program n; info= Some n }
  in
  log "@.";
  res

let find_ocaml_package n = 
  log "Checking OCaml library %s... " n;
  let res = match C.find_ocaml_package n with
    | Some s -> 
        log "(found : %s)" s;
        Ok { conf= OCamlLibrary s; info = Some s }
    | None -> 
        log "(not found)";
        Error ["not found by ocamlfind"]
  in
  log "@.";
  res

let by_pkg_config package = 
  log "Checking %s by pkg-config... " package;
  let res = match pkg_config with
    | None -> 
        log "(not found : needs pkg-config)";
        Error "needs pkg-config"
    | Some pkgcfg ->
        match C.Pkg_config.query pkgcfg ~package with
        | None -> Error "no pkg-config package"
        | Some conf -> Ok { conf= Library conf; info = Some (options_of_package_conf conf) }
  in
  begin match res with
    | Ok { info = Some x } -> log "(found : %s)" x
    | Ok { info = None } -> log "(found)"
    | Error e -> log "(not found : %s)" e
  end;
  log "@.";
  res

let by_cc c_flags link_flags headers fnames package =
  log "Checking %s using C compiler... " package;
  let includes = List.map (Printf.sprintf "#include <%s>") headers in
  let fcalls = List.map (Printf.sprintf "  ( (void(*)()) (%s) )();") fnames in
  let code = 
    String.concat "\n" 
    & includes 
      @ [ "int main(int argc, char **argv) {" ]
      @ fcalls
      @ [ "return 0; }"
        ; "\n"
        ]
  in
  let res = 
    if 
      C.c_test t 
        ~c_flags 
        ~link_flags
        code
    then begin
      let conf = { libs= link_flags; cflags= c_flags } in
      log "(found : %s)" (options_of_package_conf conf);
      Ok { conf= Library conf; info= Some (options_of_package_conf conf) }
    end else begin
      log "(not found : compilation failure)";
      Error (!% "C compilation test failed: %s" code)
    end
  in
  log "@.";
  res

let find_library name tests =
  let rec f rev_errors = function 
    | [] -> Error (List.rev rev_errors)
    | t::ts ->
        match t name with
        | Ok x -> Ok x
        | Error e -> f (e::rev_errors) ts
  in
  f [] tests

let lablgtk2 = find_ocaml_package "lablgtk2"
let graphics = find_ocaml_package "graphics"
let gs = find_program "gs"
let gif = find_library "libgif" 
    [ by_pkg_config
    ;  by_cc [] ["-lgif"] ["gif_lib.h"] ["DGifOpenFileName"] 
    ]
let jpeg = find_library "libjpeg" 
    [ by_pkg_config
    ; by_cc [] ["-ljpeg"] ["jpeglib.h"] ["jpeg_read_header"]
    ]
let png  = find_library "libpng" 
    [ by_pkg_config
    ; by_cc [] ["-lpng"; "-lz"] ["png.h"] ["png_create_read_struct"] 
    ]
let tiff = find_library "libtiff-4" 
    [ by_pkg_config
    ; by_cc [] ["-ltiff"] ["tiff.h"] ["TiffOpen"]
    ]
let freetype = find_library "freetype2" 
    [ by_pkg_config ]
let exif = find_library "libexif" 
    [ by_pkg_config
    ; by_cc [] ["-lexif"] ["exif-data.h"] ["exif_data_load_data"]
    ]
let xpm = find_library "xpm" 
    [ by_pkg_config
    ; by_cc [] ["-lXpm"] ["X11/xpm.h"] ["XpmReadFileToXpmImage"]
    ]

let rgb_txt = C.find_file_in ["rgb.txt"] ["/etc/X11"; "/usr/share/X11"]
  
let supported_libraries =
  [ "lib_gif", gif
  ; "lib_png", png
  ; "lib_jpeg", jpeg
  ; "lib_tiff", tiff
  ; "lib_freetype", freetype
  ; "lib_ps", gs
  ; "lib_xpm", xpm
  ; "lib_exif", exif
  ]

let external_files =
  [ "path_rgb_txt", rgb_txt
  ; "path_gs", gs
  ]
