#include <stdio.h>
#include "xparameters.h"
#include "xaxidma.h"
#include "xil_printf.h"
#include "xsearchengine.h"
#include "xtmrctr.h"
#include "xscugic.h"
#include "xil_cache.h"

//#include "../../../Tests/vectors.h"
#include "../../../Tests/H_size751.h"
#include "../../../Tests/output.h"

// ~~~~~~~~~~~~~~~~~~~~~~~ DEFINES ~~~~~~~~~~~~~~~~~~~~~~~

// End of line
#define kEOL                      "\n\r"

// AXI DMA reset timeout
#define kDMA_RESET_TIMEOUT        10000

// SearchEngine and DMA execution timeout
#define kSEARCH_ENGINE_TIMEOUT    10000000
#define kDMA_TIMEOUT              10000000

// Selection of vectors
#define kQVECTOR                  &HVar0
#define kPVECTOR                  &vals
#define kPVECTOR_SIZE             (kVECTOR_COUNT * kVECTOR_SIZE)

// Value of each timer in microseconds
#define kTIMER_TICK_US            ((double)1000000 / (double)XPAR_TIMER_CLOCK_FREQ_HZ)

// Timers
#define kSE_STOP_TIMER_IDX        1
#define kSE_START_TIMER_IDX       0

// Testbench configuration
#define kCFG_NUMBER_OF_CYCLES     10
#define kCFG_CORE_COUNT           1

// ~~~~~~~~~~~~~~~~~~ GLOBAL STRUCTURES ~~~~~~~~~~~~~~~~~~

#ifndef __INIT_STRUCTURE__
#define __INIT_STRUCTURE__

// Define the size of pointer
typedef unsigned long long tPointer;

typedef struct
{
  XAxiDma             eAxiDma;
  unsigned int        eAxiDmaDeviceId;

  XSearchengine       eSearchEngine;
  unsigned int        eSearchEngineDeviceId;

  XTmrCtr             eTimer;
  unsigned int        eTimerDeviceId;

  char              * eProfileVectors;
  char              * eQueryVector;
  unsigned int        eVectorSize;
  unsigned int        eProfileVectorCount;

  unsigned char       eCompareResults             : 1;
  unsigned char       RSVD                        : 7;

  long long           eVectorValue;
  long long           eExpectedValue;
  unsigned int        eVectorIndex;
  unsigned int        eExpectedIndex;

  unsigned int        eCycleCounter;
} sCore;

#endif

// ~~~~~~~~~~~~~~~~~~~ GLOBAL VARIABLES ~~~~~~~~~~~~~~~~~~

// Create buffers for vectors
char gQueryVector[sizeof(*(kQVECTOR))];
char gProfileVectors[kPVECTOR_SIZE];

// Peripherals
sCore                 gCore[kCFG_CORE_COUNT];

// ~~~~~~~~~~~~~~~~~ FUNCTION PROTOTYPES ~~~~~~~~~~~~~~~~~

unsigned int fCompleteSimilarityCheck(sCore * pCore);
unsigned int fStartSimilarityCheck(sCore * pCore);

// ~~~~~~~~~~~~~~~~~~~~~~ FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~

// Intialize each core (SearchEngine + DMA + Timer(optional))
unsigned int fInitializeCore(sCore * pCore)
{
  xil_printf("INFO Locate DMA configuration...");
  XAxiDma_Config * lDmaConfig = XAxiDma_LookupConfig(pCore->eAxiDmaDeviceId);
  if(lDmaConfig == NULL)
  {
    xil_printf("FAILED"kEOL);
    return 1;
  }
  else
    xil_printf("SUCCESS"kEOL);

  xil_printf("INFO Initialize DMA...");
  if(XAxiDma_CfgInitialize(&pCore->eAxiDma, lDmaConfig) != XST_SUCCESS)
  {
    xil_printf("FAILED"kEOL);
    return 1;
  }
  else
    xil_printf("SUCCESS"kEOL);

  xil_printf("INFO Issue DMA reset...");
  XAxiDma_Reset(&pCore->eAxiDma);
  unsigned int lTimeOut = kDMA_RESET_TIMEOUT;
  while(lTimeOut)
  {
    if(XAxiDma_ResetIsDone(&pCore->eAxiDma))
      break;
    lTimeOut--;
  }
  xil_printf("OK"kEOL);

  xil_printf("INFO Enable DMA interrupt...");
  XAxiDma_IntrEnable(&pCore->eAxiDma, XAXIDMA_IRQ_IOC_MASK, XAXIDMA_DMA_TO_DEVICE);
  xil_printf("OK"kEOL);

  // Initialize Search Engine
  xil_printf("INFO Initialize SearchEngine...");
  if(XSearchengine_Initialize(&pCore->eSearchEngine, pCore->eSearchEngineDeviceId) != XST_SUCCESS)
  {
    xil_printf("FAILED"kEOL);
    return 1;
  }
  else
    xil_printf("SUCCESS"kEOL);

  xil_printf("INFO Enable SearchEngine interrupts...");
  XSearchengine_InterruptGlobalEnable(&pCore->eSearchEngine);
  XSearchengine_InterruptEnable(&pCore->eSearchEngine, 1/*ap_done*/);
  xil_printf("OK"kEOL);

  if(&pCore->eTimer != NULL)
  {
    // Initialize timer
    xil_printf("INFO Initialize AXI timer...");
    if(XTmrCtr_Initialize(&pCore->eTimer, pCore->eTimerDeviceId) != XST_SUCCESS)
    {
      xil_printf("FAILED"kEOL);
      return 1;
    }
    else
      xil_printf("SUCCESS"kEOL);

    xil_printf("INFO Perform self-test of timer 1...");
    if(XTmrCtr_SelfTest(&pCore->eTimer, kSE_START_TIMER_IDX) != XST_SUCCESS)
    {
      xil_printf("FAILED"kEOL);
      return 1;
    }
    else
      xil_printf("SUCCESS"kEOL);

    xil_printf("INFO Perform self-test of timer 2...");
    if(XTmrCtr_SelfTest(&pCore->eTimer, kSE_STOP_TIMER_IDX) != XST_SUCCESS)
    {
      xil_printf("FAILED"kEOL);
      return 1;
    }
    else
      xil_printf("SUCCESS"kEOL);

    xil_printf("INFO Stop timers...");
    XTmrCtr_Stop(&pCore->eTimer, kSE_START_TIMER_IDX);
    XTmrCtr_Stop(&pCore->eTimer, kSE_STOP_TIMER_IDX);
    xil_printf("OK"kEOL);

    xil_printf("INFO Set timers reset value to 0...");
    XTmrCtr_SetResetValue(&pCore->eTimer, kSE_START_TIMER_IDX, 0);
    XTmrCtr_SetResetValue(&pCore->eTimer, kSE_STOP_TIMER_IDX, 0);
    xil_printf("OK"kEOL);

    xil_printf("INFO Reset timers...");
    XTmrCtr_Reset(&pCore->eTimer, kSE_START_TIMER_IDX);
    XTmrCtr_Reset(&pCore->eTimer, kSE_STOP_TIMER_IDX);
    xil_printf("OK"kEOL);

    xil_printf("INFO Set timer options...");
    XTmrCtr_SetOptions(&pCore->eTimer, kSE_START_TIMER_IDX, XTC_CAPTURE_MODE_OPTION);
    XTmrCtr_SetOptions(&pCore->eTimer, kSE_STOP_TIMER_IDX, XTC_CAPTURE_MODE_OPTION);
    xil_printf("OK"kEOL);
  }

  if(XSearchengine_IsReady(&pCore->eSearchEngine))
    xil_printf("INFO Search Engine is ready"kEOL);
  else
  {
    xil_printf("WARNING Search Engine is not ready"kEOL);
    return 1;
  }

  xil_printf("INFO Program query vector pointer...");
  XSearchengine_Set_QueryVectorPtr(&pCore->eSearchEngine, (unsigned int)(unsigned long long)pCore->eQueryVector);
  if(XSearchengine_Get_QueryVectorPtr(&pCore->eSearchEngine) == (unsigned int)(unsigned long long)pCore->eQueryVector)
    xil_printf("OK"kEOL);
  else
  {
    xil_printf("FAILED"kEOL);
    return 1;
  }

  xil_printf("INFO Program vector size...");
  XSearchengine_Set_VectorSize(&pCore->eSearchEngine, pCore->eVectorSize);
  if(XSearchengine_Get_VectorSize(&pCore->eSearchEngine) == pCore->eVectorSize)
    xil_printf("OK"kEOL);
  else
  {
    xil_printf("FAILED"kEOL);
    return 1;
  }

  pCore->eCycleCounter = 0;
  return 0;
}

// Start SearchEngine, attached AXI DMA and timer
unsigned int fStartSimilarityCheck(sCore * pCore)
{
  XSearchengine_Start(&pCore->eSearchEngine);

  if(pCore->eTimer.IsReady == XIL_COMPONENT_IS_READY)
  {
    XTmrCtr_Reset(&pCore->eTimer, kSE_START_TIMER_IDX);
    XTmrCtr_Reset(&pCore->eTimer, kSE_STOP_TIMER_IDX);
    XTmrCtr_Start(&pCore->eTimer, kSE_START_TIMER_IDX);
    XTmrCtr_Start(&pCore->eTimer, kSE_STOP_TIMER_IDX);
  }

  // Start DMA transfer
  if(XAxiDma_SimpleTransfer(
    &pCore->eAxiDma,                                    // DMA instance
    (unsigned long long)pCore->eProfileVectors,         // Source
    (pCore->eProfileVectorCount * pCore->eVectorSize),  // Size
    XAXIDMA_DMA_TO_DEVICE                               // Direction
  ) == XST_SUCCESS)
  {
    return 0;
  }
  else
    xil_printf("WARNING Failed to launch DMA to device transfer"kEOL);

  return 1;
}

// Get results and validate it
unsigned int fCompleteSimilarityCheck(sCore * pCore)
{
  if(pCore->eTimer.IsReady == XIL_COMPONENT_IS_READY)
  {
    // Stop timers
    XTmrCtr_Stop(&pCore->eTimer, kSE_START_TIMER_IDX);
    XTmrCtr_Stop(&pCore->eTimer, kSE_STOP_TIMER_IDX);
    
    unsigned int lTimerValue = XTmrCtr_GetCaptureValue(&pCore->eTimer, kSE_STOP_TIMER_IDX) - XTmrCtr_GetCaptureValue(&pCore->eTimer, kSE_START_TIMER_IDX);
    double lProcessedTimeUs = ((double)lTimerValue * kTIMER_TICK_US);
    printf("INFO SearchEngine processed %u B in %1.2f us"kEOL, (pCore->eVectorSize * pCore->eProfileVectorCount), lProcessedTimeUs);
  }

  pCore->eVectorIndex = XSearchengine_Get_return(&pCore->eSearchEngine);
  pCore->eVectorValue = (long long)XSearchengine_Get_HighestValue(&pCore->eSearchEngine);

  if(pCore->eCompareResults)
  {
#if kCFG_NUMBER_OF_CYCLES == 1
    printf("INFO Validate result...");
#endif
    if(pCore->eVectorIndex != pCore->eExpectedIndex || pCore->eVectorValue != pCore->eExpectedValue)
    {
      printf("ERROR"kEOL);
      printf("INFO: Highest vector index #%u | value: %lld (expected: #%u | value: %lld)\n",
        pCore->eVectorIndex,
        pCore->eVectorValue,
        pCore->eExpectedIndex,
        pCore->eExpectedValue);
      return 1;
    }
#if kCFG_NUMBER_OF_CYCLES == 1
    else
      printf("SUCCESS"kEOL"INFO Highest vector index #%u | value: %lld"kEOL, pCore->eVectorIndex, pCore->eVectorValue);
#endif
  }

  return 0;
}

// ~~~~~~~~~~~~~~~~~~~~~ ENTRY POINT ~~~~~~~~~~~~~~~~~~~~~

int main()
{
  // Define local variable
  unsigned int lCoreIndex;

  // Welcome message (UART test)
  xil_printf("INFO Running on A53 core"kEOL);

  // Copy vectors to the memory location
  xil_printf("INFO Copy query vector to the memory...");
  memcpy(&gQueryVector, kQVECTOR, kVECTOR_SIZE);
  xil_printf("OK"kEOL);

  xil_printf("INFO Copy profile vectors to the memory...");
  memcpy(&gProfileVectors, kPVECTOR, kPVECTOR_SIZE);
  xil_printf("OK"kEOL);

  Xil_DCacheInvalidate();

  xil_printf("INFO Profile vector count: %u"kEOL, kVECTOR_COUNT);

  // Initialize cores structure
  memset(&gCore, 0, sizeof(gCore));

  // Initialize first core, which is combination of SearchEngine IP + DMA + Timer(optional)
  gCore[0].eAxiDmaDeviceId              = XPAR_DMA_DEVICE_ID;
  gCore[0].eSearchEngineDeviceId        = XPAR_SEARCH_ENGINE_DEVICE_ID;
  gCore[0].eTimerDeviceId               = XPAR_TIMER_DEVICE_ID;
  gCore[0].eProfileVectors              = gProfileVectors;
  gCore[0].eQueryVector                 = gQueryVector;
  gCore[0].eVectorSize                  = kVECTOR_SIZE;
  gCore[0].eProfileVectorCount          = kVECTOR_COUNT;
  gCore[0].eCompareResults              = 0;                      // Do not compare results with expected ones
  gCore[0].eExpectedIndex               = gExpectedIndex;         // Expected index generated by HLS testbench
  gCore[0].eExpectedValue               = gExpectedValue;         // Expected value generated by HLS testbench

  // Initialize each core and start transfer
  for (lCoreIndex = 0; lCoreIndex < kCFG_CORE_COUNT; lCoreIndex++)
  {
    // Initialize core peripherals
    fInitializeCore(&gCore[lCoreIndex]);

    xil_printf("INFO Running the similarity check %u time(s)"kEOL, kCFG_NUMBER_OF_CYCLES);
    for(gCore[lCoreIndex].eCycleCounter = 0; gCore[lCoreIndex].eCycleCounter < kCFG_NUMBER_OF_CYCLES; gCore[lCoreIndex].eCycleCounter++)
    {
      // Start DMA transfer
      if(fStartSimilarityCheck(&gCore[lCoreIndex]) != 0)
      {
        Xil_ExceptionDisable();
        return 0;
      }

#if kCFG_NUMBER_OF_CYCLES == 1
      unsigned int lTimeout = kSEARCH_ENGINE_TIMEOUT;
      xil_printf("INFO Waiting SearchEngine to complete...");
      while(!XSearchengine_IsDone(&gCore[lCoreIndex].eSearchEngine) && lTimeout > 0) { lTimeout--; };
      if(lTimeout == 0)
        xil_printf("FAILED"kEOL);
      else
        xil_printf("OK"kEOL);
#else
      while(!XSearchengine_IsDone(&gCore[lCoreIndex].eSearchEngine));
#endif

      if(fCompleteSimilarityCheck(&gCore[lCoreIndex]) != 0) break;
    }
  }

  return 0;
}
