/*******************************************************************************************//*!
 *   \file
 *   \author Cathal O Broin - cathal@ichec.ie - first commit
 *   \copyright TBD. Do not distribute
 *   \date July 2016
 *   \brief
 *   \details
 *//*******************************************************************************************/
#include "file/file.hh"
#include "object/objsegy.hh"
#include "share/casts.hh"
namespace PIOL { namespace File {

Interface::Interface(const std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, const std::shared_ptr<Obj::Interface> obj_)
                        : piol(piol_), name(name_), obj(obj_)
{
}

Interface::Interface(const std::shared_ptr<ExSeisPIOL> piol_, const std::string name_, const Obj::Opt & objOpt, const Data::Opt & dataOpt) : piol(piol_), name(name_)
{
    switch (objOpt.getType())
    {
        case (Obj::Type::SEGY) :
        {
            piol->record(name_, Log::Layer::File, Log::Status::Note, "SEGY selected", Log::Verb::Max);
            auto opt = castOptToDeriv<Obj::SEGYOpt, Obj::Opt>(*piol, objOpt, name, Log::Layer::File);
            if (opt == nullptr)
                return;
            auto segy = new Obj::SEGY(piol_, name_, *opt, dataOpt);
            if (segy == nullptr)
                return;
            obj = castToBase<Obj::Interface, Obj::SEGY>(*piol, segy, name, Log::Layer::File);

/*            auto opt = dynamic_cast<Obj::SEGYOpt const *>(&objOpt);
            if (opt == nullptr)
            {
                piol->record(name_, Log::Layer::File, Log::Status::Error, "Obj::SEGY options object is of the wrong type.", Log::Verb::None);
                return;
            }

            auto segy = new Obj::SEGY(piol_, name_, *opt, dataOpt);
            if (segy == nullptr)   //Issue warning on the object layer
            {
                piol->record(name_, Log::Layer::File, Log::Status::Warning, "Could not create object-layer object", Log::Verb::None);
                return;
            }
            else
                obj = std::shared_ptr<Obj::Interface>(std::move(segy));*/
        }
        break;
        default :

        break;
    }
}
}}
