/*******************************************************************************************//*!
 *   \file
 *   \author Cathal O Broin - cathal@ichec.ie - first commit
 *   \copyright TBD. Do not distribute
 *   \date July 2016
 *   \brief The SEGY implementation of the Object layer interface.
 *   \details The SEGY specific implementation of the Object layer interface.
*//*******************************************************************************************/
#ifndef PIOLOBJSEGY_INCLUDE_GUARD
#define PIOLOBJSEGY_INCLUDE_GUARD
#include "global.hh"
#include "object/object.hh"
#include "data/datampiio.hh"    //For the default data layer
#include "share/segy.hh"
#include "share/units.hh"
namespace PIOL { namespace Obj {
struct SEGYFileHeader;

/*! \brief The Read SEG-Y Obj class.
 */
class ReadSEGY : public ReadInterface
{
    public :
    typedef Data::MPIIO DataT;
    std::shared_ptr<SEGYFileHeader> desc;

    /*! \brief The Read SEG-Y options structure. Currently empty.
    */
    struct Opt
    {
        typedef ReadSEGY Type;  //!< The type of the class this structure is nested in.
        unit_t incFactor;       //!< The increment factor to multiply inc by (default to SEG-Y rev 1 standard definition)

        /* \brief Default constructor to prevent intel warnings.
         */
        Opt(void)
        {
            incFactor = SI::Micro;
        }
    };

    private :
    void Init(const Opt * opt);

    public :
    /*! \brief The Read SEGY-Obj class constructor.
     *  \param[in] piol_ This PIOL ptr is not modified but is used to instantiate another shared_ptr.
     *  \param[in] name_ The name of the file associated with the instantiation.
     *  \param[in] opt_  The SEGY options.
     *  \param[in] data_ Pointer to the Data layer object (polymorphic).
     */
    ReadSEGY(const Piol piol_, const std::string name_, const Opt * opt_, std::shared_ptr<Data::Interface> data_);

    /*! \brief The Read SEGY-Obj class constructor.
     *  \param[in] piol_ This PIOL ptr is not modified but is used to instantiate another shared_ptr.
     *  \param[in] name_ The name of the file associated with the instantiation.
     *  \param[in] data_ Pointer to the Data layer object (polymorphic).
     */
    ReadSEGY(const Piol piol_, const std::string name_, std::shared_ptr<Data::Interface> data_);

    ReadSEGY(const Piol piol_, const std::string name_);

    size_t getFileSz(void) const;

    std::shared_ptr<FileMetadata> readHO(void) const;

    void readDOMD(csize_t offset, csize_t ns, csize_t sz, uchar * md) const;

    void readDODF(csize_t offset, csize_t ns, csize_t sz, uchar * df) const;

    void readDO(csize_t offset, csize_t ns, csize_t sz, uchar * d) const;

    void readDO(csize_t * offset, csize_t ns, csize_t sz, uchar * d) const;

    void readDOMD(csize_t * offset, csize_t ns, csize_t sz, uchar * md) const;

    void readDODF(csize_t * offset, csize_t ns, csize_t sz, uchar * df) const;
};

/*! \brief The Write SEG-Y Obj class.
 */
class WriteSEGY : public WriteInterface
{
    public :
    typedef Data::MPIIO DataT;
    /*! \brief The Write SEG-Y options structure. Currently empty.
    */
    struct Opt
    {
        typedef WriteSEGY Type;  //!< The Type of the class this structure is nested in.
        unit_t incFactor;        //!< The increment factor to multiply inc by (default to SEG-Y rev 1 standard definition)
        /* \brief Default constructor to prevent intel warnings.
         */
        Opt(void)
        {
            incFactor = SI::Micro;
        }
    };

    /*! \brief The Write SEGY-Obj class constructor.
     *  \param[in] piol_ This PIOL ptr is not modified but is used to instantiate another shared_ptr.
     *  \param[in] name_ The name of the file associated with the instantiation.
     *  \param[in] opt_  The SEGY options.
     *  \param[in] data_ Pointer to the Data layer object (polymorphic).
     */
    WriteSEGY(const Piol piol_, const std::string name_, const Opt * opt_, std::shared_ptr<Data::Interface> data_);

    /*! \brief The Write SEGY-Obj class constructor.
     *  \param[in] piol_ This PIOL ptr is not modified but is used to instantiate another shared_ptr.
     *  \param[in] name_ The name of the file associated with the instantiation.
     *  \param[in] data_ Pointer to the Data layer object (polymorphic).
     */
    WriteSEGY(const Piol piol_, const std::string name_, std::shared_ptr<Data::Interface> data_);

    WriteSEGY(const Piol piol_, const std::string name_) : WriteInterface(piol_, name_, std::make_shared<DataT>(piol_, name_, FileMode::Write))
    {}

    ~WriteSEGY(void);

    void setFileSz(csize_t sz) const;

    void writeHO(const std::shared_ptr<FileMetadata> ho) const;

    void writeDOMD(csize_t offset, csize_t ns, csize_t sz, const uchar * md) const;

    void writeDODF(csize_t offset, csize_t ns, csize_t sz, const uchar * df) const;

    void writeDO(csize_t offset, csize_t ns, csize_t sz, const uchar * d) const;

    void writeDO(csize_t * offset, csize_t ns, csize_t sz, const uchar * d) const;

    void writeDOMD(csize_t * offset, csize_t ns, csize_t sz, const uchar * md) const;

    void writeDODF(csize_t * offset, csize_t ns, csize_t sz, const uchar * df) const;
};

struct SEGYFileHeader : public FileMetadata
{
    SEGY::Format format;    //!< Type formats
    unit_t incFactor;       //!< The increment factor

    SEGYFileHeader(ExSeisPIOL * piol, std::string name, const ReadSEGY::Opt * opt, csize_t fsz, std::vector<uchar> & buf);
    SEGYFileHeader(void) {}
    bool operator==(SEGYFileHeader & other)
    {
        return format == other.format && incFactor == other.incFactor && FileMetadata::operator==(other);
    }
};
}}
#endif
