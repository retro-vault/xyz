/** bug-3873.c: ??
 */
 
#include <testfwk.h>

typedef unsigned char   uint8;
typedef unsigned short  uint16;

typedef struct HuffTableT
{
   uint16 mMinCode[16];
   uint16 mMaxCode[16];
   uint8 mValPtr[16];
} HuffTable;

static HuffTable gHuffTab0;
static uint8 gBits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static void huffCreate(const uint8* pBits, HuffTable* pHuffTable)
{
   uint8 i = 0;
   uint8 j = 0;

   uint16 code = 0;

   for ( ; ; )
   {
      uint8 num = pBits[i];

      if (!num)
      {
         pHuffTable->mMinCode[i] = 0x0000;
         pHuffTable->mMaxCode[i] = 0xFFFF;
         pHuffTable->mValPtr[i] = 0;
      }
      else
      {
         pHuffTable->mMinCode[i] = code;
         pHuffTable->mMaxCode[i] = code + num - 1;
         pHuffTable->mValPtr[i] = j;
         j = (uint8)(j + num);

         code = (uint16)(code + num);
      }

      code <<= 1;

      i++;
      if (i > 15)
         break;
   }
}

void testBug(void) {
#ifndef __SDCC_mcs51 // Bug #3875
    huffCreate(gBits, &gHuffTab0);

    ASSERT(gHuffTab0.mMinCode[0] == 0x0000);
    ASSERT(gHuffTab0.mMaxCode[0] == 0xffff);
    ASSERT(gHuffTab0.mValPtr[0] == 0x00);

    ASSERT(gHuffTab0.mMinCode[1] == 0x0000);
    ASSERT(gHuffTab0.mMaxCode[1] == 0x0000);
    ASSERT(gHuffTab0.mValPtr[1] == 0x00);

    ASSERT(gHuffTab0.mMinCode[2] == 0x0002);
    ASSERT(gHuffTab0.mMaxCode[2] == 0x0003);
    ASSERT(gHuffTab0.mValPtr[2] == 0x01);

    ASSERT(gHuffTab0.mMinCode[15] == 0xffe0);
    ASSERT(gHuffTab0.mMaxCode[15] == 0xffee);
    ASSERT(gHuffTab0.mValPtr[15] == 0x69);
#endif
}

