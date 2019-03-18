# nuketools
> nuketools is a collection of various tools I made for a 2D compositing module at university. My computer science background and interest in pipeline development pushed me to focus on creating a more technologically-focused product. They were written to work with NukeX9.0v6 which requires Microsoft's 2010 compiler, so your mileage may vary with other versions of Nuke.  Check the latest NDK to see if this has changed since. Please note that the projects may contain references to my local drive (for instance, post-build scripts), and therefore may need to be updated (although the project itself will compile without issue). Due to how the NDK API is structured, the code may be somewhat unclean in certain parts.

## Usage
You will have to compile all but `multi render`, as that's just a Python script.  When this project was made, the NDK required Microsoft's 2010 C++ toolchain.  This may have changed.  Visual Studio SLN files are provided for all C++ projects, so providing the API didn't change, there should not be much work to get them compiling correctly.

## bumpy
Converts and input image into a normal map with options for different algorithms, normalization, etc.

![bumpy](https://raw.githubusercontent.com/KasumiL5x/nuketools/master/screenshots/bumpy.png)

## check
Generates a checkerboard mask with options for fuzziness, rotation, and scale.

![check](https://raw.githubusercontent.com/KasumiL5x/nuketools/master/screenshots/check.png)

## fractal
Generates masks for various fractals with detailed control over each fractal.

![check](https://raw.githubusercontent.com/KasumiL5x/nuketools/master/screenshots/fractal.png)

## gradient
Generates a radial gradient mask based on a custom user-provided curve and position.

![check](https://raw.githubusercontent.com/KasumiL5x/nuketools/master/screenshots/gradient.png)

## kirei
Various image filters, such as temperature grading, channel mixing, vignette, and many more.

![check](https://raw.githubusercontent.com/KasumiL5x/nuketools/master/screenshots/kirei.png)

## multi render
Script to render all selected nodes automagically.

![check](https://raw.githubusercontent.com/KasumiL5x/nuketools/master/screenshots/multirender.png)