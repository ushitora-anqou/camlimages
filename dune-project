(lang dune 3.2)
(name camlimages)

(generate_opam_files true)

(source (gitlab camlspotter/camlimages))

(authors "Jun FURUSE <jun.furuse@gmail.com>")

(maintainers "Jun FURUSE <jun.furuse@gmail.com>")

(license "LGPL-2.1-only WITH OCaml-LGPL-linking-exception")

(package
 (name camlimages)
 (synopsis "Image processing library")
 (description "An image processing library, which provides loading and saving various image formats with an interface for the Caml graphics library. It has also an interface with the freetype library to draw texts using truetype fonts.")
 (version 5.0.5)
 (depends
   (ocaml (>= 4.12.1))
   dune
   (base :build)
   (stdio :build)
   (cppo :build)
   (dune-configurator (and :build (>= 2.0.0)))
   (ocamlfind :build))
 (depopts
  lablgtk
  graphics
  conf-libpng
  conf-libjpeg
  ; conf-libexif
  ; conf-libtiff
  ; conf-libxpm
  conf-freetype
  conf-libgif
  conf-ghostscript))
