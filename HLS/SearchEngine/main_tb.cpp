// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INCLUDES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "stdio.h"
#include "string.h"
#include "limits.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

// Add test queries
#ifndef __SYNTHESIS__
#include "../../Tests/vectors.h"
#else
#include "../../../../../../Tests/vectors.h"
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TYPE DEFINITIONS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef ap_axis<8, 0, 0, 0>   tPacket;
typedef hls::stream<tPacket>  tAxiStream;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DEFINES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define kPROFILE_VECTOR       vals
#define kQUERY_VECTOR         HVar0

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MODULE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned int SearchEngine(
  tAxiStream &profile,
  volatile char * QueryVectorPtr,
  unsigned int VectorSize,
  long long * HighestValue
);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TEST BENCH
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int main()
{
  // Define local variables
  unsigned int lIndex;
  unsigned int lByte = 0;
  unsigned int lVector = 0;
  long long lAccumulator;
  unsigned int lResult = 0;
  unsigned int lGoldenIndex = 0;
  long long lGoldenValue = LONG_MIN;
  unsigned int lCalculatedIndex = 0;
  long long lCalculatedValue = LONG_MIN;
  tPacket lPacket;
  tAxiStream lStream;

  printf("INFO: Vector count is %u\n", (sizeof(kPROFILE_VECTOR) / sizeof(kQUERY_VECTOR)));

  // Calculate the results
  for(lIndex = 0; lIndex < sizeof(kPROFILE_VECTOR); lIndex++)
  {
    // Create a stream
    lPacket.data = kPROFILE_VECTOR[lIndex];
    lPacket.last = (lIndex == (sizeof(kPROFILE_VECTOR) - 1));
    lStream.write(lPacket);

    // Compute expected result
    lAccumulator += (kPROFILE_VECTOR[lIndex] ^ kQUERY_VECTOR[lByte]);
    lByte++;

    if(lByte == kVECTOR_SIZE)
    {
      printf("INFO: Vector #%u = %d\n", lVector, lAccumulator);
      if(lGoldenValue < lAccumulator)
      {
        lGoldenIndex = lVector;
        lGoldenValue = lAccumulator;
      }
      lAccumulator = 0;
      lByte = 0;
      lVector++;
    }
  }

  // Call the model
  lCalculatedIndex = SearchEngine(
    lStream,
    (volatile char *)&kQUERY_VECTOR,
    kVECTOR_SIZE,
    &lCalculatedValue
  );

  // Validate results by comparing output value with expected one
  lResult |= (lCalculatedIndex != lGoldenIndex);
  lResult |= (lCalculatedValue != lGoldenValue);

  printf("%s: Highest vector index #%u | value: %d (expected: #%u | value: %d)\n",
    (!lResult ? "INFO" : "ERROR"),
    lCalculatedIndex,
    lCalculatedValue,
    lGoldenIndex,
    lGoldenValue);

  if(lResult)
    printf("ERROR: Test bench returned error code\n");
  else
    printf("INFO: Test completed successfully\n");

#ifndef __SYNTHESIS__
  printf("INFO: Writing output data to the file\n");
  FILE * lFile = fopen("../../../../../../Tests/output.h", "w+");
  fprintf(lFile, "const unsigned int gExpectedIndex = %u;\n", lCalculatedIndex);
  fprintf(lFile, "const long long gExpectedValue = %d;", lCalculatedValue);
  fclose(lFile);
#endif

  return lResult;
}
