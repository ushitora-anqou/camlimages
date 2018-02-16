open Configurator
open Spotlib.Spot

external (&) : ('a -> 'b) -> 'a -> 'b = "%apply"
(** Haskell's [($)]. *)

let ( ^/ ) = Filename.concat

let path_sep =
  if Sys.win32 then
    ';'
  else
    ':'

let exe = if Sys.win32 then ".exe" else ""

let get_path () =
  match Sys.getenv "PATH" with
  | exception Not_found -> []
  | s -> String.split (fun x -> x = path_sep) s

let find_file_in bases dirs =
  flip List.find_map_opt dirs & fun dir ->
    flip List.find_map_opt bases & fun base ->
      let path = dir ^/ base in
      if Sys.file_exists path then Some path else None

let find_program prog =
  let prog = prog ^ exe in
  find_file_in [prog] & get_path ()
  
let find_ocaml_program prog =
  let prog_opt = prog ^ ".opt" ^ exe in
  let prog_byte = prog ^ ".byte" in (* XXX need to check in Windows *)
  find_file_in [prog_opt; prog; prog_byte ] & get_path ()

let () = Findlib.init ()

let find_ocaml_package n =
  match Findlib.package_directory n with
  | s -> Some s
  | exception Findlib.No_such_package _ -> None
