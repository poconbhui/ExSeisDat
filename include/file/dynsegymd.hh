#include "global.hh"
#include <unordered_map>

namespace PIOL { namespace File {
struct Rule;

/*! Misc Trace Header offsets
 */
enum class Tr : size_t
{
    SeqNum      = 1U,   //!< int32_t. The trace sequence number within the Line
    SeqFNum     = 5U,   //!< int32_t. The trace sequence number within SEG-Y File
    ORF         = 9U,   //!< int32_t. The original field record number.
    TORF        = 13U,  //!< int32_t. The trace number within the ORF.
    RcvElv      = 41U,  //!< int32_t. The Receiver group elevation
    SurfElvSrc  = 45U,  //!< int32_t. The surface elevation at the source.
    SrcDpthSurf = 49U,  //!< int32_t. The source depth below surface (opposite of above?).
    DtmElvRcv   = 53U,  //!< int32_t. The datum elevation for the receiver group.
    DtmElvSrc   = 57U,  //!< int32_t. The datum elevation for the source.
    WtrDepSrc   = 61U,  //!< int32_t. The water depth for the source.
    WtrDepRcv   = 65U,  //!< int32_t. The water depth for the receive group.
    ScaleElev   = 69U,  //!< int16_t. The scale coordinate for 41-68 (elevations + depths)
    ScaleCoord  = 71U,  //!< int16_t. The scale coordinate for 73-88 + 181-188
    xSrc        = 73U,  //!< int32_t. The X coordinate for the source
    ySrc        = 77U,  //!< int32_t. The Y coordinate for the source
    xRcv        = 81U,  //!< int32_t. The X coordinate for the receive group
    yRcv        = 85U,  //!< int32_t. The Y coordinate for the receive group
    xCmp        = 181U, //!< int32_t  The X coordinate for the CMP
    yCmp        = 185U, //!< int32_t. The Y coordinate for the CMP
    il          = 189U, //!< int32_t. The Inline grid point.
    xl          = 193U  //!< int32_t. The Crossline grid point.
};

enum class Meta : size_t
{
    xSrc,
    ySrc,
    xRcv,
    yRcv,
    xCmp,
    yCmp,
    il,
    xl,
    tn
};
//C compatible structure
struct Param
{
    geom_t * f;
    llint *  i;
    short *  s;
    size_t * t;
};

struct prmRet
{
    union
    {
        llint i;
        geom_t f;
        short s;
    } val;

    operator long int ()
    {
        return val.i;
    }

    operator int ()
    {
        return val.i;
    }

    operator size_t ()
    {
        return val.i;
    }

    operator float ()
    {
        return val.f;
    }

    operator double ()
    {
        return val.f;
    }

    operator short ()
    {
        return val.s;
    }
};

struct EnumHash
{
    template <typename T>
    size_t operator()(T t) const
    {
        return static_cast<size_t>(t);
    }
};

enum class MdType : size_t
{
    Long,
    Short,
    Float
};

struct RuleEntry
{
    size_t num;
    size_t loc;
    RuleEntry(size_t num_, Tr loc_) : num(num_), loc(size_t(loc_)) { }
    virtual size_t min(void) = 0;
    virtual size_t max(void) = 0;
    virtual MdType type(void) = 0;
};

class Rule
{
    protected :
    size_t numLong;     //Number of long rules
    size_t numFloat;    //Number of float rules
    size_t numShort;    //Number of short rules
    size_t start;
    size_t end;
    struct
    {
        uint32_t badextent;
        uint32_t fullextent;
    } flag;

    std::unordered_map<Meta, RuleEntry *, EnumHash> translate;
    public :
    Rule(bool full, bool defaults);

    void addLong(Meta m, Tr loc);
    void addFloat(Meta m, Tr loc, Tr scalLoc);
    void addShort(Meta m, Tr loc);
    void rmRule(Meta);
    size_t extent(void);
};

class DynParam : public Rule
{
    Param prm;

    size_t sz;
    size_t stride;
    public :
    DynParam(bool full, bool defaults, csize_t sz_ = 0, csize_t stride_ = 0);
    ~DynParam(void);

    prmRet getPrm(size_t i, Meta entry);
    void setPrm(size_t i, Meta entry, geom_t val);
    void setPrm(size_t i, Meta entry, llint val);
    void setPrm(size_t i, Meta entry, short val);
    //Put to buf
    void fill(uchar * buf);
    //Take from buf
    void take(const uchar * buf);
};
}}
