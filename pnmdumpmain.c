#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>


////////////////////////////////////////////////////////////////////////////////
//                                   Enums                                    //
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief      An enum listing the supported types of Pgm file.
 *
 * @field      UNKNOWN  The type is unknown.
 * @field      P2       The type is of P2 format.
 * @field      P5       The type is of P5 format.
 */
enum PgmType
{
    UNKNOWN,
    P2,
    P5
};


/**
 * @brief      An enum listing the supported methods of image scaling.
 *
 * @field      NEAREST_NEIGHBOR  Nearest neighbour image scaling.
 * @field      BILINEAR          Bilinear image scaling.
 */
enum ScaleType
{
    NEAREST_NEIGHBOR,
    BILINEAR
};


////////////////////////////////////////////////////////////////////////////////
//                                  Structs                                   //
////////////////////////////////////////////////////////////////////////////////

// These declarations are required for compilation. ConversionArgs references
// PgmConverter which references ConversionArgs...
typedef struct PgmFile PgmFile;
typedef struct PgmConverter PgmConverter;
typedef struct ScaleArgs ScaleArgs;
typedef struct ConversionArgs ConversionArgs;


/**
 * @brief      A struct which holds information specific to a pgm file.
 *
 * @field      fStream       The input/output file stream for the pgm file.
 * @field      fName         The file name with which to open fStream.
 * @field      width         The number of columns in the image.
 * @field      height        The number of rows in the image.
 * @field      maxDataValue  The maximum value of the data.
 * @field      type          The pgm type, e.g. P2 or P5.
 */
struct PgmFile
{
    FILE *fStream;
    char *fName;
    int width;
    int height;
    int maxDataValue;
    enum PgmType type;
};


/**
 * @brief      Holds information specific to scaling requirements.
 * 
 * @field      scale      The command-line parameter from which to parse the
 *                        scale factor.
 * @field      scaleType  Method of image scaling, e.g. bilinear, etc.
 * @field      isEnlarge  True if the output image is larger than the input.
 * @field      hScale     The height (row) scale factor.
 * @field      wScale     The width (column) scale factor.
 */
struct ScaleArgs
{
    char *scale;
    enum ScaleType scaleType;
    bool isEnlarge;
    double hScale;
    double wScale;
};


/**
 * @brief      Holds information specific to conversion requirements.
 * 
 * @field      getData            A function which returns the required data.
 * @field      isSwapWidthHeight  True if a transformation swaps rows and
 *                                columns.
 * @field      isScale            True if the image is being resized.
 */
struct ConversionArgs
{
    int (*getData)(PgmConverter *c, int row, int col);
    bool isSwapWidthHeight;
    bool isScale;
};


/**
 * @brief      Holds information required to convert a pgm file in some manner.
 *
 * @field      iF     The input pgm file information.
 * @field      oF     The output pgm file information.
 * @field      cArgs  The conversion information.
 * @field      sArgs  The scaling information.
 * @field      data   The input data, parsed from iF.
 */
struct PgmConverter
{
    PgmFile *iF;
    PgmFile *oF;
    ConversionArgs *cArgs;
    ScaleArgs *sArgs;
    int data[512][512];
};


////////////////////////////////////////////////////////////////////////////////
//                            Function definitions                            //
////////////////////////////////////////////////////////////////////////////////

// Printing

void PrintUsage(FILE *outputStream);
bool TryPrintHexDump(int argc, char *argv[]);
void PrintHexDump(FILE *inputStream, FILE *outputStream);

// Parsing commandline parameters

bool TryParseScalar(PgmConverter *c);

// Pgm conversion

bool TryConvertPgm(PgmConverter *c);

// Reading input pgm

bool TryReadPgmInfo(PgmConverter *c);
bool TryReadPgmData(PgmConverter *c);

// Output Pgm initialization

bool TrySetPgmInfo(PgmConverter *c);

// Writing output pgm

void WritePgmInfo(PgmFile *oF);
void WritePgmData(PgmConverter *c);

// Data retrieval

int GetData(PgmConverter *c, int row, int col);
int GetReflectedData(PgmConverter *c, int row, int col);
int GetRotated90Data(PgmConverter *c, int row, int col);
int GetScaledNnData(PgmConverter *c, int row, int col);
int GetScaledUpBlData(PgmConverter *c, int row, int col);
int GetScaledDownBoxData(PgmConverter *c, int row, int col);

// Data processing

int ExtrapolateLinear(int x1, int x2);
double LinearInterpolate(double x, double fx1, double fx2);
double BilinearInterpolate(double x, double y, 
    double fxy11, double fxy12, double fxy21, double fxy22);

// Stream management

bool TryOpenStreams(PgmConverter *c);
void CloseStreams(PgmConverter *c);

// Enum functions

char* PgmTypeToStr(enum PgmType type);
enum PgmType StrToPgmType(char *str);


////////////////////////////////////////////////////////////////////////////////
//                                 Constants                                  //
////////////////////////////////////////////////////////////////////////////////

#define VERSION "1.0"


////////////////////////////////////////////////////////////////////////////////
//                                    Main                                    //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Main, program entry point.
 *
 * @param[in]  argc  The number of command line parameters.
 * @param      argv  An array of pointers to each command line parameter.
 *
 * @return     Returns 0 to indicate successful termination, 1 otherwise.
 */
int main(int argc, char *argv[])
{
    /* Validate command line parameters */

    // Ensure there more than one command line parameter, otherwise print error
    if (argc == 1)
    {
        fprintf(stderr, "pnmdump: bad arguments\n");
        PrintUsage(stderr);
        return 1;
    }
    // Else if command is "--version" and there is only one command
    else if (!strcmp(argv[1], "--version") && (argc == 2))
    {
        fprintf(stdout, "%s\n", VERSION);
    }
    // Else if command is "--usage" and there is only one command
    else if (!strcmp(argv[1], "--usage") && (argc == 2))
    {
        PrintUsage(stdout);
    }
    // Else if command is "--hexdump [INFILE]"
    else if (!strcmp(argv[1], "--hexdump"))
    {
        return !TryPrintHexDump(argc, argv);
    }
    // Else if command is "--P2toP5 [INFILE] [OUTFILE]"
    else if (!strcmp(argv[1], "--P2toP5") && (argc == 4))
    {
        PgmFile iF = { NULL, argv[2], 0, 0, 0, P2 };
        PgmFile oF = { NULL, argv[3], 0, 0, 0, P5 };
        ConversionArgs cArgs = { GetData, false, false };
        PgmConverter c = { &iF, &oF, &cArgs, NULL, {} };

        return !TryConvertPgm(&c);
    }
    // Else if command is "--P5toP2 [INFILE] [OUTFILE]"
    else if (!strcmp(argv[1], "--P5toP2") && (argc == 4))
    {
        PgmFile iF = { NULL, argv[2], 0, 0, 0, P5 };
        PgmFile oF = { NULL, argv[3], 0, 0, 0, P2 };
        ConversionArgs cArgs = { GetData, false, false };
        PgmConverter c = { &iF, &oF, &cArgs, NULL, {} };

        return !TryConvertPgm(&c);
    }
    // Else if command is "--rotate [INFILE] [OUTFILE]"
    else if (!strcmp(argv[1], "--rotate") && (argc == 4))
    {
        PgmFile iF = { NULL, argv[2], 0, 0, 0, UNKNOWN };
        PgmFile oF = { NULL, argv[3], 0, 0, 0, UNKNOWN };
        ConversionArgs cArgs = { GetReflectedData, true, false };
        PgmConverter c = { &iF, &oF, &cArgs, NULL, {} };

        return !TryConvertPgm(&c);
    }
    // Else if command is "--rotate90 [INFILE] [OUTFILE]"
    else if (!strcmp(argv[1], "--rotate90") && (argc == 4))
    {
        PgmFile iF = { NULL, argv[2], 0, 0, 0, UNKNOWN };
        PgmFile oF = { NULL, argv[3], 0, 0, 0, UNKNOWN };
        ConversionArgs cArgs = { GetRotated90Data, true, false };
        PgmConverter c = { &iF, &oF, &cArgs, NULL, {} };

        return !TryConvertPgm(&c);
    }
    // Else if command is "--scaleNn [SCALAR] [INFILE] [OUTFILE]"
    else if (!strcmp(argv[1], "--scaleNn") && (argc == 5))
    {
        PgmFile iF = { NULL, argv[3], 0, 0, 0, UNKNOWN };
        PgmFile oF = { NULL, argv[4], 0, 0, 0, UNKNOWN };
        ScaleArgs sArgs = { argv[2], NEAREST_NEIGHBOR, false, 1, 1 };
        ConversionArgs cArgs = { GetScaledNnData, false, true };
        PgmConverter c = { &iF, &oF, &cArgs, &sArgs, {} };

        return !TryConvertPgm(&c);
    }
    // Else if command is "--scaleBl [SCALAR] [INFILE] [OUTFILE]"
    else if (!strcmp(argv[1], "--scaleBl") && (argc == 5))
    {
        PgmFile iF = { NULL, argv[3], 0, 0, 0, UNKNOWN };
        PgmFile oF = { NULL, argv[4], 0, 0, 0, UNKNOWN };
        ScaleArgs sArgs = { argv[2], BILINEAR, false, 1, 1 };
        ConversionArgs cArgs = { NULL, false, true };
        PgmConverter c = { &iF, &oF, &cArgs, &sArgs, {} };

        return !TryConvertPgm(&c);
    }
    else
    // Else if no validation checks are passed, print error and usage
    {
        fprintf(stderr, "pnmdump: bad arguments\n");
        PrintUsage(stderr);
        return 1;
    }

    // If the program reaches here no validation checks have been failed;
    // exit with zero return code to indicate successful termination.
    return 0;
}


////////////////////////////////////////////////////////////////////////////////
//                             Printing functions                             //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Prints the program usage to a stream, typically stdout, stderr.
 *
 * @param      outputStream  The stream to print the usage to.
 */
void PrintUsage(FILE *outputStream)
{
    fprintf(outputStream, "Usage:\n"
        "./pnmdump.exe --version\n"
        "./pnmdump.exe --usage\n"
        "./pnmdump.exe --hexdump [FILE]\n");
}



/**
 * @brief      Prints the ASCII and hexadecimal representation of an input
 *             stream.
 *
 * @param[in]  argc  The command line parameter count.
 * @param      argv  The array of command line parameters.
 *
 * @return     Returns true when sucessful, false otherwise.
 */
bool TryPrintHexDump(int argc, char *argv[])
{
     // Check the length of the stdin file
        fseek(stdin, 0, SEEK_END);
        long stdinLength = ftell(stdin);
        fseek(stdin, 0, SEEK_SET);
 
        // If the length isn't equal to zero, print redirected input from stdin
        if (stdinLength != 0)
        {
            PrintHexDump(stdin, stdout);
            return true;
        }
        // Else, the input has not been redirected
        else
        {
            // If the argument count isn't equal to three we have bad arguments
            if (argc != 3)
            {
                fprintf(stderr, "pnmdump: bad arguments\n");
                PrintUsage(stderr);
                return false;
            }
 
            // Try and read from the file; argv[2]
            FILE* hexdumpStream = fopen(argv[2], "rb");
            if (hexdumpStream == NULL)
            {
                fprintf(stderr, "No such file: \"%s\"\n", argv[2]);
                return false;
            }
 
            // Print the hex dump to stdout and close the stream
            PrintHexDump(hexdumpStream, stdout);
            fclose(hexdumpStream);
        }

        return true;   
}


/*-------------------------------------------------------------------------*//**
 * @brief      Prints the binary contents of a stream to an output stream,
 *             showing the hexadecimal and ASCII representation of each byte.
 *
 * @param      inputStream   The stream which to translate into a hexdump.
 * @param      outputStream  The stream which to print the hexdump.
 */
void PrintHexDump(FILE *inputStream, FILE *outputStream)
{
    unsigned char chunk[8];  // Array to read into
    int bytesRead;
    int totalBytesRead = 0;

    // Iterate until the file has been completely read
    for (;;)
    {
        // Store the next 8 bytes in the chunk array
        bytesRead = fread(chunk, 1, 8, inputStream);

        // Exit if 0 bytes are read - the file has been completely read
        if (bytesRead == 0)
        {
            fprintf(outputStream, "%07x\n", totalBytesRead);
            break;
        }

        // Print the total byte count
        fprintf(outputStream, "%07x", totalBytesRead);

        // For each byte read print the hex and ASCII representations
        for (int i = 0; i < bytesRead; i++)
        {
            // Print the ASCII if it's "printable"
            if (((char)chunk[i] >= 32) && ((char)chunk[i] <= 126))
            {
                fprintf(outputStream, "  %02X %c", chunk[i], chunk[i]);
            }
            // Otherwise print a '.'
            else
            {
                fprintf(outputStream, "  %02X .", chunk[i], chunk[i]);
            }
        }

        // Once the chunk's parsed, print a newline and update totalBytesRead
        totalBytesRead += bytesRead;
        fprintf(outputStream, "\n");

        // Exit the loop if the number of bytes read is less than 8,
        // indicating that the file has been completely read.
        if (bytesRead < 8)
        {
            fprintf(outputStream, "%07x\n", totalBytesRead);
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
//                       Parsing commandline parameters                       //
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief      Parses the scale factor from the command-line parameter.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 *
 * @return     Returns true when sucessful, false otherwise.
 */
bool TryParseScalar(PgmConverter *c)
{
    // Containers for the scale factor. 'n' is to check for the null terminator
    double x = 0;
    double y = 0;
    double a = 0;
    double b = 0;
    int n = 0;

    // If the string starts with an 'm', the output is to be scaled down.
    c->sArgs->isEnlarge = c->sArgs->scale[0] == 109 ? false : true;

    // If a single factor is passed, scale width and height by the same factor
    if ((sscanf(c->sArgs->scale, "%lf%n", &x, &n) == 1
        && c->sArgs->scale[n] == '\0'))
    {
        c->sArgs->wScale = x;
        c->sArgs->hScale = x;
    }
    // If a single fraction is passed, scale width and height by the same factor
    else if ((sscanf(c->sArgs->scale, "%lf/%lf%n", &x, &y, &n) == 2
        && c->sArgs->scale[n] == '\0'))
    {
        c->sArgs->wScale = x / y;
        c->sArgs->hScale = x / y;
    }
    // If factors AxB are passed, scale width by A and height by B
    else if ((sscanf(c->sArgs->scale, "%lfx%lf%n", &x, &y, &n) == 2
        && c->sArgs->scale[n] == '\0'))
    {
        c->sArgs->wScale = x;
        c->sArgs->hScale = y;
    }
    // If two fractiosn are passed, scale dimensions by the respective factor
    else if ((sscanf(c->sArgs->scale, "%lf/%lfx%lf/%lf%n", &x, &y, &a, &b, &n) == 4
        && c->sArgs->scale[n] == '\0'))
    {
        c->sArgs->wScale = x / y;
        c->sArgs->hScale = a / b;
    }
    // If the string does not match accepted formats report error
    else
    {
        fprintf(stderr, "Error, bad scalar format. Check README for usage:\n");
        return false;
    }

    //
    if (c->sArgs->scaleType == BILINEAR)
    {
        if ((c->sArgs->wScale >= 1) && (c->sArgs->hScale >= 1))
        {
            c->cArgs->getData = GetScaledUpBlData;
        }
        else if ((c->sArgs->wScale <= 1) && (c->sArgs->hScale <= 1))
        {
            c->cArgs->getData = GetScaledDownBoxData;
        }
    }

    // Check that scaling is being done in the same way
    if ((c->sArgs->wScale < 1 && c->sArgs->hScale > 1)
        || (c->sArgs->wScale > 1 && c->sArgs->hScale < 1))
    {
        fprintf(stderr, "Error, width and height must be scaled in the same way"
            ", i.e. if width is scaled up height must also be scaled up.\n");
        return false;
    }

    // Scale factors cannot be zero or negative.
    if (c->sArgs->wScale <= 0 || c->sArgs->hScale <= 0)
    {
        fprintf(stderr, "Error, scalar must be a non zero positive.\n");
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
//                          Pgm conversion functions                          //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Converts a pgm file to another pgm format.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 *
 * @return     Returns true when sucessful, false otherwise.
 */
bool TryConvertPgm(PgmConverter *c)
{
    // Try and open the io files, parse the pgm data and info, reporting failure
    if (!TryOpenStreams(c) || !TryReadPgmInfo(c)
        || !TryReadPgmData(c) || !TrySetPgmInfo(c))
    {
        CloseStreams(c);
        return false;
    }

    // Write the output data
    WritePgmInfo(c->oF);
    WritePgmData(c);

    CloseStreams(c);
    return true;
}


////////////////////////////////////////////////////////////////////////////////
//                             Reading input pgm                              //
////////////////////////////////////////////////////////////////////////////////


/*-------------------------------------------------------------------------*//**
 * @brief      Reads the first four lines of information from a pgm file.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 *
 * @return     Returns true when successful, false otherwise.
 */
bool TryReadPgmInfo(PgmConverter *c)
{
    /*Parse the input image type, width, and height from lines 1, 3 and 4*/

    char typeStr[10];
    if (4 != fscanf(c->iF->fStream,
        "%s\n"       // Line 1 format: "P2\n"
        "%*[^\n]\n"  // Line 2 format: "[single line comment]\n", * = skip
        "%i %i\n"    // Line 3 format: "[width int] [height int]\n"    
        "%i\n",      // Line 4 format: "[maxsize int]\n"
        typeStr, &(c->iF->width), &(c->iF->height), &(c->iF->maxDataValue)))
    {
        // If the attempt was unsucessful report the error
        fprintf(stderr, "Corrupted input file\n");
        return false;
    }

    // Convert the parsed image type string to a PgmType enum
    enum PgmType imageType = StrToPgmType(typeStr);

    // If the input type is unknown, set it to the parsed input type (if valid)
    if (c->iF->type == UNKNOWN && imageType != UNKNOWN)
        c->iF->type = imageType;

    // If parsed pgm type does not match the expected input type, report error
    if(c->iF->type != imageType)
    {
        fprintf(stderr, "Input is not in %s format\n", PgmTypeToStr(c->iF->type));
        return false;
    }

    return true;
}


/*-------------------------------------------------------------------------*//**
 * @brief      Reads the data from a pgm file and stores it into a PgmConverter's
 *             data array.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 *
 * @return     Returns true when successful, false otherwise.
 */
bool TryReadPgmData(PgmConverter *c)
{
    int data = 0;
    int dataCount = 0;
    for (int row = 0; row < c->iF->height; row++)
    {
        for (int column = 0; column < c->iF->width; column++)
        {
            // If the input type is P2
            if (c->iF->type == P2)
            {
                // Scan the file for the next integer, ignoring whitespace
                if (!fscanf(c->iF->fStream, "%i", &data))
                {
                    // Error if fscanf failed to parse an integer (i.e. text)
                    fprintf(stderr, "Corrupted input file\n");
                    return false;
                }
            }
            // Else if the input type is P5 read the file character by character
            else if (c->iF->type == P5)
            {
                data = (int)fgetc(c->iF->fStream);
            }

            // If the end of the file has been reached, exit the loop
            if (feof(c->iF->fStream))
                break;

            // Increment the data count
            dataCount ++;

            // If the value of data exceeds limits, it is corrupted
            if ((data > c->iF->maxDataValue) || (data < 0))
            {
                fprintf(stderr, "Corrupted input file\n");
                return false;
            }
            else
            {
                c->data[row][column] = data;
            }
        }
    }

    // Input is corrupted if the input type is P5 and there is still information
    // to read, or if the datacount isn't equal to the image dimensions product.
    if ((c->iF->type == P5 && fgetc(c->iF->fStream) != EOF)
        || dataCount != (c->iF->width * c->iF->height))
    {
        fprintf(stderr, "Corrupted input file\n");
        return false;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
//                         Output pgm initialization                          //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Sets any output pgm information which is context sensitive.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 *
 * @return     Returns true when successful, false otherwise.
 */
bool TrySetPgmInfo(PgmConverter *c)
{
    // If the output type is unknown, set it to the parsed input type
    c->oF->type = (c->oF->type == UNKNOWN) ? c->iF->type : c->oF->type;

    // Set the output's width and height, swapped if a transformation requires
    if (c->cArgs->isSwapWidthHeight)
    {
        c->oF->width = c->iF->height;
        c->oF->height = c->iF->width;
    }
    else
    {
        c->oF->width = c->iF->width;
        c->oF->height = c->iF->height;
    }

    // Set the output's max data value equal to the input
    c->oF->maxDataValue = c->iF->maxDataValue;

    // Sets scale properties if the output is to be scaled.
    if (c->cArgs->isScale)
    {
        // Try and parse the scale factor from the command-line parameter
        if (!TryParseScalar(c))
            return false;

        // Set the scaled output width and height 
        c->oF->height *= c->sArgs->hScale;
        c->oF->width *= c->sArgs->wScale;

        // Max output size of the scaled output image is 1920x1080
        if (c->oF->width > 1920 || c->oF->height > 1080)
        {
            fprintf(stderr, "Error, output too large, max 1920x1080\n");
            return false;
        }
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////
//                             Writing output pgm                             //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Writes the first four lines of information to a pgm file.
 *
 * @param      oF    A PgmFile struct detailing the pgm files' information.
 */
void WritePgmInfo(PgmFile *oF)
{
    fprintf(oF->fStream,
        "%s\n"                          // [pgm type]\n
        "# Generated by pnmdump.exe\n"  // [comment]\n
        "%i %i\n"                       // [width] [height]\n
        "%i\n",                         // [max data value]\n
        PgmTypeToStr(oF->type), oF->width, oF->height, oF->maxDataValue);
}


/*-------------------------------------------------------------------------*//**
 * @brief      Writes the data from a PgmConverter's data array to a file.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 */
void WritePgmData(PgmConverter *c)
{
    // Write data[row][column] to the pgm [row][column]
    for (int row = 0; row < c->oF->height; row++)
    {
        for (int col = 0; col < c->oF->width; col++)
        {
            // If the output type is P2
            if (c->oF->type == P2)
            {
                // Pad with spaces unless we are on the first column of a row
                if (col == 0)
                    fprintf(c->oF->fStream, "%i", c->cArgs->getData(c, row, col));
                // Add a new line for the final col of a row
                else if (col == c->oF->width - 1)
                    fprintf(c->oF->fStream, " %i\n", c->cArgs->getData(c, row, col));
                else
                    fprintf(c->oF->fStream, " %i", c->cArgs->getData(c, row, col));
            }
            // Else if the input type is P5
            else if (c->oF->type == P5)
            {
                fprintf(c->oF->fStream, "%c", c->cArgs->getData(c, row, col));
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////
//                               Data retrieval                               //
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief      Returns the original pixel data for a given row and column.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 * @param[in]  row   The 0-indexed row where the pixel is being written to.
 * @param[in]  col   The 0-indexed column where the pixel is being written to.
 *
 * @return     The integer value of the pixel to be written at [row, col].
 */
int GetData(PgmConverter *c, int row, int col)
{
    return c->data[row][col];
}


/**
 * @brief      Returns the pixel data for a given row and column. The data
 *             returned is for if the array were reflected in y=-x.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 * @param[in]  row   The 0-indexed row where the pixel is being written to.
 * @param[in]  col   The 0-indexed column where the pixel is being written to.
 *
 * @return     The integer value of the pixel to be written at [row, col].
 */
int GetReflectedData(PgmConverter *c, int row, int col)
{
    return c->data[col][row];
}


/**
 * @brief      Returns the pixel data for a given row and column. The data
 *             returned is for if the array were rotated 90 degrees clockwise.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 * @param[in]  row   The 0-indexed row where the pixel is being written to.
 * @param[in]  col   The 0-indexed column where the pixel is being written to.
 *
 * @return     The integer value of the pixel to be written at [row, col].
 */
int GetRotated90Data(PgmConverter *c, int row, int col)
{
    return c->data[(c->oF->width - 1) - col][row];
}


/**
 * @brief      Returns the pixel data for a given row and column. The output row
 *             and column has been scaled by a factor, and so the data
 *             corresponding to the output row and column divided by this factor
 *             is returned.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 * @param[in]  row   The 0-indexed row where the pixel is being written to.
 * @param[in]  col   The 0-indexed column where the pixel is being written to.
 *
 * @return     The integer value of the pixel to be written at [row, col].
 */
int GetScaledNnData(PgmConverter *c, int row, int col)
{
    return c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)];
}


/**
 * @brief      Returns the pixel data for a given row and column. The output
 *             dimensions have been scaled up, generating new rows and columns
 *             inbetween the original. The data for these new rows and columns
 *             is generated using linear or bilinear interpolation. If data is
 *             not available for interpolation (i.e. at picture borders) then it
 *             is extrapolated from the existing data. This code is pretty messy
 *             and DRP could be enforced but I'm super out of time.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 * @param[in]  row   The 0-indexed row where the pixel is being written to.
 * @param[in]  col   The 0-indexed column where the pixel is being written to.
 *
 * @return     The integer value of the pixel to be written at [row, col].
 */
int GetScaledUpBlData(PgmConverter *c, int row, int col)
{
    /**
     * When scaling up, subpixels in new rows/columns (subrows/columns) are
     * generated using interpolation. For example, x2 scaling:
     * 
     *                0 1 2 3             0         1         2       3
     *   0 1        0 a ? b ?        0    a      (a+b)/2      b       b
     * 0 a b   =>   1 ? ? ? ?   =>   1 (a+x)/2 (a+b+x+y)/4 (b+y)/2 (b+y)/2
     * 1 x y        2 x ? y ?        2    x      (x+y)/2      y       y
     *              3 ? ? ? ?        3    x      (x+y)/2      y       y
     * 
     *                                 subcolumn         subcolumn
     *                           0        1        2        3    .. 
     *   0 1..              0    a     subpixel    b     subpixel..
     * 0 a b..   =>  subrow 1 subpixel subpixel subpixel subpixel..
     * 1 x y..              2    x     subpixel    y     subpixel..
     * : : :         subrow 3 subpixel subpixel subpixel subpixel..
     *                      :    :        :        :        :
    */

    // If at top left corner, extrapolate row and column data
    if (row < (int)(c->sArgs->hScale / 2)
        && col < (int)(c->sArgs->wScale / 2))
    {
        return (int)BilinearInterpolate(
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale) + 1]),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale)]),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) + 1]),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)]);
    }
    // If at top right corner, extrapolate row and column data
    else if (row < (int)(c->sArgs->hScale / 2)
        && col > c->oF->width - (int)((c->sArgs->wScale + 1) / 2))
    {
        return (int)BilinearInterpolate(
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale)]),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale) - 1]),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) - 1]));
    }
    // If at bottom left corner, extrapolate row and column data
    else if (row > c->oF->height - (int)((c->sArgs->hScale + 1) / 2)
        && col < (int)(c->sArgs->wScale / 2))
    {
        return (int)BilinearInterpolate(
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) + 1]),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) - 1][(int)(col / c->sArgs->wScale) + 1]),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) - 1][(int)(col / c->sArgs->wScale)]));
    }
    // If at bottom right corner, extrapolate row and column data
    else if (row > c->oF->height - (int)((c->sArgs->hScale + 1) / 2)
        && col > c->oF->width - (int)((c->sArgs->wScale + 1) / 2))
    {
        return (int)BilinearInterpolate(
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) - 1]),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) - 1][(int)(col / c->sArgs->wScale)]),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) - 1][(int)(col / c->sArgs->wScale) + 1]));
    }
    // If at the top picture edge, extrapolate the row data for interpolation
    else if (row < (int)(c->sArgs->hScale / 2))
    {
        return (int)LinearInterpolate(
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale)]),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)]);
    }
    // If at the bottom picture edge, extrapolate the row data for interpolation
    else if (row > c->oF->height - (int)((c->sArgs->hScale + 1) / 2))
    {
        return (int)LinearInterpolate(
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale) - 1][(int)(col / c->sArgs->wScale)]));
    }
    // If at the left picture edge, extrapolate the column data for interpolation
    else if (col < (int)(c->sArgs->wScale / 2))
    {
        return (int)LinearInterpolate(
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) + 1]),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)]);
    }
    // If at the right picture edge, extrapolate the column data for interpolation
    else if (col > c->oF->width - (int)((c->sArgs->wScale + 1) / 2))
    {
        return (int)LinearInterpolate(
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
            ExtrapolateLinear(
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
                c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) - 1]));
    }
    // Otherwise we're on a subrowcolumn => Bilinearly interpolate
    else
    {
        row -= (int)(c->sArgs->hScale / 2);
        col -= (int)(c->sArgs->wScale / 2);

        return (int)BilinearInterpolate(
            (col / c->sArgs->wScale) - (int)(col / c->sArgs->wScale),
            (row / c->sArgs->hScale) - (int)(row / c->sArgs->hScale),
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale)],
            c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale)],
            c->data[(int)(row / c->sArgs->hScale)][(int)(col / c->sArgs->wScale) + 1],
            c->data[(int)(row / c->sArgs->hScale) + 1][(int)(col / c->sArgs->wScale) + 1]);
    }
}


/**
 * @brief      Returns the pixel data for a given row and column. The output
 *             dimensions have been scaled down, meaning output rows and columns
 *             are generated from combined input rows and columns. The data is
 *             calculated from the average value of the original pixels.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 * @param[in]  row   The 0-indexed row where the pixel is being written to.
 * @param[in]  col   The 0-indexed column where the pixel is being written to.
 *
 * @return     The integer value of the pixel to be written at [row, col].
 */
int GetScaledDownBoxData(PgmConverter *c, int row, int col)
{
    /**
     * For example, scaling a 4x4 down by a factor of 2:
     * 
     *   0 1 2 3        
     * 0 a b c d               0           1     
     * 1 e f g h   =>   0 (a+b+e+f)/4 (c+d+g+h)/4
     * 2 i j k l        1 (i+j+m+n)/4 (k+l+o+p)/4
     * 3 m n o p
    */

    int data = 0;
    int count = 0;

    // For each row and column to be combined into one
    for (int rs = 0; rs < 1 / c->sArgs->hScale; rs++)
    {
        for (int cs = 0; cs < 1 / c->sArgs->wScale; cs++)
        {
            // Sum the pixel data for each pixel in the original set.
            data += c->data[(int)(row / c->sArgs->hScale + rs)][(int)(col / c->sArgs->wScale + cs)];
            count++;
        }

    }

    // // return the average pixel value 
    return data / count;
}


////////////////////////////////////////////////////////////////////////////////
//                              Data processing                               //
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief      Extrapolates the pixel value for a row or column out of bounds of
 *             the original data set.
 *
 * @param[in]  x1    The border pixel value.
 * @param[in]  x2    The pixel value one position deeper into the image.
 *
 * @return     The extrpolated pixel value, within the range [0, 255]
 */
int ExtrapolateLinear(int x1, int x2)
{
    int x0 = x1 - (x2 - x1);
    return x0 > 255 ? 255 : x0 < 0 ? 0 : x0;
}


/**
 * @brief      Linearly interpolates the various parameters. It is assumed
 *             that the interval [x1, x2]  has been scaled such that x1 = 0, and
 *             x2 = 1.
 *             Reference: https://en.wikipedia.org/wiki/Linear_interpolation
 *
 * @param[in]  x     The value of x for which to estimate f(x).
 * @param[in]  fx1   The value of f(x) at x = x1 = 0.
 * @param[in]  fx2   The value of f(x) at x = x2.
 *
 * @return     Returns the rounded down estimation of f(x) at x.
 */
double LinearInterpolate(double x, double fx1, double fx2)
{
    return ((fx1 * (1 - x)) + (fx2 * (x)));
}


/**
 * @brief      Bilinearly interpolates the various parameters. It is assumed
 *             that the interval [x1, x2]  has been scaled such that x1 = 0, and
 *             x2 = 1.
 *             Reference: https://en.wikipedia.org/wiki/Bilinear_interpolation
 *
 * @param[in]  x      The value of x for which to estimate f(x,y).
 * @param[in]  y      The value of y for which to estimate f(x,y).
 * @param[in]  dx     The change of x over the itnerval [x1, x2]. This function
 *                    assumes that x1 = 0 and x2 has been scaled appropriately.
 * @param[in]  dy     The change of y over the itnerval [y1, y2]. This function
 *                    assumes that y1 = 0 and y2 has been scaled appropriately.
 * @param[in]  fxy11  The value of f(x,y) at x = x1 and y = y1.
 * @param[in]  fxy12  The value of f(x,y) at x = x1 and y = y2.
 * @param[in]  fxy21  The value of f(x,y) at x = x2 and y = y1.
 * @param[in]  fxy22  The value of f(x,y) at x = x2 and y = y2.
 *
 * @return     Returns the rounded down estimation of f(x,y) at x,y.
 */
double BilinearInterpolate(double x, double y, 
    double fxy11, double fxy12, double fxy21, double fxy22)
{
    return LinearInterpolate(y,
        LinearInterpolate(x, fxy11, fxy21),
        LinearInterpolate(x, fxy12, fxy22));
}


////////////////////////////////////////////////////////////////////////////////
//                             Stream management                              //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Attempts to open input and output streams of a PgmConverter.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 *
 * @return     Returns true when successful, false otherwise.
 */
bool TryOpenStreams(PgmConverter *c)
{
    // If we were unsuccessful in opening the stream report the failure
    c->iF->fStream = fopen(c->iF->fName, "rb");
    if (c->iF->fStream == NULL)
    {
        fprintf(stderr, "No such file: \"%s\"\n", c->iF->fName);
        return false;
    }

    c->oF->fStream = fopen(c->oF->fName, "wb");

    return true;
}


/*-------------------------------------------------------------------------*//**
 * @brief      Closes the streams of a PgmConverter struct.
 *
 * @param      c     A PgmConverter detailing conversion state information.
 */
void CloseStreams(PgmConverter *c)
{
    fclose(c->iF->fStream);
    fclose(c->oF->fStream);
}


////////////////////////////////////////////////////////////////////////////////
//                               Enum functions                               //
////////////////////////////////////////////////////////////////////////////////

/*-------------------------------------------------------------------------*//**
 * @brief      Converts a PgmType enum to its string representation.
 *
 * @param[in]  type  The PgmType enum to convert.
 *
 * @return     A pointer to the string representation, or NULL if no match.
 */
char* PgmTypeToStr(enum PgmType type)
{
    // Compare the PgmType and return the matching string
    switch(type)
    {
        case P2:
            return "P2";
        case P5:
            return "P5";
        default:
            return NULL;
    }
}


/*-------------------------------------------------------------------------*//**
 * @brief      Converts a string to its PgmType enum representation.
 *
 * @param      string  The string to convert to a pgmType enum.
 *
 * @return     The matching PgmType, or UNKNOWN if no match.
 */
enum PgmType StrToPgmType(char* string)
{
    // Compare the strings and return the matching PgmType
    if (!strcmp(string, "P2"))
        return P2;
    else if (!strcmp(string, "P5"))
        return P5;
    else
        return UNKNOWN;
}
