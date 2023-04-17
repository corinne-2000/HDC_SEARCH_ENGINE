// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INCLUDES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "limits.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TYPE DEFINITIONS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef ap_axis<8, 0, 0, 0>   tPacket;
typedef hls::stream<tPacket>  tAxiStream;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// MODULE
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
unsigned int SearchEngine(
  tAxiStream &profile,
  volatile char * QueryVectorPtr,
  unsigned int VectorSize,
  long long * HighestValue
) {

// ********************* PRAGMAS *********************
// Unroll
#pragma PIPELINE unroll

// AXI Master
#pragma HLS INTERFACE m_axi port = QueryVectorPtr depth = 13 bundle = memory offset = slave

// AXI Stream
#pragma HLS INTERFACE axis port = profile

// AXI lite
#pragma HLS INTERFACE s_axilite port = VectorSize bundle = ctrl
#pragma HLS INTERFACE s_axilite port = HighestValue bundle = ctrl
#pragma HLS INTERFACE s_axilite port = return bundle = ctrl

// ***************************************************

  // Define local variables
  unsigned int lByteCount = 0;
  unsigned int lVectorCount = 0;
  unsigned int lVectorIndex = 0;
  long long lAccumulator = 0;
  tPacket lData;
  long long lHighestValue = LONG_MIN;

  while(1)
  {
    // Read data from stream and compute similarity check
    profile.read(lData);

    // XOR
    lAccumulator += lData.data ^ *(QueryVectorPtr + lByteCount);
    lByteCount++;

    // Save results in the memory when we reach the vector size
    if(lByteCount == VectorSize)
    {
      if(lHighestValue < lAccumulator)
      {
        lVectorIndex = lVectorCount;
        lHighestValue = lAccumulator;
      }
      lAccumulator = 0;
      lByteCount = 0;
      lVectorCount++;
    }
    if(lData.last)
    {
      *HighestValue = lHighestValue;
      break;
    }
  }

  return lVectorIndex;
}
