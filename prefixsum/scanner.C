#include <stdio.h>
#include "scanner.decl.h"

/*readonly*/ CProxy_Main mainProxy;
/*readonly*/ CProxy_Scanner arrProxy;
/*readonly*/ int nChares;
/*readonly*/ int nData;
/*readonly*/ int nRounds;

/*mainchare*/
class Main : public CBase_Main
{
public:
  Main_SDAG_CODE int roundIter;
  int *data;

  Main(CkArgMsg *m)
  {
    if (m->argc != 2)
    {
      CkPrintf("Usage: ./scanner <nChares>\n");
      CkExit();
    }

    nChares = atoi(m->argv[1]);
    mainProxy = thisProxy;
    nRounds = ceil(log2(nChares));

    data = new int[nChares];
    roundIter = 0;

    srand(time(NULL));
    for (int i = 0; i < nChares; i++)
      data[i] = rand() % 100;

    CkArrayOptions opts;
    CkPrintf("Running Scanner on %d processors with %d chares\n", CkNumPes(), nChares);
    arrProxy = CProxy_Scanner::ckNew(nChares);
  };

  void ScanInRoundsReduction()
  {
    CkPrintf("Round %d\n", roundIter);
    arrProxy.scanSend(roundIter);
  }

  void ScanInRounds()
  {
    arrProxy.scanSend(roundIter);
  }

  void roundDone()
  {
    roundIter++;
    if (roundIter >= nRounds)
      thisProxy.done();
    else
      thisProxy.ScanInRounds();
  }

  void initDone()
  {
    for (int i = 0; i < nChares; i++)
    {
      arrProxy[i].setValue(data[i]);
    }
  }

  void done(void)
  {
    // compute true solution
    int *trueScan = new int[nChares];
    int runningTotal = 0;
    for (int i = 0; i < nChares; i++)
    {
      runningTotal += data[i];
      trueScan[i] = runningTotal;
    }

    CkPrintf("Main chare reports: \n[");
    for (int i = 0; i < nChares; i++)
    {
      CkPrintf(" %d", trueScan[i]);
    }
    CkPrintf("\n[");

    for (int i = 0; i < nChares; i++)
    {
      CkPrintf(" %d", data[i]);
    }
    CkPrintf("\n");

    for (int i = 0; i < nChares; i++)
    {
      arrProxy[i].verify(trueScan[i]);
    }
  };

  void exit()
  {
    CkPrintf("All results verified. Exiting...\n");
    CkExit();
  }
};

class Scanner : public CBase_Scanner
{
public:
  Scanner_SDAG_CODE int myScanValue;
  int myStartValue;

  int numUpdates;
  int lastRound;

  Scanner()
  {
    numUpdates = 0;
    lastRound = -1;
    CkCallback initCB(CkIndex_Main::initDone(), mainProxy);
    contribute(initCB);
  }

  Scanner(CkMigrateMessage *m) {}

  void setValue(int value)
  {
    myScanValue = value;
    myStartValue = value;

    if (thisIndex == 3)
    {
      CkPrintf("Chare %d: Value: %d\n", thisIndex, myScanValue);
    }

    CkCallback cb(CkIndex_Main::ScanInRoundsReduction(), mainProxy);
    contribute(cb);
  }

  void verify(int data)
  {
    if (myScanValue != data)
    {
      CkPrintf("ABORT!! Chare %d: Incorrect value: %d with %d updates\n", thisIndex, myScanValue, numUpdates);
      CkExit();
    }
    CkCallback cb(CkIndex_Main::exit(), mainProxy);
    contribute(cb);
  }
};

#include "scanner.def.h"
