#ifndef MemoryState_H

#include <sys/types.h>
#include <signal.h>
#include <QtGui>

typedef unsigned		uint32;
typedef unsigned long long	uint64;

inline uint32 SYSmax(uint32 a, uint32 b)
{
    return a > b ? a : b;
}
inline uint32 SYSmin(uint32 a, uint32 b)
{
    return a < b ? a : b;
}

class MemoryState {
public:
     MemoryState();
    ~MemoryState();

    bool	openPipe(int argc, char *argv[]);
    bool	loadFromPipe(int max_read);

    enum Visualization {
	LINEAR,
	BLOCK
    };
    void	setVisualization(Visualization vis)
		{ myVisualization = vis; }
    void	fillImage(QImage &image) const;

private:
    void	fillLinear(QImage &image) const;
    void	fillRecursiveBlock(QImage &image) const;

    typedef uint32	State;

    static const int	theTopBytes = 16;
    static const uint32	theTopSize = 1 << theTopBytes;
    static const uint32	theTopMask = theTopSize-1;

    static const int	theBottomBytes = 32-theTopBytes;
    static const uint32	theBottomSize = 1 << theBottomBytes;
    static const uint32	theBottomMask = theBottomSize-1;

    struct StateArray {
	StateArray()
	{
	    memset(myState, 0, theBottomSize*sizeof(State));
	    memset(myType, 0, theBottomSize*sizeof(char));
	}

	State	myState[theBottomSize];
	char	myType[theBottomSize];
    };

    int		topIndex(uint32 addr) const
		{ return (addr >> (32-theTopBytes)) & theTopMask; }
    int		bottomIndex(uint32 addr) const
		{ return addr & theBottomMask; }

    State	getEntry(uint32 addr) const
		{
		    StateArray	*row = myTable[topIndex(addr)];
		    return row ? row->myState[bottomIndex(addr)] : 0;
		}
    void	setEntry(uint32 addr, State val, char type)
		{
		    StateArray	*&row = myTable[topIndex(addr)];
		    int		  idx = bottomIndex(addr);
		    if (!row)
			row = new StateArray;
		    row->myState[idx] = val;
		    row->myType[idx] = type;
		}

    uint32	mapColor(StateArray *row, int j) const
		{
		    State	val = row->myState[j];
		    char	type = row->myType[j];
		    uint32	diff;

		    if (myTime >= val)
			diff = myTime - val + 1;
		    else
			diff = val - myTime + 1;

		    uint32	bits = __builtin_clz(diff);
		    uint32	clr = bits*8;

		    if (bits > 28)
			clr += ~(diff << (bits-28)) & 7;
		    else
			clr += ~(diff >> (28-bits)) & 7;

		    return type == 'I' ? myILut[clr] :
			(type == 'L' ? myRLut[clr] : myWLut[clr]);
		}

private:
    // Raw memory state
    StateArray	*myTable[theTopSize];
    State	 myTime;	// Rolling counter

    // Display LUT
    uint32	 myILut[256];
    uint32	 myRLut[256];
    uint32	 myWLut[256];

    // Child process
    pid_t	 myChild;
    FILE	*myPipe;

    // The number of low-order bits to ignore.  This value determines the
    // resolution and memory use for the profile.
    int		 myIgnoreBits;

    Visualization	myVisualization;
};

#endif