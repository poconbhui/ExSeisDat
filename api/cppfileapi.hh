////////////////////////////////////////////////////////////////////////////////
/// @file
/// @author Cathal O Broin - cathal@ichec.ie - first commit
/// @copyright TBD. Do not distribute
/// @date Autumn 2016
/// @brief
/// @details Primary C++ API header
////////////////////////////////////////////////////////////////////////////////
#ifndef PIOLCPPFILEAPI_INCLUDE_GUARD
#define PIOLCPPFILEAPI_INCLUDE_GUARD

#include "file/dynsegymd.hh"
#include "file/file.hh"
#include "global.hh"


namespace PIOL {

/*! This class provides access to the ExSeisPIOL class but with a simpler API
 */
class ExSeis : public ExSeisPIOL {
  public:
    /*! Constructor with optional maxLevel and which initialises MPI.
     *  @param[in] comm     The MPI communicator
     *  @param[in] maxLevel The maximum log level to be recorded.
     *  @return A shared pointer to a PIOL object.
     */
    static std::shared_ptr<ExSeis> New(
      const Verbosity maxLevel = PIOL_VERBOSITY_NONE,
      MPI_Comm comm            = MPI_COMM_WORLD)
    {
        return std::shared_ptr<ExSeis>(new ExSeis(maxLevel, comm));
    }

    /*! ExSeis Deleter.
     */
    ~ExSeis();

    /*! Shortcut to get the commrank.
     *  @return The comm rank.
     */
    size_t getRank(void) const;

    /*! Shortcut to get the number of ranks.
     *  @return The comm number of ranks.
     */
    size_t getNumRank(void) const;

    /*! Shortcut for a communication barrier
     */
    void barrier(void) const;

    /*! Return the maximum value amongst the processes
     *  @param[in] n The value to take part in the reduction
     *  @return Return the maximum value amongst the processes
     */
    size_t max(size_t n) const;

    /*! @brief A function to check if an error has occured in the PIOL. If an
     *         error has occured the log is printed, the object destructor is
     *         called and the code aborts.
     *  @param[in] msg A message to be printed to the log.
     */
    void isErr(const std::string& msg = "") const;

  private:
    /// The constructor is private! Use the ExSeis::New(...) function.
    /// @copydetails ExSeis::New
    ExSeis(
      const Verbosity maxLevel = PIOL_VERBOSITY_NONE,
      MPI_Comm comm            = MPI_COMM_WORLD);
};


namespace File {

/*! This class implements the C++14 File Layer API for the PIOL. It constructs
 *  the Data, Object and File layers.
 */
class ReadDirect {
  protected:
    /// The pointer to the base class (polymorphic)
    std::shared_ptr<ReadInterface> file;

  public:
    /*! Constructor with options.
     *  @tparam    D    The nested options structure of interest for the data
     *                  layer.
     *  @tparam    O    The nested options structure of interest for the object
     *                  layer.
     *  @tparam    F    The nested options structure of interest for the file
     *                  layer.
     *  @param[in] piol This PIOL ptr is not modified but is used to instantiate
     *                  another shared_ptr.
     *  @param[in] name The name of the file associated with the instantiation.
     *  @param[in] d    The Data options.
     *  @param[in] o    The Object options.
     *  @param[in] f    The File options.
     */
    template<class F, class O, class D>
    ReadDirect(
      std::shared_ptr<ExSeisPIOL> piol,
      const std::string name,
      const D& d,
      const O& o,
      const F& f)
    {
        auto data =
          std::make_shared<typename D::Type>(piol, name, d, FileMode::Read);
        auto obj = std::make_shared<typename O::Type>(
          piol, name, o, data, FileMode::Read);
        file = std::make_shared<typename F::Type>(piol, name, f, obj);
        if (!file)
            piol->log->record(
              name, Log::Layer::API, Log::Status::Error,
              "ReadInterface creation failure in ReadDirect<F,O,D>()",
              PIOL_VERBOSITY_NONE);
    }

    /*! Constructor without options.
     *  @param[in] piol This PIOL ptr is not modified but is used to instantiate
     *                  another shared_ptr.
     *  @param[in] name The name of the file associated with the instantiation.
     */
    ReadDirect(std::shared_ptr<ExSeisPIOL> piol, const std::string name);

    /*! Copy constructor for a std::shared_ptr<ReadInterface> object
     * @param[in] file The ReadInterface shared_ptr object.
     */
    ReadDirect(std::shared_ptr<ReadInterface> file);

    /*! Empty destructor
     */
    ~ReadDirect(void);

    /*! Overload of member of pointer access
     *  @return Return the base File layer class Interface.
     */
    ReadInterface* operator->() const { return file.get(); }

    /*! Operator to convert to an Interface object.
     *  @return Return the internal \c Interface pointer.
     */
    operator ReadInterface*() const { return file.get(); }

    /*! @brief Read the human readable text from the file.
     *  @return A string containing the text (in ASCII format).
     */
    const std::string& readText(void) const;

    /*! @brief Read the number of samples per trace
     *  @return The number of samples per trace
     */
    size_t readNs(void) const;

    /*! @brief Read the number of traces in the file
     *  @return The number of traces
     */
    size_t readNt(void) const;

    /*! @brief Read the number of increment between trace samples
     *  @return The increment between trace samples
     */
    geom_t readInc(void) const;

    /*! @brief Read the traces from offset to offset+sz.
     *  @param[in]  offset The starting trace number.
     *  @param[in]  sz     The number of traces to process
     *  @param[out] trace  A contiguous array of each trace
     *                     (size sz*ns*sizeof(trace_t))
     *  @param[out] prm    The parameter structure
     *
     *  @details When prm==PIOL_PARAM_NULL only the trace DF is read.
     */
    void readTrace(
      const size_t offset,
      const size_t sz,
      trace_t* trace,
      Param* prm = PIOL_PARAM_NULL) const;

    /*! @brief Function to read the trace parameters from offset to offset+sz of
     *         the respective trace headers.
     *  @param[in]  offset The starting trace number.
     *  @param[in]  sz     The number of traces to process.
     *  @param[out] prm    The parameter structure
     */
    void readParam(const size_t offset, const size_t sz, Param* prm) const;

    /*! @brief Read the traces specified by the offsets in the passed offset
     *         array. The offsets should in ascending order,
     *         i.e. offset[i] < offset[i+1].
     *  @param[in]  sz     The number of traces to process
     *  @param[in]  offset An array of trace numbers to read.
     *  @param[out] trace  A contiguous array of each trace
     *                     (size sz*ns*sizeof(trace_t))
     *  @param[out] prm    The parameter structure
     *
     *  @details When prm==PIOL_PARAM_NULL only the trace DF is read.
     */
    void readTraceNonContiguous(
      const size_t sz,
      const size_t* offset,
      trace_t* trace,
      Param* prm = PIOL_PARAM_NULL) const;

    /*! @brief Read the traces specified by the offsets in the passed offset
     *         array. The offset array need not be in any order.
     *  @param[in]  sz     The number of traces to process
     *  @param[in]  offset An array of trace numbers to read.
     *  @param[out] trace  A contiguous array of each trace
     *                     (size sz*ns*sizeof(trace_t))
     *  @param[out] prm    The parameter structure
     *
     *  @details When prm==PIOL_PARAM_NULL only the trace DF is read.
     */
    void readTraceNonMonotonic(
      const size_t sz,
      const size_t* offset,
      trace_t* trace,
      Param* prm = PIOL_PARAM_NULL) const;

    /*! @brief Read the traces specified by the offsets in the passed offset
     *         array. The offsets should be in ascending order,
     *         i.e. offset[i] < offset[i+1].
     *  @param[in] sz     The number of traces to process
     *  @param[in] offset An array of trace numbers to read.
     *  @param[out] prm   The parameter structure
     */
    void readParamNonContiguous(
      const size_t sz, const size_t* offset, Param* prm) const;
};

/*! This class implements the C++14 File Layer API for the PIOL. It constructs
 *  the Data, Object and File layers.
 */
class WriteDirect {
  protected:
    /// The pointer to the base class (polymorphic)
    std::shared_ptr<WriteInterface> file;

  public:
    /*! Constructor with options.
     *  @tparam    D    The nested options structure of interest for the data
     *                  layer.
     *  @tparam    O    The nested options structure of interest for the object
     *                  layer.
     *  @tparam    F    The nested options structure of interest for the file
     *                  layer.
     *  @param[in] piol This PIOL ptr is not modified but is used to instantiate
     *                  another shared_ptr.
     *  @param[in] name The name of the file associated with the instantiation.
     *  @param[in] d    The Data options.
     *  @param[in] o    The Object options.
     *  @param[in] f    The File options.
     */
    template<class D, class O, class F>
    WriteDirect(
      std::shared_ptr<ExSeisPIOL> piol,
      const std::string name,
      const D& d,
      const O& o,
      const F& f)
    {
        auto data =
          std::make_shared<typename D::Type>(piol, name, d, FileMode::Write);
        auto obj = std::make_shared<typename O::Type>(
          piol, name, o, data, FileMode::Write);
        file = std::make_shared<typename F::Type>(piol, name, f, obj);
        if (!file)
            piol->log->record(
              name, Log::Layer::API, Log::Status::Error,
              "WriteInterface creation failure in WriteDirect<F,O,D>()",
              PIOL_VERBOSITY_NONE);
    }

    /*! Constructor without options.
     *  @param[in] piol This PIOL ptr is not modified but is used to instantiate
     *                  another shared_ptr.
     *  @param[in] name The name of the file associated with the instantiation.
     */
    WriteDirect(std::shared_ptr<ExSeisPIOL> piol, const std::string name);

    /*! Copy constructor for a std::shared_ptr<ReadInterface> object
     * @param[in] file The ReadInterface shared_ptr object.
     */
    WriteDirect(std::shared_ptr<WriteInterface> file);

    /*! Empty destructor
     */
    ~WriteDirect(void);

    /*! Overload of member of pointer access
     *  @return Return the base File layer class Interface.
     */
    WriteInterface& operator->() const { return *file; }

    /*! Operator to convert to an Interface object.
     *  @return Return the internal \c Interface pointer.
     */
    operator WriteInterface&() const { return *file; }

    /*! @brief Write the human readable text from the file.
     *  @param[in] text_ The new string containing the text (in ASCII format).
     */
    void writeText(const std::string text_);

    /*! @brief Write the number of samples per trace
     *  @param[in] ns_ The new number of samples per trace.
     */
    void writeNs(const size_t ns_);

    /*! @brief Write the number of traces in the file
     *  @param[in] nt_ The new number of traces.
     */
    void writeNt(const size_t nt_);

    /*! @brief Write the increment between trace samples.
     *  @param[in] inc_ The new increment between trace samples.
     */
    void writeInc(const geom_t inc_);

    /*! @brief Read the traces from offset to offset+sz.
     *  @param[in] offset The starting trace number.
     *  @param[in] sz     The number of traces to process
     *  @param[in] trace  A contiguous array of each trace
     *                    (size sz*ns*sizeof(trace_t))
     *  @param[in] prm    The parameter structure
     *  @warning This function is not thread safe.
     *
     *  @details When prm==PIOL_PARAM_NULL only the trace DF is written.
     */
    void writeTrace(
      const size_t offset,
      const size_t sz,
      trace_t* trace,
      const Param* prm = PIOL_PARAM_NULL);

    /*! @brief Write the trace parameters from offset to offset+sz to the
     *         respective trace headers.
     *  @param[in] offset The starting trace number.
     *  @param[in] sz     The number of traces to process.
     *  @param[in] prm    The parameter structure
     *
     *  @details It is assumed that this operation is not an update. Any
     *           previous contents of the trace header will be overwritten.
     */
    void writeParam(const size_t offset, const size_t sz, const Param* prm);

    /*! @brief write the traces specified by the offsets in the passed offset
     *         array.
     *  @param[in] sz     The number of traces to process
     *  @param[in] offset An array of trace numbers to write.
     *  @param[in] trace  A contiguous array of each trace
     *                    (size sz*ns*sizeof(trace_t))
     *  @param[in] prm    The parameter structure
     *
     *  @details When prm==PIOL_PARAM_NULL only the trace DF is written.  It is
     *           assumed that the parameter writing operation is not an update.
     *           Any previous contents of the trace header will be overwritten.
     */
    void writeTraceNonContiguous(
      const size_t sz,
      const size_t* offset,
      trace_t* trace,
      const Param* prm = PIOL_PARAM_NULL);

    /*! @brief write the traces specified by the offsets in the passed offset
     *         array.
     *  @param[in] sz     The number of traces to process
     *  @param[in] offset An array of trace numbers to write.
     *  @param[in] prm    The parameter structure
     *
     *  @details It is assumed that the parameter writing operation is not an
     *           update. Any previous contents of the trace header will be
     *           overwritten.
     */
    void writeParamNonContiguous(
      const size_t sz, const size_t* offset, const Param* prm);
};


/// @todo DOCUMENT brief
class ReadModel : public ReadDirect {
  public:
    /// @param[in] piol_ This PIOL ptr is not modified but is used to
    ///                  instantiate another shared_ptr.
    /// @param[in] name_ The name of the file associated with the instantiation.
    ReadModel(std::shared_ptr<ExSeisPIOL> piol_, const std::string name_);

    /// @todo DOCUMENT brief and return type
    /// @param[in] gOffset   DOCUMENT ME
    /// @param[in] numGather DOCUMENT ME
    /// @param[in] gather    DOCUMENT ME
    /// @return DOCUMENT ME
    std::vector<trace_t> virtual readModel(
      size_t gOffset, size_t numGather, Uniray<size_t, llint, llint>& gather);
};

}  // namespace File
}  // namespace PIOL

#endif
