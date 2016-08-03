/*******************************************************************************************//*!
 *   \file
 *   \author Cathal O Broin - cathal@ichec.ie - first commit
 *   \copyright TBD. Do not distribute
 *   \date July 2016
 *   \brief The File layer interface
 *   \details The File layer interface is a base class which specific File implementations
 *   work off
*//*******************************************************************************************/
#ifndef PIOLFILE_INCLUDE_GUARD
#define PIOLFILE_INCLUDE_GUARD
#include "global.hh"

namespace PIOL { namespace File {
typedef std::pair<geom_t, geom_t> coord_t;  //!< The type for coordinate points
typedef std::pair<llint, llint> grid_t;     //!< The type for grid points

/*! \brief Possible coordinate sets
 */
enum class Coord : size_t
{
    Src,    //!< Source Coordinates
    Rcv,    //!< Receiver Coordinates
    Cmp     //!< Common Midpoint Coordinates
};

/*! \brief Possible Grids
 */
enum class Grid : size_t
{
    Line    //!< Inline/Crossline grid points
};

/*! \brief The File layer interface. Specific File implementations
 *  work off this base class.
 */
class Interface
{
    protected :
    std::shared_ptr<ExSeisPIOL> piol;               //!< The PIOL object.
    std::string name;                               //!< Store the file name for debugging purposes.
    std::shared_ptr<Obj::Interface> obj = nullptr;  //!< Pointer to the Object-layer object (polymorphic).
    size_t ns;                                      //!< The number of samples per trace.
    size_t nt;                                      //!< The number of traces.
    std::string text;                               //!< Human readable text extracted from the file
    geom_t inc;                                     //!< The increment between samples in a trace

    /*! \brief The constructor used for unit testing. It does not try to create an Object-layer object
     *  \param[in] piol_ This PIOL ptr is not modified but is used to instantiate another shared_ptr.
     *  \param[in] name_ The name of the file associated with the instantiation.
     *  \param[in] obj_ Pointer to the associated Object-layer object.
     */
    Interface(const std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, const std::shared_ptr<Obj::Interface> obj_);

    public :
    /*! \brief The constructor.
     *  \param[in] piol_ This PIOL ptr is not modified but is used to instantiate another shared_ptr.
     *  \param[in] name_ The name of the file associated with the instantiation.
     *  \param[in] objOpt The options to use when creating the Object layer
     *  \param[in] dataOpt The options to use when creating the Data layer
     */
    Interface(const std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, const Obj::Opt & objOpt, const Data::Opt & dataOpt);

    /*! \brief Read the human readable text from the file
     *  \return A string containing the text (in ASCII format)
     */
    std::string readText(void) const;

    /*! \brief Read the number of samples per trace
     *  \return The number of samples per trace
     */
    size_t readNs(void) const;

    /*! \brief Read the number of traces in the file
     *  \return The number of traces
     */
    size_t readNt(void) const;

    /*! \brief Read the number of increment between trace samples
     *  \return The increment between trace samples
     */
    geom_t readInc(void) const;

    /*! \brief Pure virtual function to read the ith-trace coordinate pair
     *  \param[in] item The coordinate pair of interest
     *  \param[in] i The trace number
     *  \return The ith-trace coordinate pair
     */
    virtual coord_t readCoordPoint(const Coord item, const size_t i) const = 0;

    /*! \brief Pure virtual function to read coordinate pairs from the ith-trace to i+sz.
     *  \param[in] item The coordinate pair of interest
     *  \param[in] i The starting trace number
     *  \param[in] sz The tumber of traces to process
     *  \param[out] buf The buffer
     */
    virtual void readCoordPoint(const Coord item, csize_t i, csize_t sz, coord_t * buf) const = 0;

    /*! \brief Pure virtual function to read grid pairs from the ith-trace to i+sz.
     *  \param[in] item The grid pair of interest
     *  \param[in] i The starting trace number
     *  \param[in] sz The tumber of traces to process
     *  \param[out] buf The buffer
     */
    virtual void readGridPoint(const Grid item, csize_t i, csize_t sz, grid_t * buf) const = 0;

    /*! \brief Pure virtual function to read the ith-trace grid pair
     *  \param[in] item The Grid pair of interest
     *  \param[in] i The trace number
     *  \return The ith-trace grid pair
     */
    virtual grid_t readGridPoint(const Grid item, const size_t i) const = 0;

    /*! \brief Pure virtual function to write the human readable text from the file.
     *  \param[in] text_ The new string containing the text (in ASCII format).
     */
    virtual void writeText(const std::string text_) = 0;

    /*! \brief Pure virtual function to write the number of samples per trace
     *  \param[in] ns_ The new number of samples per trace.
     */
    virtual void writeNs(const size_t ns_) = 0;

    /*! \brief Pure virtual function to write the number of traces in the file
     *  \param[in] nt_ The new number of traces.
     */
    virtual void writeNt(const size_t nt_) = 0;

    /*! \brief Pure virtual function to write the number of increment between trace samples.
     *  \param[in] inc_ The new increment between trace samples.
     */
    virtual void writeInc(const geom_t inc_) = 0;

    /*! \brief Pure virtual function to write the ith-trace coordinate pair.
     *  \param[in] item The coordinate pair of interest
     *  \param[in] i The trace number.
     *  \param[in] coord The coordinate to write
     */
    virtual void writeCoordPoint(const Coord item, const size_t i, const coord_t coord) const = 0;

    /*! \brief Pure virtual function to write the ith-trace grid pair.
     *  \param[in] item The Grid pair of interest
     *  \param[in] i The trace number.
     *  \param[in] grid the grid point to write.
     */
    virtual void writeGridPoint(const Grid item, const size_t i, const grid_t grid) const = 0;
};

/*! \brief An enum of the possible derived classes for the file layer.
 */
enum class Type : size_t
{
    SEGY //!< The SEGY implementation. Currently the only option.
};

/*! \brief The base-options structure. Specific File implementations include a derived version of this.
 */
struct Opt
{
    Type type;      //!< The File type.

    /* \brief Default constructor to prevent intel warnings
     */
    Opt(void)
    {
        type = Type::SEGY;      //!< The File type.
    }

    /*! \brief This function returns the File type. This function is mainly included to provide a virtual function
     * to allow polymorphic behaviour.
     */
    virtual Type getType(void) const
    {
        return type;
    }
};
}}
#endif