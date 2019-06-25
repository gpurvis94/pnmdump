Authors: Joanna Hunter Gordon, George Purvis.
Version: 1.0
Date: 2018/03/23

Program Description
This program works with pgm files of format P2 and P5. There are a variety of
different commands that can be used that alter pgm files including; converting
it, rotating it and changing its size. The program usage is detailed below.

Usage
Write commands after the exe name as command-line parameters.

"--version"
    - The version number is printed to stdout.
"--usage"
    - The usage is printed to stdout.
"--hexdump [INFILE]"
    - Prints the content of the file in ASCII and hexadecimal representation.
"--P2toP5 [INFILE] [OUTFILE]"
    - Converts pgm file of P2 format to a pgm file of P5 format. 
"--P5toP2 [INFILE] [OUTFILE]"
    - Converts a pgm file of P5 format to converts pgm file of P2.
"--rotate [INFILE] [OUTFILE]"
    - Reflects pgm in the line y=x, keeps input type and output type the same.
"--rotate90 [INFILE] [OUTFILE]"
    - Rotates pgm file by 90 degrees, keeps input type and output type the same.
"--scaleNn [SCALAR] [INFILE] [OUTFILE]"
    - Scales an image using nearest neighbour interpolation.
"--scaleBl [SCALAR] [INFILE] [OUTFILE]"
    - Scales an image using nearest bilinear interpolation.
[SCALAR] format
    - The scalar can be given as a double with the follow formats. If the second
      format is specified, the input is treated as a fraction:
        [SCALAR] = [double], [double]/[double]
    - The scalar can also be given with the following format. In this case the
      width is scaled by a factor equal to the first scalar, and the height is
      scaled by a factor equal to the second scalar.
        [SCALAR] = [SCALAR]x[SCALAR]

Example usage of scalar command:
pnmdump.exe --scaleBl 0.9 tests/feep_p2.pgm output.pgm
    - Scales width and height to 0.9x of the original value.
pnmdump.exe --scaleNn 2x3 tests/feep_p2.pgm output.pgm
    - Scales width to 2x and height 3x of the original value.


Limitations:
    - Input files must be no larger than 512x512.
    - Output files must be no larger than 1920x1080.
    - When scaling images, the height must be scaled in the same manner as the
      width. That is, if the width is being scaled by a factor >= 1, so must the
      height, and equivalently for factors <= 1.
    - When scaling bilinearly for factors < 1, box interpolation is used.


\To compile the code type the following into the terminal:
    $ make
    
Changelog:
    date Version (1.0)
        - Program completed
