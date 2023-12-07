let build_image () =
  let i = Rgb24.create 256 256 in
  for x = 0 to 255 do
    for y = 0 to 255 do
      Rgb24.set i x y
        { r= x;
          g= y;
          b= (x * y) mod 256 }
    done
  done;
  i

let jpg i =
  Jpeg.save "tmp.jpg" [] (Rgb24 i);
  ignore @@ Jpeg.load "tmp.jpg" []

let png i =
  Png.save "tmp.png" [] (Rgb24 i);
  ignore @@ Png.load "tmp.png" []

let gif i =
  Gif.save_image "tmp.gif" [] (Index8 i);
  ignore @@ Gif.load "tmp.gif" []

let tiff i =
  Tiff.save "tmp.tiff" [] (Rgb24 i);
  ignore @@ Tiff.load "tmp.tiff" []

let xpm () =
  List.iter (fun f ->
      ignore @@ Xpm.load f [])
    [
      "./test/images/xpm.xpm";
      "./examples/liv/Folder.xpm";
      "./examples/liv/BulletHole.xpm";
      "./examples/liv/Monalisa.xpm";
      "./examples/liv/FilesLink.xpm";
      "./examples/liv/FolderLink.xpm";
      "./examples/liv/sound.xpm";
      "./examples/liv/File.xpm";
      "./examples/liv/FileUnknown.xpm"
    ]

let draw_text img x y s =
  let module FtDraw = Fttext.Make(Rgb24) in
  let library = Freetype.init () in
  let face, _face_info = Freetype.new_face library "test/micap.ttf" 0 in
  Freetype.set_char_size face 36.0 36.0 144 144;
  let str = Fttext.unicode_of_latin s in
  let _x1,_y1,_x2,_y2 = Fttext.size face str in
  FtDraw.draw_text face Fttext.func_darken_only img
    x y
    (* (- (truncate x1)) (truncate y2) *) str

let () =
  for _ = 0 to 100 do
    let i = build_image () in
    draw_text i 100 100 "hello";
    let i8 = Reduce.error_diffuse i Color.r3g3b2 in
    jpg i;
    png i;
    gif i8;
    tiff i;
    xpm ()
  done
