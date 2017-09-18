#include "sglobal.hh"
#include "cppfileapi.hh"
#include "ops/minmax.hh"
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace PIOL;

// Create the initial KML file settings to order to describe the output in general
void initKML(std::ofstream &file, std::string oname, std::string folder)
{
    file.open(oname);
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"\n";
    file << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\"\n";
    file << " xmlns:kml=\"http://www.opengis.net/kml/2.2\"\n";
    file << " xmlns:atom=\"http://www.w3.org/2005/Atom\">\n\n";
    file << "<Document>\n<Folder>\n<name>"+folder+"</name>\n<Style id=\"pathstyle\">\n\t<LineStyle>\n\t\t";
    file << "<color>ff190cff</color>\n\t\t<width>2</width>\n\t</LineStyle>\n</Style>\n";
    file << "<Schema name=\"Intrepid\" id=\"Intrepid\">\n\t<SimpleField name=\"Name\" type=\"string\"></SimpleField>";
    file << "\n\t<SimpleField name=\"Description\" type=\"string\"></SimpleField>\n</Schema>\n\n";
}

std::string highPrecStr(geom_t coord)
{
    return std::to_string(static_cast<long long>(std::floor(coord))) +"."+
    std::to_string(static_cast<long long>(std::floor((coord - std::floor(coord))*10000000000)));
}

// Add a line containing coordinates data to the KML file.
void addLine(std::ofstream &file, std::string name, std::vector<CoordElem> coords)
{
    file << "<Placemark>\n\t<name>"+name+"</name>\n\t<styleUrl>#pathstyle</styleUrl>\n\t<LineString>\n\t\t";
    file << "<ExtendedData>\n\t\t\t<SchemaData schemaUrl=\"#Intrepid\">\n\t\t\t\t";
    file <<  "<SimpleData name=\"Name\">"+name+"</SimpleData>\n\t\t\t</SchemaDat>\n\t\t</ExtendedData>\n\t\t\t";
    file << "<tessellate>1</tessellate>\n\t\t<coordinates>\n\t\t\t"+highPrecStr(coords[2].val)+","+highPrecStr(coords[0].val);
    file << +",0\n\t\t\t"+highPrecStr(coords[3].val)+","+highPrecStr(coords[1].val)+",0\n\t\t </coordinates>\n\t";
    file << "</LineString>\n</Placemark>\n";
}

void closeKML(std::ofstream &file)
{
    file << "</Folder>\n</Document>\n</kml>";
    file.close();
}

// Compute a lat long value from a UTM input
void utm2LatLong(geom_t easting, geom_t northing, std::string utmZone, geom_t  & lat, geom_t & lng)
{
    geom_t hemi = (utmZone.back()="N" ? 1 : -1);
    geom_t numZone = std::stof(utmZone);

    geom_t const eqRad = 6378137;
    geom_t const polRad = 6356752;
    geom_t const k0 = 0.9996;
    geom_t E = std::sqrt(1 - (polRad * polRad) / (eqRad * eqRad));

    //FIXME I guess these formulae come from somewhere? Can you include a link to that so improve the understandability and so they can be checked?
    geom_t ei = (1 - std::sqrt(1 - E*E)) / (1 + std::sqrt(1 - E*E));
    geom_t x = 500000 - easting;
    geom_t mu = (northing / k0) / (eqRad *  (1 - std::pow(E, 2U) / 4 - 3 * std::pow(E, 4U) / 64 - 5 * std::pow(E, 6U) / 256));
    geom_t phi = mu + (3* ei / 2 - 27 * std::pow(ei, 3) / 32) * std::sin(2 * mu) + (21 / 16 * std::pow(ei, 2U) - 55 / 32 *
                std::pow(ei, 4U)) * std::sin(4 * mu) + (151 / 96 * std::pow(ei, 3U) - 417 / 128 *std::pow(ei, 5U)) *
                std::sin(6 * mu) + (1097 / 512 * std::pow(ei, 4U)) *std::sin(8 * mu);
    geom_t c = (E * E / (1 - E * E)) * std::pow(std::cos(phi), 2);
    geom_t d = x / (k0 * eqRad / std::sqrt(1 - std::pow(E * std::sin(phi), 2)));
    lat = (180 * hemi / std::acos(-1)) * (phi - ((1 - std::pow(E * std::sin(phi), 2)) * std::tan(phi) / (1 - E * E)) * (
          (d * d / 2) + ((5 + 3 * std::pow(std::tan(phi), 2) + 10 * c - 4 * c * c - 9 * (E * E / (1 - E * E)))
          * std::pow(d, 4) / 24) + ((61 + 90 * std::pow(std::tan(phi), 2) + 298 * std::pow(std::tan(phi), 4) -
          252 * (E * E / (1 - E * E)) - 3 * c * c) * std::pow(d, 6) / 720)));
    lng = (6 * numZone - 183) - (180 * (d + ((1 + 2 * std::pow(std::tan(phi), 2) + c) * std::pow(d, 3) / 6) +
          ((5 - 2 * c + 28 * std::pow(std::tan(phi), 2) - 3 * c * c + 24 * std::pow(std::tan(phi), 4) + 8 *
          (E * E / (1 - E * E))) * std::pow(d, 5) / 120)) / (std::cos(phi) * std::acos(-1)));
}

/*! Read from the input file. Find the min/max  xSrc, ySrc, xRcv, yRcv, xCmp
 *  and yCMP. Write the matching traces to the output file in that order.
 *  \param[in] iname Input file
 *  \param[in] oname Output file
 */
void calcMin(ExSeis piol, std::string iname, std::vector<CoordElem> & minmax)
{
    File::ReadDirect in(piol, iname);

    auto dec = decompose(piol, in);
    size_t offset = dec.first;
    size_t lnt = dec.second;

    File::Param prm(lnt);
    in.readParam(offset, lnt, &prm);

    File::getMinMax(piol, offset, lnt, Meta::xSrc, Meta::ySrc, &prm, minmax.data());
    File::getMinMax(piol, offset, lnt, Meta::xRcv, Meta::yRcv, &prm, minmax.data()+4U);
};

/* Main function for segy to kml
 *  \param[in] argc The number of input strings.
 *  \param[in] argv The array of input strings.
 *  \return zero on success, non-zero on failure
 *  \details Options:
 *           -i \<file\> : input file name
 *           -o \<file\> : output file name
 *           -f \<folder\> : KML Folder name
 *           -z \<UTMZone\> : UTM Zone if coordinates in UTM
 */
int main(int argc, char ** argv)
{
    std::vector<std::string> iname;
    std::string oname = "";
    std::string utmZone = "";
    std::string folder = "SEG-Y";

    std::string opt = "i:o:f:z:";  //TODO: uses a GNU extension
    for (int c = getopt(argc, argv, opt.c_str()); c != -1; c = getopt(argc, argv, opt.c_str()))
        switch (c)
        {
            case 'i' :
                iname.push_back(optarg);
            break;
            case 'o' :
                oname = optarg;
            break;
            case 'f' :
                folder = optarg;
            break;
            case 'z' :
                utmZone = optarg;
            break;

            return -1;
        }

    if (iname.size()<1 || oname == "")
    {
        std::cerr << "Invalid arguments given.\n";
        std::cerr << "Arguments: -i for input file, -o for output file\n";
        return -1;
    }

    // Expect UTM zone to have the form xS or xN where x is an integer between 1 and 60
    // FIXME I added parentheses for clarity, can you check if you agree?
    if (utmZone != ""  && ((std::toupper(utmZone.back()) != 'N' && std::toupper(utmZone.back()) != 'S') || (std::stof(utmZone) < 1 ||
       std::stof(utmZone) > 60) || std::stof(utmZone) != std::floor(std::stof(utmZone))))
        {
            std::cerr << "Invalid UTM Zone (WGS-84). \n";
            std::cerr << "Zones must an integer between 1 and 60 in hemisphere N or S\n";
            return -1;
        }

    if (std::toupper(iname[0].back()) != 'Y')
    {
        std::ifstream fileLst(iname[0]);
        iname.clear();
        std::string sgynm;
        while (std::getline(fileLst, sgynm))
            iname.push_back(sgynm);
    }
    std::vector<CoordElem> minmax(8U);

    std::ofstream ofile;
    initKML(ofile, oname, folder);
    ExSeis piol;
    for (size_t i = 0; i< iname.size(); i++)
    {
        calcMin(piol, iname[i], minmax);
        // FIXME What happens if minax > 180?
        if (minmax[0].val > 180)
        {
            utm2LatLong(minmax[0].val, minmax[2].val, utmZone, minmax[0].val, minmax[2].val);
            utm2LatLong(minmax[1].val, minmax[3].val, utmZone, minmax[1].val, minmax[3].val);
        }
        addLine(ofile, iname[i], minmax);
    }
    closeKML(ofile);
    return 0;
}


