/*
AUTHOR: Avinash Srinivasan
Code File: restoringNullBytes.c
This program hides a filesystem ".dmg" within the
slack space of specified files.

The program takes as input a mapFile created by
'strippingNullBytes.c' and the .dmg file returned by
'retrievingNoNullImg.c'. It then replaces all the null bytes
that were stripped (which is noted in the mapFile) and returns
a dmg file that is (should be) an exact replica of the
original dmg file used by 'strippingNullBytes.c'
*/

// below macro def is necessary for asprintf
// has to appear before #include<stdio.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "slackFS.h"

int main(int argc, char const *argv[])
{
  // root folder of all project files
  // need to update the fixedPATH for your system
  char *fixedPATH = "../../";
  char srcFullPATH [128] = {0};
  char dstFullPATH [128] = {0};
  char mapPath [128] = {0};

  // relative paths to srcIMG, dstIMG, and mapFile from fixedPATH
  // file without NULL bytes that needs to be restored with
  // stripped off NULL bytes reinserted using mapFile entries
  char srcIMG[] = "imageFiles/myimg_noNull.img";

  // below file will be created with NULL bytes inserted
  // using mapFile entries. should have the same size/hash
  // of the srcIMG used in "slackFS_removingNULLBytes.c"
  char dstIMG[] = "imageFiles/myimg_Restored.img";

  // below file was created by "slackFS_removingNULLBytes.c"
  // with the start address and length of every NULL byte
  // sequence in the srcIMG. we will use this file to reinsert
  // the NULL bytes at correct address (byte offset) and length
  char mapFile[] = "mapFiles/mapFile_fat16FS_full200MB.txt";

  strncpy(srcFullPATH, fixedPATH, strlen(fixedPATH));
  strcat(srcFullPATH, srcIMG);

  strncpy(dstFullPATH, fixedPATH, strlen(fixedPATH));
  strcat(dstFullPATH, dstIMG);

  strncpy(mapPath, fixedPATH, strlen(fixedPATH));
  strcat(mapPath, mapFile);

  char oneChar;
  FILE *fpMap, *fpSRC, *fpDST;

  struct stat st;
  // computing size of the original filesystem ".dmg" file
  // you need to update this accordingly
  stat("../../imageFiles/myimg.img", &st);
  int statFileSize = st.st_size;

  // open the map file
  fpMap = fopen(mapPath, "r");
  if (fpMap==NULL)
  {
    ferror(fpMap);
    fclose(fpMap);
    exit(EXIT_FAILURE);
  }
  // image that needs to be restored to full size
  // by reinserting null-byte(s) referencing *fpMap
  fpSRC = fopen(srcFullPATH, "rb");
  if (fpSRC == NULL)
  {
    ferror(fpSRC);
    fclose(fpSRC);
    exit(EXIT_FAILURE);
  }

  // file to write the restored filesystem image
  fpDST = fopen(dstFullPATH, "wb");
  if (fpDST == NULL)
  {
    perror("fpDST");
    ferror(fpDST);
    fclose(fpDST);
    exit(EXIT_FAILURE);
  }

// variables to track start address and length
// null bytes sequence entries in mapFile
int nullADD=0, nullLEN=0;

// variables to track start address and length
// of data bytes in the srcIMG (.dmg without NULL bytes)
int dataADD=0, dataLEN=0;

while(1)
  {
    nullADD=0, nullLEN=0, dataLEN=0;
    if(!feof(fpMap))
    {
      fscanf(fpMap, "%d %d", &nullADD, &nullLEN);

      // checking NULL byte start address and then deciding how
      // many bytes to read from the srcIMG with no NULL
      dataLEN = abs( nullADD - dataADD );
      if(dataADD+dataLEN > statFileSize)
      {
        dataLEN = statFileSize - dataADD;
      }

      char *tempDataBuff = malloc(dataLEN * sizeof(char));
      fread(tempDataBuff, dataLEN, sizeof(char), fpSRC);
      if(!fseek(fpDST, dataADD, SEEK_SET))
        ferror(fpDST);

      if(!fwrite(tempDataBuff, dataLEN, sizeof(char), fpDST))
        ferror(fpDST);
      memset(tempDataBuff, 0x00, dataLEN);

      char *tempNullBuff = malloc(nullLEN * sizeof(char));
      memset(tempNullBuff, 0x00, nullLEN);
      if(!fseek(fpDST, nullADD, SEEK_SET))
        ferror(fpDST);
      if(!fwrite(tempNullBuff, nullLEN, sizeof(char), fpDST))
        ferror(fpDST);

      if(((dataADD+dataLEN) > statFileSize) || ((nullADD+nullLEN) > statFileSize) || (feof(fpMap)))
      {
        break;
      }
      // || (feof(fpMap))
      // else if((nullADD+nullLEN) > statFileSize)
      // {
      //   break;
      // }
      // else if(feof(fpMap))
      // {
      //   break;
      // }
      else
      {
        dataADD = (nullADD + nullLEN);
      }
    }

    // else if(feof(fpMap))
    // {
    //   // printf("inside else-if block");
    //   dataLEN = statFileSize - dataADD;
    //   // printf("dataLEN=%i\n", dataLEN);
    //   char *tempDataBuff = malloc(dataLEN * sizeof(char));
    //   fread(tempDataBuff, dataLEN, sizeof(char), fpSRC);
    //   if(!fseek(fpDST, dataADD, SEEK_SET))
    //     ferror(fpDST);
    //   // printf("dataADD(ftell)=%li \n", ftell(fpDST));
    //   if(!fwrite(tempDataBuff, dataLEN, sizeof(char), fpDST))
    //     ferror(fpDST);
    //   memset(tempDataBuff, 0x00, dataLEN);
    // }
  }

  fclose(fpMap);
  fclose(fpSRC);
  fclose(fpDST);

  return 0;
}
