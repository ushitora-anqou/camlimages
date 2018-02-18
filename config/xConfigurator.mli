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
(** Find a program in the PATH, ex. [find_program "ls" = Some "ls.exe"] *)

val find_ocaml_program : string -> string option
(** Find an OCaml program in the PATH, ex. [find_ocaml_program "foo" = Some "foo.opt.exe"] *)

val find_ocaml_package : string -> string option
(** Find OCamlFind package and returns its directory if found *)

module Package_conf : sig
  type t = Configurator.Pkg_config.package_conf
    = { libs   : string list
      ; cflags : string list
      } [@@deriving conv{ocaml}]

  val merge : t -> t -> t
  
  val empty : t
end

type item =
  | Pkg_config   of unit         option
  | File         of string       option
  | Program      of string       option
  | Library      of Package_conf.t option
  | OCamlLibrary of string       option

[@@deriving conv{ocaml}]

module Make(A : sig val name : string end) : sig
  val pkg_config : item

  val find_program
    :  string
    -> item

  val find_ocaml_package
    :  string
    -> item

  val by_pkg_config
    :  string
    -> unit
    -> Package_conf.t option

  val by_cc
    :  string list
    -> string list
    -> string list
    -> string list
    -> unit
    -> Package_conf.t option

  val find_library
    : (unit -> Package_conf.t option) list
    -> item

  val find_file
    :  string
    -> string list
    -> item

  val make_header : fname:string -> (string * item) list -> unit

  val write_package_conf_sexps : string -> item list -> unit
end
