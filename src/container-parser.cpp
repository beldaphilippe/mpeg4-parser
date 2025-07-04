// Implémentation d'un parser de conteneur mp4.
// Construit un arbre dont les noeuds sont les boîtes contenues dans fichier
// .mp4. Les champs de données 'lourds' ne sont pas stockés, on conserve
// seulement leur index de début et de fin dans le bitstream (cf boîte `mdat`).


#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

#include <container-parser.hpp>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <type_traits>


// BoxType boxTypeFromString(const std::string& a_code) {
//     if (a_code == "root") return BoxType::Root; 
//     if (a_code == "ftyp") return BoxType::Ftyp;
//     if (a_code == "mdat") return BoxType::Mdat;
//     if (a_code == "free") return BoxType::Free;
//     if (a_code == "pdin") return BoxType::Pdin;
//     if (a_code == "moov") return BoxType::Moov;
//     if (a_code == "mvhd") return BoxType::Mvhd;
//     if (a_code == "trak") return BoxType::Trak;
//     if (a_code == "tkhd") return BoxType::Tkhd;
//     std::cout << a_code << std::endl;
//     return BoxType::Unknown;
// }

std::string boxTypeToString(BoxType a_type) {
    switch (a_type) {
    case BoxType::Root: return "root"; 
    case BoxType::Ftyp: return "ftyp";
    case BoxType::Mdat: return "mdat";
    case BoxType::Free: return "free";
    case BoxType::Pdin: return "pdin";
    case BoxType::Moov: return "moov";
    case BoxType::Mvhd: return "mvhd";
    case BoxType::Trak: return "trak";
    case BoxType::Tkhd: return "tkhd";
    case BoxType::Edts: return "edts";
    case BoxType::Elst: return "elst";
    case BoxType::Mdia: return "mdia";
    case BoxType::Mdhd: return "mdhd";
    case BoxType::Hdlr: return "hdlr";
    case BoxType::Minf: return "minf";
    case BoxType::Vmhd: return "vmhd";
    case BoxType::Dinf: return "dinf";
    case BoxType::Url:  return "url ";
    case BoxType::Urn:  return "urn ";
    case BoxType::Dref: return "dref";
    case BoxType::Stbl: return "stbl";
    case BoxType::Btrt: return "btrt";
    case BoxType::Stsd: return "stsd";
    case BoxType::Meta: return "meta";
    default: return "unknown";
    }
}

template<typename T>
void readBigEndian(std::istream& a_file, T& a_x) {
    const uint8_t length = sizeof(a_x);
    uint8_t bytes[length];
    a_file.read(reinterpret_cast<char*>(bytes), length);
    a_x = 0;
    for (uint8_t i = 0; i < length; i++) {
        a_x |= T(bytes[i]) << (length-1 - i)*8;
    }
}

// Avance et écrit le bitstream dans `str` tant que le charactère '\0' n'est pas
// rencontré.
//     @file: le bitstream lu
//     @str: référence vers la chaine de sortie
//     @return: le nombre d'octets lus
uint32_t readNullTerminatedString(std::istream& a_file, std::string& a_str) {
    uint32_t cmt = 0;
    a_str = "";
    char buffer = a_file.get();
    cmt ++;
    while (buffer != '\0') {
        if (buffer == EOF) {
            throw std::runtime_error("End of file reached reading a string.");
        }
        a_str += buffer;
        buffer = a_file.get();
        cmt ++;
    }
    return cmt;
}

// Parse le header directement à la position du stream.
//     @file: un pointeur vers le bitstream de lecture
//     @size: pointeur vers la taille de la boîte, à renseigner
//     @type: chaîne de 4 charactères indiquant le type de la boîte, à renseigner 
//     @return: l'offset à appliquer lors du parsing de la boite
// uint parseHeader_bak(std::ifstream& a_file, uint64_t& a_size, BoxType& a_type) {
//     char buffer[8];

//     uint32_t tmp_32;
//     readBigEndian<uint32_t>(a_file, tmp_32);
//     a_size = tmp_32;

//     a_file.read(buffer, 4);
//     a_type = boxTypeFromString(std::string(buffer, 4));
    
//     if (a_size == 1) {          // cas largesize
//         readBigEndian<uint64_t>(a_file, a_size);
//         return 16;
//     }
//     return 8;
// }

std::unique_ptr<Box> makeBoxFromString(std::string a_type) {
    if (a_type == "root") return std::make_unique<Root>();
    if (a_type == "ftyp") return std::make_unique<Ftyp>();
    if (a_type == "mdat") return std::make_unique<Mdat>();
    if (a_type == "free") return std::make_unique<Free>();
    if (a_type == "pdin") return std::make_unique<Pdin>();
    if (a_type == "moov") return std::make_unique<Moov>();
    if (a_type == "mvhd") return std::make_unique<Mvhd>();
    if (a_type == "trak") return std::make_unique<Trak>();
    if (a_type == "tkhd") return std::make_unique<Tkhd>();
    if (a_type == "edts") return std::make_unique<Edts>();
    if (a_type == "elst") return std::make_unique<Elst>();
    if (a_type == "mdia") return std::make_unique<Mdia>();
    if (a_type == "mdhd") return std::make_unique<Mdhd>();
    if (a_type == "hdlr") return std::make_unique<Hdlr>();
    if (a_type == "minf") return std::make_unique<Minf>();
    if (a_type == "vmhd") return std::make_unique<Vmhd>();
    if (a_type == "dinf") return std::make_unique<Dinf>();
    if (a_type == "dref") return std::make_unique<Dref>();
    if (a_type == "stbl") return std::make_unique<Stbl>();
    if (a_type == "url ") return std::make_unique<Url>();
    if (a_type == "urn ") return std::make_unique<Urn>();
    if (a_type == "stsd") return std::make_unique<Stsd>();
    if (a_type == "meta") return std::make_unique<Meta>();
    
        //     pChildBox = new Mdat;
        // } else if (type =="avc1")) {
        //     pChildBox = new Mdat;
        // } else if (type =="stts")) {
        //     pChildBox = new Mdat;
        // } else if (type =="stss")) {
        //     pChildBox = new Mdat;
        // } else if (type =="stsc")) {
        //     pChildBox = new Mdat;
        // } else if (type =="stsz")) {
        //     pChildBox = new Mdat;
        // } else if (type =="stco")) {
        //     pChildBox = new Mdat;
    char err_msg[35];
    std::sprintf(err_msg, "Box of type `%s` not supported.", a_type.c_str());
    throw std::runtime_error(err_msg);
}

// Parse le header directement à la position du stream.
//     @file: un pointeur vers le bitstream de lecture
//     @return: la boite du type lu
std::unique_ptr<Box> parseHeader(std::ifstream& a_file) {
    char buffer[4];
    
    // size
    uint32_t size;
    readBigEndian<uint32_t>(a_file, size);

    // type
    a_file.read(buffer, 4);
    std::unique_ptr<Box> box;
    try { // debug
        box = makeBoxFromString(std::string(buffer, 4));
    } catch (std::runtime_error e) {
        std::cout << "Error: " << e.what() << std::endl
                  << "pos: "   << std::hex << a_file.tellg() << std::endl;
        throw;
    }
    
    box->size = size;

    // largesize
    uint8_t parse_offset = 8;
    if (size == 1) {
        readBigEndian<uint64_t>(a_file, box->size);
        parse_offset += 8;
    }
    box->setParseOffset(parse_offset);

    std::cout << "-- header parsed\n"; // debug
    return box;
}


// Parse la boîte à la position du bitstream.
//     @file: un pointeur vers le bitstream de lecture
//     @box:  la boîte analysée, parente des boîtes suivantes
//     @box_size: taille de la boîte
//     @return: pas de valeur de sortie
void parseBox(std::ifstream& a_file, Box& a_box) {
    // position du début de la boîte, après l'entête
    uint64_t beg_box = (uint64_t) a_file.tellg();
    uint64_t end_box;
    if (a_box.size != 0) {
        // position de la fin de la boîte
        end_box = beg_box + a_box.size - a_box.getParseOffset(); 
    } else {                             // cas de lecture jusqu'à la fin du fichier
        end_box = (uint64_t) -1;         // max uint64
    }
    std::unique_ptr<Box> child_box;
    while ( (uint64_t) a_file.tellg() < end_box && a_file.peek() != EOF) { // 2e condition pour le cas box_size = 0
        child_box = parseHeader(a_file);
        child_box->setParent(&a_box);
        
        Box* raw_ptr = child_box.get();
        a_box.addChild(child_box);
        raw_ptr->parse(a_file);
        raw_ptr->print(std::cout); // debug
    }
}

void Box::setParent(Box* a_parent, const BoxType a_expected_parent_type) {
    if (a_parent->type == a_expected_parent_type) {
        m_parent = a_parent;
    } else {
        char err_msg[50];
        std::sprintf(err_msg, "`%s` box parent should be `%s`, not `%s`",
                     boxTypeToString(type).c_str(),
                     boxTypeToString(a_expected_parent_type).c_str(),
                     boxTypeToString(a_parent->type).c_str());
        throw std::runtime_error(err_msg);
    }
}
void Box::setParent(Box* a_parent, const std::vector<BoxType> a_expected_parent_type) {
    BoxType parent_type = a_parent->type;
    for (auto expected_type : a_expected_parent_type) {
        if ( parent_type == expected_type) {
            m_parent = a_parent;
            return;
        } 
    }
    char err_msg[50];
    std::sprintf(err_msg, "`%s` box parent should not be `%s`",
                 boxTypeToString(type).c_str(),
                 boxTypeToString(a_parent->type).c_str());
    throw std::runtime_error(err_msg);
}

void Box::print(std::ostream& a_outstream) { 
    a_outstream << "type: "
                << boxTypeToString(type)
                << "\nsize: "
                << size
                << std::endl;
}

void FullBox::parse(std::ifstream& a_file) {
    // version
    readBigEndian<uint8_t>(a_file, version);
    // flags
    a_file.read(flags.data()+2, 1);
    a_file.read(flags.data()+1, 1);
    a_file.read(flags.data(), 1);

    m_parse_offset += 4;
}
void FullBox::print(std::ostream& a_outstream) {
    Box::print(a_outstream);
    a_outstream << "version: " << (int) version << std::endl
                << "flags: ";
    for (uint8_t i = 0; i < 3; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            a_outstream << (flags[i] & (1 << j));
        }
        a_outstream << ' ';
    }
    a_outstream << std::endl;
}

void Root::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Root::setParent(Box* a_parent) {
    (void) a_parent;
    throw std::runtime_error("Root object cannot have a parent.");
}

void Ftyp::parse(std::ifstream& a_file) {
    a_file.read(major_brand.data(), 4);

    readBigEndian<uint32_t>(a_file, minor_version);
    
    std::array<char, 4> brand;
    if (size == 0) {           // on lit jusqu'à la fin du fichier
        while (a_file.peek() != EOF) {
            a_file.read(brand.data(), 4);
            compatible_brands.push_back(brand);
        }
    } else { 
        for (uint64_t i=m_parse_offset+8; i<size; i+=4) {
            a_file.read(brand.data(), 4);
            compatible_brands.push_back(brand);
        }
    }
}
void Ftyp::print(std::ostream& a_outstream) {
    Box::print(a_outstream);
    a_outstream << "major brand: " << std::string(major_brand.data(), 4) << std::endl
                << "minor version: " << (int)minor_version << std::endl
                << "compatible brands: " << std::endl;
    for (int i=0; i < (int) compatible_brands.size(); i++) {
        a_outstream << '\t';
        a_outstream.write(compatible_brands[i].data(), 4);
        a_outstream << std::endl;
    }
    a_outstream << std::endl;
}
void Ftyp::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Root);
}

void Mdat::parse(std::ifstream& a_file) {
    beg_data = a_file.tellg();
    if (size == 0) {           // on lit jusqu'à la fin du fichier
        a_file.seekg(0, a_file.end);
    } else {
        a_file.seekg((uint64_t) a_file.tellg() + size - m_parse_offset);
    }
}
void Mdat::print(std::ostream& a_outstream) {
    Box::print(a_outstream);
    a_outstream << "beginning of data: " << beg_data
                << std::endl
                << std::endl;
}
void Mdat::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Root);
}

void Free::parse(std::ifstream& a_file) {
    a_file.seekg( (uint64_t) a_file.tellg() + size - m_parse_offset);
}
void Free::setParent(Box* a_parent) {
    m_parent = a_parent;
}

void Pdin::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    uint32_t buffer;

    for (uint64_t i=m_parse_offset; i<size; i+=8) {
        readBigEndian<uint32_t>(a_file, buffer);
        rate.push_back(buffer);
        
        readBigEndian<uint32_t>(a_file, buffer);
        initial_delay.push_back(buffer);
    }
}
void Pdin::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "rate\tinitial delay:" << std::endl;
    for (int i = 0; i < (int) rate.size(); i++) {
        a_outstream << rate[i]
                    << ' '
                    << initial_delay[i]
                    << std::endl;
    }
    a_outstream << std::endl;
}
void Pdin::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Root);
}

void Moov::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Moov::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Root);
}

void Mvhd::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    if (version == 1) {
        // creation_time
        readBigEndian<uint64_t>(a_file, creation_time);
        // modification_time
        readBigEndian<uint64_t>(a_file, modification_time);
        // timescale
        readBigEndian<uint32_t>(a_file, timescale);
        // duration
        readBigEndian<uint64_t>(a_file, duration);
    } else if (version == 0) {
        uint32_t tmp_32;
        // creation_time
        readBigEndian<uint32_t>(a_file, tmp_32);
        creation_time = tmp_32;
        // modification_time
        readBigEndian<uint32_t>(a_file, tmp_32);
        modification_time = tmp_32;
        // timescale
        readBigEndian<uint32_t>(a_file, timescale);
        // duration
        readBigEndian<uint32_t>(a_file, tmp_32);
        duration = tmp_32;
    } else {
        throw std::runtime_error("Mvhd version must be 0 or 1.");
    }
    // rate
    readBigEndian<uint32_t>(a_file, rate);
    // volume
    readBigEndian<uint16_t>(a_file, volume);
    // reserved (2 octets)
    a_file.seekg( (uint64_t) a_file.tellg() + 2);
    // reserved ( (4 octets)[2] )
    a_file.seekg( (uint64_t) a_file.tellg() + 8);
    // matrix
    for (int i=0; i<9; i++) {
        readBigEndian<int32_t>(a_file, matrix[i]);
    }
    // pre_defined ( (4 octets)[6] )
    a_file.seekg( (uint64_t) a_file.tellg() + 24);
    // next_track_ID
    readBigEndian<uint32_t>(a_file, next_track_ID);
}
void Mvhd::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "creation time: " << creation_time << std::endl
                << "timescale: "     << timescale << std::endl
                << "duration: "      << duration << std::endl
                << "rate: "          << (int) (rate >> 16) << '.'
                                     << (int) (rate & ((1<<16)-1) ) << std::endl
                << "volume: "        << (int) (volume >> 8) << '.'
                                     << (int) (volume & 255) << std::endl
                << "matrix: "        << std::endl;
    for (uint8_t i = 0; i < 3; i++) {
        a_outstream << '\t';
        for (uint8_t j = 0; j < 3; j++) {
            a_outstream << matrix[3*i+j] << ' ';
        }
        a_outstream << std::endl;
    }
    a_outstream << "next track ID: " << next_track_ID << std::endl
                << std::endl;
}
void Mvhd::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Moov );
}

void Trak::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Trak::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Moov);
}

void Tkhd::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);    
    if (version == 1) {
        // creation_time
        readBigEndian<uint64_t>(a_file, creation_time);
        // modification_time
        readBigEndian<uint64_t>(a_file, modification_time);
        // track_ID
        readBigEndian<uint32_t>(a_file, track_ID);
        // reserved (4 octets)
        a_file.seekg( (uint64_t) a_file.tellg() + 4);
        // duration
        readBigEndian<uint64_t>(a_file, duration);
    } else if (version == 0) {
        uint32_t tmp_32;
        // creation_time
        readBigEndian<uint32_t>(a_file, tmp_32);
        creation_time = tmp_32;
        // modification_time
        readBigEndian<uint32_t>(a_file, tmp_32);
        modification_time = tmp_32;
        // track_ID
        readBigEndian<uint32_t>(a_file, track_ID);
        // reserved (4 octets)
        a_file.seekg( (uint64_t) a_file.tellg() + 4);
        // duration
        readBigEndian<uint32_t>(a_file, tmp_32);
        duration = tmp_32;
    } else {
        throw std::runtime_error("Mvhd version must be 0 or 1.");
    }
    // reserved ((4 octets)[2])
    a_file.seekg( (uint64_t) a_file.tellg() + 8);
    // layer
    readBigEndian<int16_t>(a_file, layer);
    // alternate_group
    readBigEndian<int16_t>(a_file, alternate_group);
    // volume
    readBigEndian<int16_t>(a_file, volume);
    // reserved (2 octets)
    a_file.seekg( (uint64_t) a_file.tellg() + 2);
    // matrix
    for (int i=0; i<9; i++) {
        readBigEndian<int32_t>(a_file, matrix[i]);
    }
    // width
    readBigEndian<uint32_t>(a_file, width);
    // height
    readBigEndian<uint32_t>(a_file, height);
}
void Tkhd::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "creation time: "     << creation_time << std::endl
                << "modification time: " << modification_time << std::endl
                << "track ID: "          << track_ID << std::endl
                << "duration: "          << duration << std::endl
                << "layer: "             << layer << std::endl
                << "alternate group: "   << alternate_group << std::endl
                << "volume: "            << (int) (volume >> 8) << '.'
                                         << (int) (volume & 255) << std::endl
                << "matrix: "            << std::endl;
    for (uint8_t i = 0; i < 3; i++) {
        a_outstream << '\t';
        for (uint8_t j = 0; j < 3; j++) {
            a_outstream << matrix[3*i+j] << ' ';
        }
        a_outstream << std::endl;
    }
    a_outstream << "width: "  << width << std::endl
                << "height: " << height << std::endl
                << std::endl;
}
void Tkhd::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Trak);
}

void Edts::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Edts::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Trak);
}

void Elst::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    
    // entry_count
    readBigEndian<uint32_t>(a_file, entry_count);
    if (version == 1) {
        uint64_t ubuffer;
        int64_t  sbuffer;
        for (uint32_t i=0; i<entry_count; i++) {
            // segment_duration
            readBigEndian<uint64_t>(a_file, ubuffer);
            segment_duration.push_back(ubuffer);
            // media_time
            readBigEndian<int64_t>(a_file, sbuffer);
            media_time.push_back(sbuffer);
        }
    } else if (version == 0) {
        uint32_t ubuffer;
        int32_t  sbuffer;
        int16_t  ibuffer;
        for (uint32_t i=0; i<entry_count; i++) {
            // segment_duration
            readBigEndian<uint32_t>(a_file, ubuffer);
            segment_duration.push_back(ubuffer);
            // media_time
            readBigEndian<int32_t>(a_file, sbuffer);
            media_time.push_back(sbuffer);
            // media_rate_integer
            readBigEndian<int16_t>(a_file, ibuffer);
            media_rate_integer.push_back(ibuffer);
            // media_rate_fraction
            readBigEndian<int16_t>(a_file, ibuffer);
            media_rate_fraction.push_back(ibuffer);
        }
    } else {
        throw std::runtime_error("FullBox version must be 0 or 1.");
    }
}
void Elst::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "entry count: " << entry_count << std::endl
                << "segment duration: ";
    for (uint32_t i=0; i<entry_count; i++) {
        a_outstream << segment_duration[i] << ' ';
    }
    a_outstream << "\nmedia time: ";
    for (uint32_t i=0; i<entry_count; i++) {
        a_outstream << media_time[i] << ' ';
    }
    a_outstream << "\nmedia rate: ";
    for (uint32_t i=0; i<entry_count; i++) {
        a_outstream << media_rate_integer[i] << '.'
                    << media_rate_fraction[i] << ' ';
    }
    a_outstream << '\n';
}
void Elst::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Edts);
}

void Mdia::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Mdia::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Trak);
}

void Mdhd::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    
    if (version == 1) {
        // creation_time
        readBigEndian<uint64_t>(a_file, creation_time);
        // modification_time
        readBigEndian<uint64_t>(a_file, modification_time);
        // timescale
        readBigEndian<uint32_t>(a_file, timescale);
        // duration
        readBigEndian<uint64_t>(a_file, duration);
    } else if (version == 0) {
        uint32_t buffer;
        // creation_time
        readBigEndian<uint32_t>(a_file, buffer);
        creation_time = buffer;
        // modification_time
        readBigEndian<uint32_t>(a_file, buffer);
        modification_time = buffer;
        // timescale
        readBigEndian<uint32_t>(a_file, timescale);
        // duration
        readBigEndian<uint32_t>(a_file, buffer);
        duration = buffer;
    } else {
        throw std::runtime_error("FullBox version must be 0 or 1.");
    }
    // padding (1 octet)
    // a_file.seekg( (uint64_t) a_file.tellg() + 1);
    // language
    readBigEndian<uint16_t>(a_file, language);
    // pre_defined (2 octets)
    a_file.seekg( (uint64_t) a_file.tellg() + 2);
}
void Mdhd::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "creation time: "     << creation_time     << '\n'
                << "modification time: " << modification_time << '\n'
                << "timescale: "         << timescale         << '\n'
                << "duration: "          << duration          << '\n'
                << "language: " << language << '\n';
}
void Mdhd::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Mdia);
}

void Hdlr::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    
    // pre_defined (4 octets)
    a_file.seekg( (uint64_t) a_file.tellg() + 4);
    // handler_type
    readBigEndian<uint32_t>(a_file, handler_type);
    // reserved (4 octets)[3]
    a_file.seekg( (uint64_t) a_file.tellg() + 12);

    m_parse_offset += 20;
    // name
    m_parse_offset += readNullTerminatedString(a_file, name);
    if (m_parse_offset > size) {
        throw std::runtime_error("Overflow box size while reading (hdlr)");
    }
}
void Hdlr::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "handler type: " << handler_type << '\n'
                << "name: "         << name         << '\n';
}
void Hdlr::setParent(Box* a_parent) {
    Box::setParent(a_parent, std::vector<BoxType> {{BoxType::Mdia, BoxType::Meta}});
}

void Minf::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Minf::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Mdia);
}

void Vmhd::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);

    // graphicsmode
    readBigEndian<uint16_t>(a_file, graphicsmode);
    // opcolor
    for (uint8_t i=0; i<3; i++) {
        readBigEndian<uint16_t>(a_file, opcolor[i]);
    }
}
void Vmhd::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "graphics mode: " << graphicsmode << '\n'
                << "opcolor: "       << opcolor[0]
                                     << opcolor[1]
                                     << opcolor[2]   << '\n';
}
void Vmhd::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Minf);
}

void Dinf::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Dinf::setParent(Box* a_parent) {
    Box::setParent(a_parent, {BoxType::Minf, BoxType::Meta});
}

void Url::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    
    // location (optionnel)
    if (flags[0] != 1) {
        m_parse_offset += readNullTerminatedString(a_file, location);
    }
}
void Url::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "location: " << location << '\n';
}
void Url::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Dref);
}

void Urn::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    
    // name
    m_parse_offset += readNullTerminatedString(a_file, name);
    // location (optionnel)
    if (flags[0] != 1) {
        m_parse_offset += readNullTerminatedString(a_file, location);
    }
}
void Urn::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "name: "    << name     << '\n'
                <<"location: " << location << '\n';
}
void Urn::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Dref);
}

void Dref::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    
    // entry_count
    readBigEndian<uint32_t>(a_file, entry_count);
    m_parse_offset += 4;
    // data_entry
    parseBox(a_file, *this);
}
void Dref::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "entry count: " << entry_count << '\n';
}
void Dref::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Dinf);
}

void Stbl::parse(std::ifstream& a_file) {
    parseBox(a_file, *this);
}
void Stbl::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Minf);
}

void SampleEntry::parse(std::ifstream& a_file) {
    // reserved (1 octet)[6]
    a_file.seekg( (uint64_t) a_file.tellg() + 6);
    // data_reference_index
    readBigEndian<uint16_t>(a_file, data_reference_index);
    m_parse_offset += 8;
}
void SampleEntry::print(std::ostream& a_outstream) {
    Box::print(a_outstream);
    a_outstream << "data reference index: " << data_reference_index << '\n';
}

void Btrt::parse(std::ifstream& a_file) {
    // bufferSizeDB
    readBigEndian<uint32_t>(a_file, bufferSizeDB);
    // maxBitrate
    readBigEndian<uint32_t>(a_file, maxBitrate);
    // avgBitrate
    readBigEndian<uint32_t>(a_file, avgBitrate);
}
void Btrt::print(std::ostream& a_outstream) {
    Box::print(a_outstream);
    a_outstream << "bufferSizeDB: " << bufferSizeDB << '\n'
                << "maxBitrate: "   << maxBitrate   << '\n'
                << "avgBitrate: "   << avgBitrate   << '\n';
}
void Btrt::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Minf);
}

void Stsd::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);

    // entry_count
    readBigEndian<uint32_t>(a_file, entry_count);
    m_parse_offset += 4;
    // sample entries
    parseBox(a_file, *this);
}
void Stsd::print(std::ostream& a_outstream) {
    FullBox::print(a_outstream);
    a_outstream << "entry count: " << entry_count << '\n';
}
void Stsd::setParent(Box* a_parent) {
    Box::setParent(a_parent, BoxType::Stbl);
}

void Meta::parse(std::ifstream& a_file) {
    FullBox::parse(a_file);
    parseBox(a_file, *this);
}
void Meta::setParent(Box *a_parent) {
    Box::setParent(a_parent, std::vector<BoxType> {{BoxType::Moov, BoxType::Trak}});
}


void displayFileTree(Box* pRoot, const std::string fileName) {
    std::vector<TreeBoxDisplay> queue;
    queue.emplace_back(TreeBoxDisplay{pRoot, 0});
    TreeBoxDisplay current;

    std::cout << fileName << std::endl;
    
    while (!queue.empty()) {
        // parcours en profondeur
        current = queue.back();
        queue.pop_back();
        const std::vector<std::unique_ptr<Box>>& children = current.pBox->getChildren();
        for (int i = children.size(); i > 0; i--) {
            queue.push_back(TreeBoxDisplay{children[i-1].get(), current.level + 1});
        }
        // for (const std::unique_ptr<Box>& pChildBox : current.pBox->getChildren()) {
        //     queue.push_back(TreeBoxDisplay{pChildBox.get(), current.level + 1});
        // }
        // affichage graphique sur la sortie standard
        for (int i=0; i<current.level; i++) {
            std::cout << "│   ";
        }
        std::cout << "└───"
                  << boxTypeToString(current.pBox->type)
                  << std::endl;
        current.pBox->print(std::cout);
    }
    std::cout << std::endl;
}

int main() {
    char filepath[] = "test/big_buck_bunny_240p_1mb.mp4";
    // Open the binary file for reading
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for reading.";
        return 1;
    }

    Root root;
    root.size = 0;
    root.parse(file);

    displayFileTree(&root, filepath);

    // Ftyp ftyp_box;
    // uint64_t a;
    // BoxType b;
    // parseHeader(file, a, b);
    // ftyp_box.setSize(a);
    // std::cout << boxTypeToString(b) << std::endl;
    // ftyp_box.parse(file);
    
    // displayFileTree(&ftyp_box, filepath);
    // ftyp_box.print(std::cout);
    return 0;
}
        
