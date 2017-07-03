/*******************************************************************************************//*!
 *   \file
 *   \author Cathal O Broin - cathal@ichec.ie - first commit
 *   \copyright TBD. Do not distribute
 *   \date May 2017
 *   \brief
 *   \details
 *//*******************************************************************************************/
#include "flow/cache.hh"
#include "share/api.hh"
#include "file/dynsegymd.hh"

//TODO: Remove when all options implemented
#include <iostream>

namespace PIOL {
std::shared_ptr<TraceBlock> Cache::getCache(std::shared_ptr<File::Rule> rule, FileDeque & desc, bool cPrm, bool cTrc)
{
    auto it = std::find_if(cache.begin(), cache.end(), [desc] (const CacheElem & elem) -> bool { return elem.desc == desc; });
    if (it == cache.end() || !it->block || !it->block->prm)
    {
        size_t lsnt = 0LU;
        for (auto & f : desc)
            lsnt += f->ilst.size();

        size_t off = piol->comm->offset(lsnt);
        auto prm = std::make_unique<File::Param>(rule, lsnt);
        //TODO: Do not make assumptions about Parameter sizes fitting in memory.
        size_t nt = 0LU;
        size_t loff = 0LU;
        size_t c = 0LU;
        for (auto & f : desc)
        {
            f->ifc->readParam(f->ilst.size(), f->ilst.data(), prm.get(), loff);
            for (size_t i = 0LU; i < f->ilst.size(); i++)
            {
                File::setPrm(loff+i, Meta::gtn, off + loff + f->ilst[i], prm.get());
                File::setPrm(loff+i, Meta::ltn, f->ilst[i] * desc.size() + c, prm.get());
            }
            c++;
            loff += f->ilst.size();
            nt += f->ifc->readNt();
        }

        if (it == cache.end())
        {
            cache.emplace_back(desc, std::move(prm));
            it = cache.end() - 1LU;
        }
        else
        {
            if (!it->block)
                it->block = std::make_shared<TraceBlock>();
            it->block->prm = std::move(prm);
        }
        it->block->nt = nt;
        it->block->ns = desc[0]->ifc->readNs();
        it->block->inc = desc[0]->ifc->readInc();
    }
    else
    {
        //TODO: Check if prm is cached and use Meta::Copy
    }
    return it->block;
}

std::vector<size_t> Cache::getOutputTrace(FileDeque & desc, csize_t offset, csize_t sz, File::Param * prm)
{
    std::vector<size_t> final;

    auto it = std::find_if(cache.begin(), cache.end(), [desc] (const CacheElem & elem) -> bool { return elem.checkPrm(desc); });
    if (it != cache.end())
    {
        auto iprm = it->block->prm.get();
        size_t loc = 0;
        final.resize(sz);
        for (size_t i = 0; i < desc.size() && loc < offset+sz; i++)
        {
            size_t fsz = desc[i]->olst.size();
            size_t nloc = loc + fsz;

            if (nloc > offset)  //Some data should be copied from this file
            {
                size_t lsz = std::min(offset+sz, nloc) - offset;
                size_t foff = offset - loc;
                std::copy(desc[i]->olst.begin() + foff, desc[i]->olst.begin() + foff + lsz, final.begin() + loc - offset);
            }

            loc = nloc;
        }

        std::vector<size_t> sortlist = getSortIndex(sz, final.data());
        for (size_t j = 0U; j < sz; j++)
            File::cpyPrm(sortlist[j], iprm, j, prm);
    }
    return final;
}
}
