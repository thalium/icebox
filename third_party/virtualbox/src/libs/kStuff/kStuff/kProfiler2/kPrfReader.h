

#include <string>

typedef

/**
 * Debug info cache.
 *
 * An objects of this class acts a frontend to the low-level
 * debug info readers.
 */
class kPrfDebugInfoCache
{
public:
    kPrfDebugInfoCache(unsigned cMaxModules = ~0U);
    ~kPrfDebugInfoCache();

    /** Resolves a symbol in a specific module. */
    int findSymbol();
    int findLine();
};

/**
 * Internal class which does the reader job behind the API / commandline tool.
 */
class kPrfReader
{
public:
    kPrfReader(const char *pszDataSetPath);
    ~kPrfReader();

    /** Analyses the data set. */
    int analyse(int fSomeOptionsIHaventFiguredOutYet);

    /** Writes the analysis report as HTML. */
    int reportAsHtml(FILE *pOut);

    /** Dumps the data set in a raw fashion to the specified file stream. */
    int dump(FILE *pOut);

protected:
    /** Pointer to the debug info cache object. */
    kPrfDebugInfoCache *pDbgCache;
};
