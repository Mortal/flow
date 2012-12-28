Terrain generation
==================

An implementation of the
[diamond-square algorithm](http://en.wikipedia.org/wiki/Diamond-square_algorithm)
to generate realistic looking heightmaps.

Requires Boost program options and Boost chrono.

Build with `make`. For usage instructions, see `--help`.

The default output format is ASCII. For a greyscale binary Netpbm, use the
`--pgm` switch. This can be converted to other image formats using e.g. the
ImageMagick `convert` utility.

```sh
./height --pgm > map.pgm
convert map.pgm map.png
```

Furthermore, a heatmap filter can be applied with ImageMagick by applying a
color-lookup table (CLUT). In the following example, a 1x4 CLUT is generated,
and the `-clut` option will translate the black-to-white values in the input
map to graded colors.

```sh
convert map.pgm \( xc:black xc:black xc:blue xc:cyan +append \) -clut heatmap.png
```

Light maps
----------

```sh
$ ./height --roughness 5.0 --size 500 --light 5.0 --pgm > light.pgm
Seed: 44336164721644
$ ./height --roughness 5.0 --size 500 --seed 44336164721644 --pgm > map.pgm
Seed: 44336164721644
$ convert light.pgm light.png
$ convert map.pgm \( xc:red xc:yellow xc:blue +append \) -clut map.png
$ convert light.png map.png -compose multiply -composite composite.png
```

```sh
$ ./height --size 1920x1080 --seed 1290978805858707 --roughness 3 --pgm > bg.pgm
$ ./height --size 1920x1080 --seed 1290978805858707 --roughness 3 --pgm --light 5.0 > bglight.pgm
$ convert \( bg.pgm \( xc:white xc:green xc:green xc:PaleGreen3 xc:DodgerBlue \
                       xc:DodgerBlue xc:DodgerBlue xc:DodgerBlue +append \) \
             -clut \) \
          \( bg.pgm bglight.pgm -compose screen -composite \) \
          -compose multiply -composite composite.png
```
