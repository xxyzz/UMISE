/* Origin code: https://slexy.org/view/s21wNL2X3x */

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <libgen.h> // dirname
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <unistd.h>   // access, getopt

#define DWORD uint32_t

#define DDSD_CAPS 0x00000001
#define DDSD_HEIGHT 0x00000002
#define DDSD_WIDTH 0x00000004
#define DDSD_PITCH 0x00000008
#define DDSD_PIXELFORMAT 0x00001000
#define DDSD_MIPMAPCOUNT 0x00020000
#define DDSD_LINEARSIZE 0x00080000
#define DDSD_DEPTH 0x00800000
#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_FOURCC 0x00000004
#define DDPF_RGB 0x00000040
#define DDSCAPS_COMPLEX 0x00000008
#define DDSCAPS_TEXTURE 0x00001000
#define DDSCAPS_MIPMAP 0x00400000
#define DDSCAPS2_CUBEMAP 0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDSCAPS2_VOLUME 0x00200000

typedef struct {
  DWORD dwMagic;
  DWORD dwSize;
  DWORD dwFlags;
  DWORD dwHeight;
  DWORD dwWidth;
  DWORD dwPitchOrLinearSize;
  DWORD dwDepth;
  DWORD dwMipMapCount;
  DWORD dwReserved1[11];
  /*  DDPIXELFORMAT   */
  struct {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    DWORD dwRGBBitCount;
    DWORD dwRBitMask;
    DWORD dwGBitMask;
    DWORD dwBBitMask;
    DWORD dwAlphaBitMask;
  } sPixelFormat;
  /*  DDCAPS2 */
  struct {
    DWORD dwCaps1;
    DWORD dwCaps2;
    DWORD dwDDSX;
    DWORD dwReserved;
  } sCaps;
  DWORD dwReserved2;
} DDS_header;

typedef struct PakHeader {
  DWORD magic; /* KAPL -> "LPAK" */
  float version;
  DWORD startOfIndex;       /* -> 1 DWORD per file */
  DWORD startOfFileEntries; /* -> 5 DWORD per file */
  DWORD startOfFileNames;   /* zero-terminated string */
  DWORD startOfData;
  DWORD sizeOfIndex;
  DWORD sizeOfFileEntries;
  DWORD sizeOfFileNames;
  DWORD sizeOfData;
} PakHeader;

typedef struct PakFileEntry {
  DWORD fileDataPos; /* + startOfData */
  DWORD fileNamePos; /* + startOfFileNames */
  DWORD dataSize;
  DWORD dataSize2;  /* real size? (always =dataSize) */
  DWORD compressed; /* compressed? (always 0) */
} PakFileEntry;

/* Recursive mkdir from
 * http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
 */
static void _mkdir(const char *path) {
  char opath[256];
  char *p;
  size_t len;

  strncpy(opath, path, sizeof(opath));
  len = strlen(opath);
  if (opath[len - 1] == '/')
    opath[len - 1] = '\0';
  for (p = opath; *p; p++)
    if (*p == '/') {
      *p = '\0';
      if (access(opath, F_OK))
        mkdir(opath, S_IRWXU);
      *p = '/';
    }
  if (access(opath, F_OK)) /* if path is not terminated with / */
    mkdir(opath, S_IRWXU);
}

/* Write out a dds file from dxt data */
void write_dds(char *fileName, char *DDS_data, DWORD DDS_size) {
  DDS_header header;
  FILE *fout;
  DWORD *int_data = (DWORD *)DDS_data;
  DDS_size -= 12;
  DDS_data += 12;
  memset(&header, 0, sizeof(DDS_header));
  header.dwMagic = ('D' << 0) | ('D' << 8) | ('S' << 16) | (' ' << 24);
  header.sPixelFormat.dwFourCC = int_data[0];
  header.dwWidth = int_data[1];
  header.dwHeight = int_data[2];
  header.dwSize = 124;
  header.dwFlags =
      DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_LINEARSIZE;
  header.dwPitchOrLinearSize = DDS_size;
  header.sPixelFormat.dwSize = 32;
  header.sPixelFormat.dwFlags = DDPF_FOURCC;
  header.sCaps.dwCaps1 = DDSCAPS_TEXTURE;
  fout = fopen(fileName, "wb");
  fwrite(&header, sizeof(DDS_header), 1, fout);
  fwrite(DDS_data, 1, DDS_size, fout);
  fclose(fout);
}

int main(int argc, char *argv[]) {
  PakHeader header;
  PakFileEntry *entries;
  char fileName[FILENAME_MAX];
  char *inputPath;
  char *outputFolder;
  unsigned int numEntries;
  unsigned int i;
  int len;
  FILE *outFile;
  FILE *file;
  char *buffer;
  int opt;

  /* Get command-line arguments */
  while ((opt = getopt(argc, argv, "i:o:")) != -1) {
    switch (opt) {
    case 'i':
      inputPath = optarg;
      break;
    case 'o':
      outputFolder = optarg;
      break;
    default:
      fprintf(stderr,
              "Usage: %s [-i ./inputpath/Monkey1.pak] [-o ./output_folder/]",
              argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if ((file = fopen(inputPath, "rb")) == NULL) {
    fprintf(stderr, "Could not open file: %s\n", inputPath);
    exit(EXIT_FAILURE);
  }
  /* Read header */
  fread(&header, sizeof(PakHeader), 1, file);
  /* Read file entries */
  numEntries = header.sizeOfFileEntries / sizeof(PakFileEntry);
  entries = malloc(header.sizeOfFileEntries);
  fseek(file, header.startOfFileEntries, SEEK_SET);
  fread(entries, header.sizeOfFileEntries, 1, file);
  /* Dump files */
  for (i = 0; i < numEntries; i++) {
    fseek(file, entries[i].fileNamePos + header.startOfFileNames, SEEK_SET);
    fgets(fileName, FILENAME_MAX, file);
    printf("Extracting %s...\n", fileName);
    char outputPath[FILENAME_MAX];
    char *scp;
    strncpy(outputPath, outputFolder, FILENAME_MAX);
    strncat(outputPath, fileName, FILENAME_MAX);
    scp = strdup(outputPath);
    _mkdir(dirname(scp));
    free(scp);
    if ((outFile = fopen(outputPath, "wb")) == NULL) {
      printf("Could not write to %s\n", outputPath);
      return -1;
    }
    fseek(file, entries[i].fileDataPos + header.startOfData, SEEK_SET);
    buffer = malloc(entries[i].dataSize);
    fread(buffer, entries[i].dataSize, 1, file);
    fwrite(buffer, entries[i].dataSize, 1, outFile);
    fclose(outFile);
    /* Store dxt files in dds container */
    len = strlen(outputPath);
    if (strcmp("dxt", &outputPath[len - 3]) == 0) {
      outputPath[len - 2] = 'd';
      outputPath[len - 1] = 's';
      printf("Writing %s...\n", outputPath);
      write_dds(outputPath, buffer, entries[i].dataSize);
    }
    free(buffer);
  }
  fclose(file);
  free(entries);
  return 0;
}
