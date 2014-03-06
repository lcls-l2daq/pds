#include <stdio.h>
#include <vector>

class HistReport
{
public:
  HistReport(float fFrom, float fTo, float fInterval):_fFrom(fFrom),
    _fTo(fTo), _fInterval(fInterval)
  {
    reset();

    if (_fFrom == (int) _fFrom && _fTo == (int) _fTo && _fInterval == (int) _fInterval)
      _bIntRange = true;
    else
      _bIntRange = false;
  }

  void reset()
  {
    _fMax         = -1e99;
    _fMin         = 1e99;
    _fAvg         = 0;
    _fAvgInRange  = 0;
    // assert( _fFrom < _fTo && _fInterval > 0 );
    int iSizeHist = 1 + (int) ((_fTo - _fFrom) / _fInterval);

    _lCounts.resize(iSizeHist + 2, 0);
    _lCounts.assign(iSizeHist + 2, 0);  // see the comment of _lCounts below
    _iTotalCounts = 0;
  }

  void addValue(double fValue)
  {
    int iIndex = (int) ((fValue - _fFrom) / _fInterval + 1e-4);

    if (iIndex < 0)
      iIndex = (int) _lCounts.size() - 2;
    else if (iIndex >= (int) _lCounts.size() - 2)
      iIndex = (int) _lCounts.size() - 1;

    // assert( _fFrom < _fTo && _fInterval > 0 );
    ++(_lCounts[iIndex]);
    ++_iTotalCounts;

    if (fValue > _fMax)
      _fMax = fValue;
    if (fValue < _fMin)
      _fMin = fValue;
    _fAvg +=  (fValue - _fAvg) / _iTotalCounts;

    if (iIndex < (int) _lCounts.size() - 2)
    {
      int iCountsInRange = _iTotalCounts - _lCounts[_lCounts.size() - 1] - _lCounts[_lCounts.size() - 2];
      if ( iCountsInRange > 0 )
      _fAvgInRange +=  (fValue - _fAvgInRange) / iCountsInRange;
    }
  }

  void report(const char *sTitle)
  {
    printf("# Histogram -- %s -- Count: %d\n", sTitle, _iTotalCounts);
    printf("#   avg(in range) %-4g avg(all) %-4g min %-4g max %-4g\n",
      _fAvgInRange, _fAvg, _fMin, _fMax);

    if (_lCounts[_lCounts.size() - 2] != 0)
      printf("#    ( -Inf , %-4g ) : %7d (%4.1f%%)\n", _fFrom,
       _lCounts[_lCounts.size() - 2],
       _lCounts[_lCounts.size() - 2] * 100.0 / _iTotalCounts);


    for (unsigned iIndex = 0; iIndex < _lCounts.size() - 2; ++iIndex)
    {
      if (_lCounts[iIndex] != 0) {
        if (_bIntRange)
          printf("#             %4d   : %7d (%4.1f%%)\n",
           (int) (_fFrom + iIndex * _fInterval), _lCounts[iIndex],
           _lCounts[iIndex] * 100.0 / _iTotalCounts);
        else
          printf("#    [ %-4g , %-4g ) : %7d (%4.1f%%)\n",
           _fFrom + iIndex * _fInterval,
           _fFrom + (1 + iIndex) * _fInterval, _lCounts[iIndex],
           _lCounts[iIndex] * 100.0 / _iTotalCounts);
      }
    }

    if (_lCounts[_lCounts.size() - 1] != 0)
      printf("#    [ %-4g , +Inf ) : %7d (%4.1f%%)\n",
       _fFrom + (_lCounts.size() - 2) * _fInterval,
       _lCounts[_lCounts.size() - 1],
       _lCounts[_lCounts.size() - 1] * 100.0 / _iTotalCounts);
  }

  float max()         {return _fMax;}
  float min()         {return _fMin;}
  float avg()         {return _fAvg;}
  float avgInRange()  {return _fAvgInRange;}

private:
  float _fFrom;
  float _fTo;
  float _fInterval;
  std::vector < int >_lCounts;  // The last two elements for storing (v < _fFrom) and ( v >= _fTo + _fInterval ) values
  int   _iTotalCounts;
  bool  _bIntRange;
  float _fMax, _fMin, _fAvg, _fAvgInRange;
};
