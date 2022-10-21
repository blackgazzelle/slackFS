/*
AUTHOR: Avinash Srinivasan
Code File: retrievingNoNullImg.c
This is a program retrieves the null-free .dmg
file hidden by 'hidingNoNullImg.c' from the slack
space of files specified with a coverfile list.

The program takes as input the coverfile list(.txt file)
used by 'hidingNoNullImg.c' with the available slack space
for each coverfile used.
It reassembles the contents of all the cover files OR upto a
specified number of bytes and returns a .dmg file saved to
dstFullPATH.
*/

// below macro def is necessary for asprintf
// has to appear before #include<stdio.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include "slackFS.h"

int main(int argc, char const *argv[])
{
  char fixedBasePATH [] = "../../";
  char dstFullPATH [128] = {0};

  char coverFilePATH [128] = {0};
  char bmapPATH [128] = "sudo ../../bmap/bmap --mode slack ";

  char dstIMG[] = "imageFiles/myimg_Retrieved.img";

  strncpy(dstFullPATH, fixedBasePATH, strlen(fixedBasePATH));
  strcat(dstFullPATH, dstIMG);

  char coverFileLIST[] = "textFiles/usrCoverFileList.txt";
  strncpy(coverFilePATH, fixedBasePATH, strlen(fixedBasePATH));
  strcat(coverFilePATH, coverFileLIST);

  int sysRet;

  FILE *fp_dstIMG, *fp_coverFileList;

  struct stat st;
  // provide the correct path to the original file with null bytes
  stat("../../imageFiles/myimg_noNull.img", &st);
  int statFileSize = st.st_size;

  fp_dstIMG = fopen(dstFullPATH, "wb");
  if (fp_dstIMG == NULL)
  {
    printf("Error message: %s\n", strerror(errno));
    perror("Message from perror");
    fclose(fp_dstIMG);
    exit(EXIT_FAILURE);
  }

  fp_coverFileList = fopen(coverFilePATH, "r");
  if (fp_coverFileList == NULL)
  {
    printf("Error message: %s\n", strerror(errno));
    perror("fopen (srcIMG)");
    fclose(fp_coverFileList);
    exit(EXIT_FAILURE);
  }

  int bytesRestored=0, bytesRemaining = statFileSize, coverFileCounter=0;
  while(!feof(fp_dstIMG) || !feof(fp_coverFileList))
  {
    if(bytesRestored >= statFileSize)
      break;
    coverFileCounter++;
    char finalCMD[1024]={0};
    char coverFileName[128]={0};

    int availableSlack=0;

    char fixedCMDL[128] = "sudo ../../bmap/bmap --mode slack ";
    char leftCMD[256]= {0};
    strncpy(leftCMD, fixedCMDL, strlen(fixedCMDL));

    char fixedCMDR[128] = " | dd of=";
    strcat(fixedCMDR, dstFullPATH);
    strcat(fixedCMDR, " oflag=seek_bytes count=1 bs=");
    char rightCMD[256]= {0};
    strncpy(rightCMD, fixedCMDR, strlen(fixedCMDR));

    fscanf(fp_coverFileList, "%s %i", coverFileName, &availableSlack);

    struct stat st1;

    if( stat(coverFileName, &st1) == -1)
    {
      if( S_ISREG( st1.st_mode ) != 0 )
      {
        printf("Error message inside: %s\n", strerror(errno));
        printf("Check for S_ISREG failed for %s. Skipping this file.\n", coverFileName);
        coverFileCounter--;
        continue;
      }
      printf("Error message outside: %s\n", strerror(errno));
    }

    strcat(leftCMD, coverFileName);
    strcat(leftCMD, " ");
    char *seekSize, *retrieveSize;
    if(bytesRemaining <= availableSlack)
    {
      asprintf(&seekSize, "%d", bytesRestored);
      asprintf(&retrieveSize, "%d", bytesRemaining);
      bytesRestored = bytesRestored + bytesRemaining;
      bytesRemaining = 0;
    }
    else
    {
      asprintf(&seekSize, "%d", bytesRestored);
      asprintf(&retrieveSize, "%d", availableSlack);
      bytesRestored = bytesRestored + availableSlack;
      bytesRemaining = bytesRemaining - availableSlack;
    }
    strcat(leftCMD, retrieveSize);
    strcat(rightCMD, retrieveSize);
    strcat(rightCMD, " seek=");
    strcat(rightCMD, seekSize);

    strcpy(finalCMD, leftCMD);
    strcat(finalCMD, rightCMD);

    sysRet = system(finalCMD);
    if (sysRet == -1 && WIFEXITED(sysRet))
      printf("Error: Terminated with status %d\n", WEXITSTATUS(sysRet));

    strncpy(leftCMD, "\0", 256);
    strncpy(rightCMD, "\0", 256);
    strncpy(finalCMD, "0", 1024);
    strncpy(fixedCMDL, "\0", 128);
    strncpy(fixedCMDR, "0", 128);
  }
  fclose(fp_dstIMG);
  fclose(fp_coverFileList);

  return 0;
}
