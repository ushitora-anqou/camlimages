val ( ^/ ) : string -> string -> string
(** File path concatenation *)
  
val path_sep : char
(** PATH variable seprator *)

val exe : string
(** executable extension *)

val get_path : unit -> string list
(** Get the PATH compontents *)

val find_file_in : string list -> string list -> string option
(** [find_file_in bases dirs] find one of the file specified in [bases] in the directories [dirs] *)

val find_program : string -> string option
(** Find a program in the PATH, ex. [find_program "ls" = "ls.exe"] *)

val find_ocaml_program : string -> string option
(** Find an OCaml program in the PATH, ex. [find_ocaml_program "foo" = "foo.opt.exe"] *)

val find_ocaml_package : string -> string option
(** Find OCamlFind package and returns its directory if found *)
