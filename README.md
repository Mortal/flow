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
