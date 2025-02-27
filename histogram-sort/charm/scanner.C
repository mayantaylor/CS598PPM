#include <stdio.h>
#include "scanner.decl.h"
#include <queue>

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_Scanner arrProxy;
/*readonly*/ int nChares;
/*readonly*/ int nData;
/*readonly*/ int nRounds;

int MAX_DATA = 100;
// int NBUCKETS = 2;
int NROUNDS = 1; // number of rounds for radix sort

/*mainchare*/
class Main : public CBase_Main
{
public:
  Main_SDAG_CODE int roundIter;
  int *data;
  int histRound;

  Main(CkArgMsg *m)
  {
    if (m->argc != 3)
    {
      CkPrintf("Usage: ./scanner <nChares> <nData>\n");
      CkExit();
    }

    nChares = atoi(m->argv[1]);
    nData = atoi(m->argv[2]);
    mainProxy = thisProxy;
    nRounds = ceil(log2(nChares));

    roundIter = 0;
    histRound = 0;

    CkArrayOptions opts;
    arrProxy = CProxy_Scanner::ckNew(nData, nChares);
  };

  void ScanInRounds()
  {
    arrProxy.scanSend(roundIter);
  }

  void roundDone()
  {
    roundIter++;
    if (roundIter >= nRounds)
      thisProxy.broadcastGlobal();
    else
      thisProxy.ScanInRounds();
  }

  void startHistRound()
  {
    roundIter = 0; // reset round for scanning

    if (histRound == NROUNDS)
      arrProxy.verify();
    else
      arrProxy.buildHist(MAX_DATA, histRound);

    histRound++; // increment round for radix sort
  }

  void startSends()
  {
    arrProxy.sendItems();
  }

  void broadcastGlobal(void)
  {
    arrProxy[nChares - 1].sendGlobal();
  };

  void exit()
  {
    CkPrintf("All results verified. Exiting...\n");
    CkExit();
  }
};

class Scanner : public CBase_Scanner
{
private:
  std::queue<int> *myHistogram;

public:
  // scanner things
  Scanner_SDAG_CODE int *myScanValues;

  // data to sort
  int *data;
  int countPerProc;

  // histogram things
  int *hist;
  int nBuckets;

  int *sortedData;

  int receivedCount;
  int round;

  int totalData;

  int *globalHistScan;

  Scanner(int dS)
  {
    totalData = dS * nChares;
    countPerProc = dS;
    data = new int[countPerProc];

    // generate random data on each sorter, between 0 and MAX_DATA (non inclusive)
    srand(time(NULL));
    for (int i = 0; i < countPerProc; i++)
    {
      data[i] = rand() % MAX_DATA;
    }

    // start histogram radix sort
    CkCallback initCB(CkIndex_Main::startHistRound(), mainProxy);
    contribute(initCB);
  }

  /* Inline helper function to compute bucket given the round (for radix sort) */
  int computeBucket(int data, int round)
  {
    // int mask = 1 << round;
    // return (data & mask) >> round;
    return data % nBuckets;
  }

  /* Called at the beginning of each radix sort round. Build histogram and other components.*/
  void buildHist(int nB, int globalRound)
  {
    round = globalRound;

    if (round == 0)
    {
      nBuckets = nB;
      hist = new int[nBuckets];
    }

    else
    {
      // cleanup data from previous round
      delete[] data;
      delete[] myHistogram;
      delete[] myScanValues;
      delete[] globalHistScan;

      // set up data for this round
      data = sortedData;
    }

    // initialize histogram
    for (int i = 0; i < nBuckets; i++)
      hist[i] = 0;

    for (int i = 0; i < countPerProc; i++)
    {
      int d = data[i];
      int bucket = computeBucket(d, round);
      hist[bucket]++;
    }

    // set up histogram queues (used to track data within histogram buckets)
    myHistogram = new std::queue<int>[nBuckets];
    for (int i = 0; i < nBuckets; i++)
      myHistogram[i] = std::queue<int>();

    for (int i = 0; i < countPerProc; i++)
    {
      int d = data[i];
      int bucket = computeBucket(d, round);
      myHistogram[bucket].push(d);
    }

    // initialize values required for scanning (prefix sum)
    myScanValues = new int[nBuckets];
    for (int i = 0; i < nBuckets; i++)
      myScanValues[i] = hist[i];

    CkCallback cbSet(CkReductionTarget(Main, ScanInRounds), mainProxy);
    contribute(cbSet);
  }

  /* On receiving the global sums for each bucket (globalHist), compute information necessary for sending.*/
  void getGlobalHist(int *ghist, int size)
  {
    int *globalHist = ghist;
    int totalCount = std::accumulate(globalHist, globalHist + size, 0);
    countPerProc = ceil(totalCount / nChares);

    globalHistScan = new int[size];

    // compute scan of globalHist (exclusive)
    int runningTotal = 0;
    for (int i = 0; i < size; i++)
    {
      globalHistScan[i] = runningTotal;
      runningTotal += globalHist[i];
    }

    if (totalCount != totalData)
    {
      CkPrintf("ERROR: Total count %d does not match total data %d on proc %d, with %d buckets\n", totalCount, totalData, thisIndex, size);
      CkExit();
    }

    // allocate space for my elements
    sortedData = new int[countPerProc];
    receivedCount = 0;

    // begin sending
    CkCallback cb(CkReductionTarget(Main, startSends), mainProxy);
    contribute(cb);
  }

  void sendItems()
  {
    int totalSent = 0;
    // for each bucket, compute destination for my data
    for (int b = 0; b < nBuckets; b++)
    {
      int myub = globalHistScan[b] + myScanValues[b]; // gives my global index in the total order, for elements in this bucket
      int mylb = myub - hist[b];                      // gives my global index in the total order, for elements in this bucket

      assert(myHistogram[b].size() == hist[b]);
      assert(hist[b] == myub - mylb);
      for (int itemIdx = mylb; itemIdx < myub; itemIdx++)
      {
        // compute destination processor
        int destproc = itemIdx / countPerProc;

        assert(myHistogram[b].size() > 0);
        int item = myHistogram[b].front();
        myHistogram[b].pop();

        assert(item < MAX_DATA);
        if (destproc == thisIndex)
        {
          receivedCount++;
          sortedData[itemIdx % countPerProc] = item;
          totalSent++;
        }
        else
        {
          thisProxy[destproc].recvItem(item, itemIdx);
          totalSent++;
        }
      }
    }

    assert(totalSent == countPerProc);

    if (receivedCount == countPerProc)
    {
      // will not receive more
      CkCallback cb(CkReductionTarget(Main, startHistRound), mainProxy);
      contribute(cb);
    }
  }

  void recvItem(int item, int index)
  {
    sortedData[index % countPerProc] = item;
    receivedCount++;

    if (receivedCount > countPerProc)
    {
      CkPrintf("ERROR: Chare %d: Received count %d exceeds count per processor %d\n", thisIndex, receivedCount, countPerProc);
      CkExit();
    }
    assert(receivedCount <= countPerProc);
    if (receivedCount == countPerProc)
    {
      if (round == NROUNDS - 1)
      {
        // check if sortedElements is sorted
        bool sorted = true;
        for (int i = 1; i < countPerProc; i++)
        {
          if (sortedData[i] < sortedData[i - 1])
          {
            sorted = false;
            break;
          }
        }

        if (!sorted)
        {
          CkPrintf("ERROR: Chare %d: Elements not sorted!\n", thisIndex);
          CkExit();
        }
      }
      CkCallback cb(CkReductionTarget(Main, startHistRound), mainProxy);
      contribute(cb);
    }
  }

  void
  sendGlobal()
  {
    if (thisIndex == nChares - 1)
    {
      int mytotal = std::accumulate(myScanValues, myScanValues + nBuckets, 0);
      assert(mytotal = totalData);
      thisProxy.getGlobalHist(myScanValues, nBuckets);
    }
  }

  void verify()
  {
    if (thisIndex == nChares - 1)
    {
      // I'm last proc, don't send for verification
      CkCallback cb(CkReductionTarget(Main, exit), mainProxy);
    }
    else
    {
      // send to next proc
      thisProxy[thisIndex + 1].verifyFromLeft(sortedData[countPerProc - 1]);

      if (thisIndex == 0)
      {
        // I'm first proc, don't receive for verification

        CkCallback cb(CkReductionTarget(Main, exit), mainProxy);
        contribute(cb);
      }
    }
  }

  void verifyFromLeft(int data)
  {
    if (data > sortedData[0])
    {
      CkPrintf("ERROR: Chare %d: Data %d from left is greater than min element %d\n", thisIndex, data, sortedData[0]);
      CkExit();
    }
    else
    {
      CkCallback cb(CkReductionTarget(Main, exit), mainProxy);
      contribute(cb);
    }
  }
};

#include "scanner.def.h"
