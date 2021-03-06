////////////////////////////////////////////////////////////////////////////////
/// @file
/// @author Cathal O Broin - cathal@ichec.ie - first commit
/// @copyright TBD. Do not distribute
/// @date November 2016
/// @brief
/// @details
////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <glob.h>
#include <map>
#include <numeric>
#include <regex>
#include <tuple>

#include "data/datampiio.hh"
#include "file/filesegy.hh"
#include "flow/set.hh"
#include "global.hh"
#include "object/objsegy.hh"
#include "ops/agc.hh"
#include "ops/gather.hh"
#include "ops/minmax.hh"
#include "ops/sort.hh"
#include "ops/taper.hh"
#include "ops/temporalfilter.hh"
#include "share/decomp.hh"
#include "share/misc.hh"  //For getSort..
#include "share/uniray.hh"

// TODO: remove this when all errors are addressed
#include <iostream>

namespace PIOL {

/*! For CoordElem. Update the dst element based on if the operation gives true.
 *  If the elements have the same value, set the trace number to the
 *  smallest trace number.
 *  @tparam Op The true/false operation to use for comparison
 *  @param[in] src The source one will be using for updating
 *  @param[in, out] dst The destination which will be updated.
 */
template<typename Op>
void updateElem(CoordElem* src, CoordElem* dst)
{
    Op op;
    if (src->val == dst->val)
        dst->num = std::min(dst->num, src->num);
    else if (op(src->val, dst->val)) {
        dst->val = src->val;
        dst->num = src->num;
    }
}

//////////////////////////////// CLASS MEMBERS /////////////////////////////////

Set::Set(
  std::shared_ptr<ExSeisPIOL> piol_,
  std::string pattern,
  std::string outfix_,
  std::shared_ptr<File::Rule> rule_) :
    piol(piol_),
    outfix(outfix_),
    rule(rule_),
    cache(piol_)
{
    rank    = piol->comm->getRank();
    numRank = piol->comm->getNumRank();
    add(pattern);
}

Set::Set(std::shared_ptr<ExSeisPIOL> piol_, std::shared_ptr<File::Rule> rule_) :
    piol(piol_),
    rule(rule_),
    cache(piol_)
{
    rank    = piol->comm->getRank();
    numRank = piol->comm->getNumRank();
}

Set::~Set(void)
{
    if (outfix != "") output(outfix);
}

void Set::add(std::unique_ptr<File::ReadInterface> in)
{
    file.emplace_back(std::make_shared<FileDesc>());
    auto& f = file.back();
    f->ifc  = std::move(in);

    auto dec = decompose(piol.get(), f->ifc.get());
    f->ilst.resize(dec.second);
    f->olst.resize(dec.second);
    std::iota(f->ilst.begin(), f->ilst.end(), dec.first);

    auto key =
      std::make_pair<size_t, geom_t>(f->ifc->readNs(), f->ifc->readInc());
    fmap[key].emplace_back(f);

    auto& off = offmap[key];
    std::iota(f->olst.begin(), f->olst.end(), off + dec.first);
    off += f->ifc->readNt();
}

void Set::add(std::string pattern)
{
    outmsg = "ExSeisPIOL: Set layer output\n";
    glob_t globs;
    int err = glob(pattern.c_str(), GLOB_TILDE | GLOB_MARK, NULL, &globs);
    if (err) exit(-1);

    std::regex reg(
      ".*se?gy$", std::regex_constants::icase | std::regex_constants::optimize
                    | std::regex::extended);

    for (size_t i = 0; i < globs.gl_pathc; i++) {
        // For each input file which matches the regex
        if (std::regex_match(globs.gl_pathv[i], reg)) {
            add(File::makeFile<File::ReadSEGY>(piol, globs.gl_pathv[i]));
        }
    }
    globfree(&globs);
    piol->isErr();
}

void Set::summary(void) const
{
    for (auto& f : file) {
        std::string msg = "name: " + f->ifc->readName() + "\n"
                          + "-\tNs: " + std::to_string(f->ifc->readNs()) + "\n"
                          + "-\tNt: " + std::to_string(f->ifc->readNt()) + "\n"
                          + "-\tInc: " + std::to_string(f->ifc->readInc())
                          + "\n";

        piol->log->record(
          "", Log::Layer::Set, Log::Status::Request, msg, PIOL_VERBOSITY_NONE);
    }

    if (!rank) {
        for (auto& m : fmap)
            piol->log->record(
              "", Log::Layer::Set, Log::Status::Request,
              "Local File count for (" + std::to_string(m.first.first) + " nt, "
                + std::to_string(m.first.second)
                + " inc) = " + std::to_string(m.second.size()),
              PIOL_VERBOSITY_NONE);
        piol->log->procLog();
    }
}

void RadonState::makeState(
  const std::vector<size_t>& offset, const Uniray<size_t, llint, llint>& gather)
{
    // TODO: DON'T USE MAGIC NAME
    std::unique_ptr<File::ReadSEGYModel> vm =
      File::makeFile<File::ReadSEGYModel>(piol, vmname);
    vNs  = vm->readNs();
    vInc = vm->readInc();

    vtrc = vm->readModel(offset.size(), offset.data(), gather);

    il.resize(offset.size());
    xl.resize(offset.size());

    for (size_t i = 0; i < offset.size(); i++) {
        auto gval = gather[offset[i]];
        il[i]     = std::get<1>(gval);
        xl[i]     = std::get<2>(gval);
    }
}

// TODO: Gather to Single is fine, Single to Gather is not
std::unique_ptr<TraceBlock> Set::calcFunc(
  FuncLst::iterator fCurr,
  const FuncLst::iterator fEnd,
  FuncOpt type,
  std::unique_ptr<TraceBlock> bIn)
{
    if (fCurr == fEnd || !(*fCurr)->opt.check(type)) return bIn;
    switch (type) {
        case FuncOpt::Gather:
            if ((*fCurr)->opt.check(FuncOpt::Gather)) {
                auto bOut = std::make_unique<TraceBlock>();
                dynamic_cast<Op<Mod>*>(fCurr->get())
                  ->func(bIn.get(), bOut.get());
                bIn = std::move(bOut);
            }
            ++fCurr;
            // fallthrough
        case FuncOpt::SingleTrace:
            if ((*fCurr)->opt.check(FuncOpt::SingleTrace)) {
                type = FuncOpt::SingleTrace;
                dynamic_cast<Op<InPlaceMod>*>(fCurr->get())->func(bIn.get());
            }
        default:
            break;
    }
    return calcFunc(++fCurr, fEnd, type, std::move(bIn));
}

std::vector<std::string> Set::startSingle(
  FuncLst::iterator fCurr, const FuncLst::iterator fEnd)
{
    std::vector<std::string> names;
    for (auto& o : fmap) {
        auto& fQue = o.second;

        std::string name;
        if (!fQue.size()) return std::vector<std::string>{};

        size_t ns  = fQue[0]->ifc->readNs();
        geom_t inc = fQue[0]->ifc->readInc();

        if (fQue.size() == 1)
            name = outfix + ".segy";
        else
            name =
              outfix + std::to_string(ns) + "_" + std::to_string(inc) + ".segy";

        names.push_back(name);

        std::unique_ptr<File::WriteInterface> out =
          File::makeFile<File::WriteSEGY>(piol, name);
        // TODO: Will need to delay ns call depending for operations that modify
        //       the number of samples per trace
        out->writeNs(ns);
        out->writeInc(inc);
        out->writeText(outmsg);

        const size_t memlim = 1024LU * 1024LU * 1024LU;
        size_t max          = memlim
                     / (5LU * sizeof(size_t) + SEGSz::getDOSz(ns)
                        + 2LU * rule->paramMem() + 2LU * SEGSz::getDFSz(ns));

        for (auto& f : fQue) {
            File::ReadInterface* in = f->ifc.get();
            size_t lnt              = f->ilst.size();
            size_t ns               = in->readNs();
            // Initialise the blocks
            auto biggest = piol->comm->max(lnt);
            size_t extra =
              biggest / max - lnt / max + (biggest % max > 0) - (lnt % max > 0);
            for (size_t i = 0; i < lnt; i += max) {
                size_t rblock = (i + max < lnt ? max : lnt - i);
                std::vector<trace_t> trc(rblock * ns);
// TODO: Rule should be made of rules stored in function list
#warning Use non-monotonic call here
                File::Param prm(rule, rblock);

                in->readTraceNonContiguous(
                  rblock, f->ilst.data() + i, trc.data(), &prm);
                std::vector<size_t> sortlist =
                  getSortIndex(rblock, f->olst.data() + i);

                auto bIn = std::make_unique<TraceBlock>();
                bIn->prm.reset(new File::Param(rule, rblock));
                bIn->trc.resize(rblock * ns);
                bIn->ns  = ns;
                bIn->inc = inc;

                for (size_t j = 0LU; j < rblock; j++) {
                    cpyPrm(sortlist[j], &prm, j, bIn->prm.get());
                    for (size_t k = 0LU; k < ns; k++)
                        bIn->trc[j * ns + k] = trc[sortlist[j] * ns + k];
                    sortlist[j] = f->olst[i + sortlist[j]];
                }

                auto bFinal =
                  calcFunc(fCurr, fEnd, FuncOpt::SingleTrace, std::move(bIn));
                out->writeTraceNonContiguous(
                  rblock, sortlist.data(), bFinal->trc.data(),
                  bFinal->prm.get());
            }

            for (size_t i = 0; i < extra; i++) {
                in->readTraceNonContiguous(0, nullptr, nullptr, nullptr);

                auto bIn = std::make_unique<TraceBlock>();
                bIn->prm.reset(new File::Param(rule, 0LU));
                bIn->trc.resize(0LU);
                bIn->ns  = f->ifc->readNs();
                bIn->nt  = 0LU;
                bIn->inc = f->ifc->readInc();

                calcFunc(fCurr, fEnd, FuncOpt::Gather, std::move(bIn));
                out->writeTraceNonContiguous(0, nullptr, nullptr, nullptr);
            }
        }
    }
    return names;
}

std::string Set::startGather(
  FuncLst::iterator fCurr, const FuncLst::iterator fEnd)
{
    if (file.size() > 1LU) {
        OpOpt opt = {FuncOpt::NeedMeta, FuncOpt::NeedTrcVal,
                     FuncOpt::SingleTrace};
        FuncLst tFunc;
        tFunc.push_back(std::make_shared<Op<InPlaceMod>>(
          opt, nullptr, nullptr, [](TraceBlock*) -> std::vector<size_t> {
              return std::vector<size_t>{};
          }));

        std::string tOutfix            = outfix;
        outfix                         = "temp";
        std::vector<std::string> names = calcFunc(tFunc.begin(), tFunc.end());
        outfix                         = tOutfix;
        drop();
        for (std::string n : names) {
            // TODO: Open with delete on close?
            add(n);
        }
    }
    std::string gname;
    for (auto& o : file) {
        // Locate gather boundaries.
        auto gather = File::getIlXlGathers(piol.get(), o->ifc.get());
        auto gdec   = decompose(gather.size(), numRank, rank);

        size_t numGather = gdec.second;

        std::vector<size_t> gNums;
        for (auto fTemp = fCurr;
             fTemp != fEnd && (*fTemp)->opt.check(FuncOpt::Gather); fTemp++) {
            auto* p = dynamic_cast<Op<Mod>*>(fTemp->get());
            assert(p);
            for (size_t i = 0; i < numGather; i++)
                gNums.push_back(i * numRank + rank);

            p->state->makeState(gNums, gather);
        }

        // TODO: Loop and add rules
        // TODO: need better rule handling, create rule of all rules in gather
        //       functions
        auto rule = std::make_shared<File::Rule>(
          std::initializer_list<Meta>{PIOL_META_il, PIOL_META_xl});

        auto fTemp = fCurr;
        while (++fTemp != fEnd && (*fTemp)->opt.check(FuncOpt::Gather))
            ;

        gname = (fTemp != fEnd ? "gtemp.segy" : outfix + ".segy");

        // Use inputs as default values. These can be changed later
        std::unique_ptr<File::WriteInterface> out =
          File::makeFile<File::WriteSEGY>(piol, gname);

        size_t wOffset = 0LU;
        size_t iOffset = 0LU;
        size_t extra   = piol->comm->max(numGather) - numGather;
        size_t ig      = 0;
        for (size_t gNum : gNums) {
            auto gval         = gather[gNum];
            const size_t iGSz = std::get<0>(gval);

            // Initialise the blocks
            auto bIn = std::make_unique<TraceBlock>();
            bIn->prm.reset(new File::Param(rule, iGSz));
            bIn->trc.resize(iGSz * o->ifc->readNs());
            bIn->ns   = o->ifc->readNs();
            bIn->nt   = o->ifc->readNt();
            bIn->inc  = o->ifc->readInc();
            bIn->numG = numGather;
            bIn->gNum = ig++;

            size_t ioff = piol->comm->offset(iGSz);
            o->ifc->readTrace(
              iOffset + ioff, bIn->prm->size(), bIn->trc.data(),
              bIn->prm.get());
            iOffset += piol->comm->sum(iGSz);

            auto bOut = calcFunc(fCurr, fEnd, FuncOpt::Gather, std::move(bIn));

            size_t woff = piol->comm->offset(bOut->prm->size());
            // For simplicity, the output is now
            out->writeNs(bOut->ns);
            out->writeInc(bOut->inc);

            out->writeTrace(
              wOffset + woff, bOut->prm->size(),
              (bOut->prm->size() ? bOut->trc.data() : nullptr),
              bOut->prm.get());
            wOffset += piol->comm->sum(bOut->prm->size());
        }
        for (size_t i = 0; i < extra; i++) {
            piol->comm->offset(0LU);
            o->ifc->readTrace(size_t(0), size_t(0), nullptr, nullptr);
            piol->comm->sum(0LU);

            auto bIn = std::make_unique<TraceBlock>();
            bIn->prm.reset(new File::Param(rule, 0LU));
            bIn->trc.resize(0LU);
            bIn->ns  = o->ifc->readNs();
            bIn->nt  = o->ifc->readNt();
            bIn->inc = o->ifc->readInc();

            auto bOut = calcFunc(fCurr, fEnd, FuncOpt::Gather, std::move(bIn));
            out->writeNs(bOut->ns);
            out->writeInc(bOut->inc);

            piol->comm->offset(0LU);
            out->writeTrace(size_t(0), size_t(0), nullptr, nullptr);
            piol->comm->sum(0LU);
        }
    }

    while (++fCurr != fEnd && (*fCurr)->opt.check(FuncOpt::Gather))
        ;

    if (fCurr != fEnd) {
        drop();
        add(gname);
        return "";
    }
    else
        return gname;
}

// calc for subsets only
FuncLst::iterator Set::calcFuncS(
  FuncLst::iterator fCurr, const FuncLst::iterator fEnd, FileDeque& fQue)
{
    std::shared_ptr<TraceBlock> block;

    if ((*fCurr)->opt.check(FuncOpt::NeedMeta)) {
        if (!(*fCurr)->opt.check(FuncOpt::NeedTrcVal))
            block = cache.cachePrm((*fCurr)->rule, fQue);
        else
            std::cerr << "Not implemented both trace + parameters yet\n";
    }
    else if ((*fCurr)->opt.check(FuncOpt::NeedTrcVal))
        block = cache.cacheTrc(fQue);

    // The operation call
    std::vector<size_t> trlist =
      dynamic_cast<Op<InPlaceMod>*>(fCurr->get())->func(block.get());

    size_t j = 0;
    for (auto& f : fQue) {
        std::copy(&trlist[j], &trlist[j + f->olst.size()], f->olst.begin());
        j += f->olst.size();
    }

    if (++fCurr != fEnd)
        if ((*fCurr)->opt.check(FuncOpt::SubSetOnly))
            return calcFuncS(fCurr, fEnd, fQue);
    return fCurr;
}

FuncLst::iterator Set::startSubset(
  FuncLst::iterator fCurr, const FuncLst::iterator fEnd)
{
    std::vector<FuncLst::iterator> flist;

    // TODO: Parallelisable
    for (auto& o : fmap) {
        // Iterate across the full function list
        flist.push_back(calcFuncS(fCurr, fEnd, o.second));
    }

    assert(std::equal(flist.begin() + 1LU, flist.end(), flist.begin()));

    return flist.front();
}

std::vector<std::string> Set::calcFunc(
  FuncLst::iterator fCurr, const FuncLst::iterator fEnd)
{
    if (fCurr != fEnd) {
        if ((*fCurr)->opt.check(FuncOpt::SubSetOnly))
            fCurr = startSubset(fCurr, fEnd);
        else if ((*fCurr)->opt.check(FuncOpt::Gather)) {
            std::string gname = startGather(fCurr, fEnd);
            // If startGather returned an empty string, then fCurr == fEnd was
            // reached.
            if (gname != "") {
                return std::vector<std::string>{gname};
            }

// TODO: Later this will need to be changed when the gather also continues with
//       single trace cases
#warning Trick goes here
            for (; fCurr != fEnd && (*fCurr)->opt.check(FuncOpt::Gather);
                 ++fCurr)
                ;
        }
        else if ((*fCurr)->opt.check(FuncOpt::SingleTrace)) {
            std::vector<std::string> sname = startSingle(fCurr, fEnd);
            if (sname.size()) return sname;
            for (; fCurr != fEnd && (*fCurr)->opt.check(FuncOpt::SingleTrace);
                 ++fCurr)
                ;
        }
        else {
            std::cerr
              << "Error, not supported yet. TODO: Implement support for a truly global operation.\n";
            ++fCurr;
        }

        calcFunc(fCurr, fEnd);
    }
    return std::vector<std::string>{};
}

std::vector<std::string> Set::output(std::string oname)
{
    OpOpt opt = {FuncOpt::NeedMeta, FuncOpt::NeedTrcVal, FuncOpt::SingleTrace};
    func.emplace_back(std::make_shared<Op<InPlaceMod>>(
      opt, nullptr, nullptr, [](TraceBlock*) -> std::vector<size_t> {
          return std::vector<size_t>{};
      }));

    outfix                       = oname;
    std::vector<std::string> out = calcFunc(func.begin(), func.end());
    func.clear();

    return out;
}

void Set::getMinMax(
  MinMaxFunc<File::Param> xlam, MinMaxFunc<File::Param> ylam, CoordElem* minmax)
{
    // TODO: This needs to be changed to be compatible with ExSeisFlow
    minmax[0].val = std::numeric_limits<geom_t>::max();
    minmax[1].val = std::numeric_limits<geom_t>::min();
    minmax[2].val = std::numeric_limits<geom_t>::max();
    minmax[3].val = std::numeric_limits<geom_t>::min();
    for (size_t i = 0; i < 4; i++)
        minmax[i].num = std::numeric_limits<size_t>::max();

    CoordElem tminmax[4LU];

    for (auto& f : file) {
        std::vector<File::Param> vprm;
        File::Param prm(rule, f->ilst.size());

        f->ifc->readParamNonContiguous(f->ilst.size(), f->ilst.data(), &prm);

        for (size_t i = 0; i < f->ilst.size(); i++) {
            vprm.emplace_back(rule, 1LU);
            cpyPrm(i, &prm, 0, &vprm.back());
        }
        // TODO: Minmax can't assume ordered data! Fix this!
        size_t offset = piol->comm->offset(f->ilst.size());
        File::getMinMax(
          piol.get(), offset, f->ilst.size(), vprm.data(), xlam, ylam, tminmax);
        for (size_t i = 0LU; i < 2LU; i++) {
            updateElem<std::less<geom_t>>(&tminmax[2LU * i], &minmax[2LU * i]);
            updateElem<std::greater<geom_t>>(
              &tminmax[2LU * i + 1LU], &minmax[2LU * i + 1LU]);
        }
    }
}

void Set::sort(CompareP sortFunc)
{
    auto r = std::make_shared<File::Rule>(std::initializer_list<Meta>{
      PIOL_META_il, PIOL_META_xl, PIOL_META_xSrc, PIOL_META_ySrc,
      PIOL_META_xRcv, PIOL_META_yRcv, PIOL_META_xCmp, PIOL_META_yCmp,
      PIOL_META_Offset, PIOL_META_WtrDepRcv, PIOL_META_tn});

    // TODO: This is not the ideal mechanism, hack for now. See the note in the
    //       calcFunc for single traces
    rule->addRule(*r);
    sort(r, sortFunc);
}

void Set::sort(std::shared_ptr<File::Rule> r, CompareP sortFunc)
{
    OpOpt opt = {FuncOpt::NeedMeta, FuncOpt::ModMetaVal, FuncOpt::DepMetaVal,
                 FuncOpt::SubSetOnly};

    func.push_back(std::make_shared<Op<InPlaceMod>>(
      opt, r, nullptr, [this, sortFunc](TraceBlock* in) -> std::vector<size_t> {
          // TODO: It will eventually be necessary to support this use case.
          if (piol->comm->min(in->prm->size()) < 3LU) {
              piol->log->record(
                "", Log::Layer::Set, Log::Status::Error,
                "Email cathal@ichec.ie if you want to sort -very- small sets of files with multiple processes.",
                PIOL_VERBOSITY_NONE);
              return std::vector<size_t>{};
          }
          else {
              return File::sort(piol.get(), in->prm.get(), sortFunc);
          }
      }));
}

void Set::toAngle(
  std::string vmName, const size_t vBin, const size_t oGSz, geom_t oInc)
{
    OpOpt opt  = {FuncOpt::NeedMeta,   FuncOpt::NeedTrcVal, FuncOpt::ModTrcVal,
                 FuncOpt::ModMetaVal, FuncOpt::DepTrcVal,  FuncOpt::DepTrcOrder,
                 FuncOpt::DepTrcCnt,  FuncOpt::DepMetaVal, FuncOpt::Gather};
    auto state = std::make_shared<RadonState>(piol, vmName, vBin, oGSz, oInc);
    func.emplace_back(std::make_shared<Op<Mod>>(
      opt, rule, state, [state](const TraceBlock* in, TraceBlock* out) {
          const size_t iGSz = in->prm->size();
          out->ns           = in->ns;
          out->inc          = state->oInc;  // 1 degree in radians
          out->trc.resize(state->oGSz * out->ns);
          out->prm.reset(new File::Param(in->prm->r, state->oGSz));
          if (!in->prm->size()) return;

          // For each angle in the angle gather
          for (size_t j = 0; j < state->oGSz; j++) {
              // For each sample (angle + radon)
              for (size_t z = 0; z < in->ns; z++) {
                  // We are using coordinate level accuracy when its not
                  // performance critical.
                  geom_t vmModel =
                    state->vtrc
                      [in->gNum * state->vNs
                       + std::min(
                           size_t(geom_t(z * in->inc) / state->vInc),
                           state->vNs)];
                  llint k =
                    llround(vmModel / cos(geom_t(j * out->inc))) / state->vBin;
                  if (k > 0 && size_t(k) < iGSz)
                      out->trc[j * out->ns + z] = in->trc[k * in->ns + z];
              }
          }

          for (size_t j = 0; j < state->oGSz; j++) {
              // TODO: Set the rest of the parameters
              // TODO: Check the get numbers
              File::setPrm(
                j, PIOL_META_il, state->il[in->gNum], out->prm.get());
              File::setPrm(
                j, PIOL_META_xl, state->xl[in->gNum], out->prm.get());
          }
      }));
}


void Set::taper(TaperFunc tapFunc, size_t nTailLft, size_t nTailRt)
{
    OpOpt opt = {FuncOpt::NeedTrcVal, FuncOpt::ModTrcVal, FuncOpt::DepTrcVal,
                 FuncOpt::SingleTrace};
    func.emplace_back(std::make_shared<Op<InPlaceMod>>(
      opt, rule, nullptr,
      [tapFunc, nTailLft, nTailRt](TraceBlock* in) -> std::vector<size_t> {
          File::taper(
            in->prm->size(), in->ns, in->trc.data(), tapFunc, nTailLft,
            nTailRt);
          return std::vector<size_t>{};
      }));
}

void Set::AGC(AGCFunc agcFunc, size_t window, trace_t normR)
{
    OpOpt opt = {FuncOpt::NeedTrcVal, FuncOpt::ModTrcVal, FuncOpt::DepTrcVal,
                 FuncOpt::SingleTrace};
    func.emplace_back(std::make_shared<Op<InPlaceMod>>(
      opt, rule, nullptr,
      [agcFunc, window, normR](TraceBlock* in) -> std::vector<size_t> {
          File::AGC(
            in->prm->size(), in->ns, in->trc.data(), agcFunc, window, normR);
          return std::vector<size_t>{};
      }));
}

void Set::text(std::string outmsg_)
{
    outmsg = outmsg_;
}

/********************************** Non-Core **********************************/
void Set::sort(SortType type)
{
    Set::sort(File::getComp(type));
}

void Set::getMinMax(Meta m1, Meta m2, CoordElem* minmax)
{
    bool m1Add = rule->addRule(m1);
    bool m2Add = rule->addRule(m2);

    Set::getMinMax(
      [m1](const File::Param& a) -> geom_t {
          return File::getPrm<geom_t>(0LU, m1, &a);
      },
      [m2](const File::Param& a) -> geom_t {
          return File::getPrm<geom_t>(0LU, m2, &a);
      },
      minmax);

    if (m1Add) rule->rmRule(m1);
    if (m2Add) rule->rmRule(m2);
}

void Set::taper(TaperType type, size_t nTailLft, size_t nTailRt)
{
    Set::taper(File::getTap(type), nTailLft, nTailRt);
}

void Set::AGC(AGCType type, size_t window, trace_t normR)
{
    Set::AGC(File::getAGCFunc(type), window, normR);
}

void Set::temporalFilter(
  FltrType type,
  FltrDmn domain,
  PadType pad,
  trace_t fs,
  std::vector<trace_t> corners,
  size_t nw,
  size_t winCntr)
{
    assert(corners.size() == 2);
    OpOpt opt = {FuncOpt::NeedTrcVal, FuncOpt::ModTrcVal, FuncOpt::DepTrcVal,
                 FuncOpt::SingleTrace};
    func.emplace_back(std::make_shared<Op<InPlaceMod>>(
      opt, rule, nullptr,
      [type, domain, corners, pad, fs, nw,
       winCntr](TraceBlock* in) -> std::vector<size_t> {
          File::temporalFilter(
            in->prm->size(), in->ns, in->trc.data(), fs, type, domain, pad, nw,
            winCntr, corners);
          return std::vector<size_t>{};
      }));
}

void Set::temporalFilter(
  FltrType type,
  FltrDmn domain,
  PadType pad,
  trace_t fs,
  size_t N,
  std::vector<trace_t> corners,
  size_t nw,
  size_t winCntr)
{
    assert(corners.size() == 2);
    OpOpt opt = {FuncOpt::NeedTrcVal, FuncOpt::ModTrcVal, FuncOpt::DepTrcVal,
                 FuncOpt::SingleTrace};
    func.emplace_back(std::make_shared<Op<InPlaceMod>>(
      opt, rule, nullptr,
      [type, domain, corners, pad, fs, nw, winCntr,
       N](TraceBlock* in) -> std::vector<size_t> {
          File::temporalFilter(
            in->prm->size(), in->ns, in->trc.data(), fs, type, domain, pad, nw,
            winCntr, corners, N);
          return std::vector<size_t>{};
      }));
}

}  // namespace PIOL
