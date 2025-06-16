/* Implémentation d'un parser de conteneur mp4.
 * Construit un arbre dont les noeuds sont les boîtes contenues dans fichier
 * .mp4. Les champs de données 'lourds' ne sont pas stockés, on conserve
 * seulement leur index de début et de fin dans le bitstream (cf boîte `mdat`).
 */

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <container-parser.hpp>
#include <type_traits>

// Remonte une erreur indiquant le type de boîte attendu et le type de boîte
// obtenu
//     @pBox: la boîte qui est issu d'un parent innatendu
//     @parentType: le type attendu du parent
void errWrongParentBox(Box *pBox, char *parentType) {
    char err_msg[50];
    std::sprintf(err_msg, "`%s` box parent should be `%s`, not %s", pBox->getType(), parentType, pBox->getParent()->getType());
    throw err_msg;
}

int16_t  str2int16(char *str) {
    char str_cpy[3];
    for (int i=0; i<2; i++) {
        str_cpy[i] = str[i];
    }
    str_cpy[2] = 0;
    return (int16_t) std::stoi(str_cpy, 0, 256);
}
uint16_t str2uint16(char *str) {
    uint16_t result;
    std::memcpy(&result, str, sizeof(uint16_t));
    return result;
}
int32_t  str2int32(char *str) {
    char str_cpy[5];
    for (int i=0; i<4; i++) {
        str_cpy[i] = str[i];
    }
    str_cpy[4] = 0;
    return (int32_t) std::stoi(str_cpy, 0, 256);
}
uint32_t str2uint32(char *str) {
    uint32_t result;
    std::memcpy(&result, str, sizeof(uint32_t));
    return result;
}
int64_t  str2int64(char *str) {
    char str_cpy[9];
    for (int i=0; i<8; i++) {
        str_cpy[i] = str[i];
    }
    str_cpy[8] = 0;
    return (int64_t) std::stoi(str_cpy, 0, 256);
}
uint64_t str2uint64(char *str) {
    uint64_t result;
    std::memcpy(&result, str, sizeof(uint64_t));
    return result;
}

// Parse le header directement à la position du stream.
//     @pFile: un pointeur vers le bitstream de lecture
//     @pSize: pointeur vers la taille de la boîte, à renseigner
//     @type:  chaîne de 4 charactères indiquant le type de la boîte, à renseigner 
//     @return: pas de valeur de sortie
void parseHeader(std::ifstream *pFile, uint64_t *pSize, char type[4]) {
    char buffer[8];
    pFile->read(buffer, 4);
    *pSize = str2uint32(buffer);
    
    pFile->read(type, 4);
    
    if (*pSize == 1) {          // cas largesize
        pFile->read(buffer, 8);
        *pSize = str2uint64(buffer) - 16; // taille de la boîte, sans header
    } else if (*pSize != 0) {
        *pSize -= 8;            // taille de la boîte, sans header
    }
}

// Parse la boîte à la position du bitstream.
//     @pFile: un pointeur vers le bitstream de lecture
//     @pBox:  la boîte analysée, parente des boîtes suivantes
//     @box_size: taille de la boîte
//     @return: pas de valeur de sortie
void parseBox(std::ifstream *pFile, Box *pBox, uint64_t box_size) {
    uint64_t beg_box = (uint64_t) pFile->tellg(); // position du début de la boîte, après l'entête
    uint64_t end_box;
    if (box_size != 0) {
        end_box = beg_box + box_size ; // position de la fin de la boîte
    } else {                           // cas de lecture jusqu'à la fin du fichier
        end_box = (uint64_t) -1;       // max uint64
    }
    uint64_t size;
    char type[5];
    Box *pChildBox;
    while ( (uint64_t) pFile->tellg() < end_box && pFile->good()) { // 2e condition pour le cas box_size = 0
        parseHeader(pFile, &size, type);
        type[4] = '\0'; // pour la comparaison avec la string constante
        if (std::strcmp(type, "ftyp")) {
            pChildBox = new Ftyp;
        } else if (std::strcmp(type, "pdin")) {
            pChildBox = new Pdin;
        } else if (std::strcmp(type, "free")) {
            pChildBox = new Free;
        } else if (std::strcmp(type, "mdat")) {
            pChildBox = new Mdat;
        } else if (std::strcmp(type, "moov")) {
            pChildBox = new Moov;
        } else if (std::strcmp(type, "mvhd")) {
            pChildBox = new Mvhd;
        } else if (std::strcmp(type, "trak")) {
            pChildBox = new Trak;
        } else if (std::strcmp(type, "tkhd")) {
            pChildBox = new Tkhd;
             
        } else {
            return;
        }
        // } else if (std::strcmp(type, "edts")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "mdia")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "mdhd")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "hdlr")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "minf")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "vmhd")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "dinf")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "dref")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stbl")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stcd")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "avc1")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stts")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stss")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stsc")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stsz")) {
        //     pChildBox = new Mdat;
        // } else if (std::strcmp(type, "stco")) {
        //     pChildBox = new Mdat;
        // } else {
        //     char err_msg[50];
        //     std::sprintf(err_msg, "Box of type `%s` not supported.", type);
        //     throw err_msg;
        // }
        pChildBox->setSize(size);
        
        pChildBox->setParent(pBox);
        pBox->addChild(pChildBox);
        pChildBox->parse(pFile);
    }
}


void Box::setParent(Box *pParent, const char expectedParentType[5]) {
    if (strcmp(pParent->getType(), expectedParentType) == 0) {
        _parent = pParent;
    } else {
        char err_msg[50];
        std::sprintf(err_msg, "`%s` box parent should be `%s`, not `%s`", _type, expectedParentType, pParent->getType());
        throw err_msg;
    }
}

void FullBox::parseVF(std::ifstream *pFile) {
    char buffer[4];
    // version
    pFile->read(buffer, 1);
    _version = (uint8_t) buffer[0];
    // flags
    pFile->read(buffer, 3);
    buffer[3] = 0;
    _flags = str2uint32(buffer);
}

void Root::parse(std::ifstream *pFile) {
    parseBox(pFile, this, _size);
}
void Root::setParent(Box *pParent) {
    (void) pParent;
    throw "Root object can not have a parent.";
}

void Ftyp::parse(std::ifstream *pFile) {
    char buffer[4]; // buffer pour la lecture du fichier
    
    pFile->read(major_brand, 4);
    
    pFile->read(buffer, 4);
    minor_version = str2uint32(buffer);

    if (_size == 0) {           // on lit jusqu'à la fin du fichier
        while (pFile->good()) {
            pFile->read(buffer, 4);
            compatible_brands.push_back(buffer);
        }
    } else {    // on utilise _largesize
        for (uint64_t i = 0; 16 + 4 * i < _size; i++) {
            pFile->read(buffer, 4);
            compatible_brands.push_back(std::to_array(buffer));
        }
    }
}
void Ftyp::setParent(Box *pParent) {
    Box::setParent(pParent, "root");
}

void Mdat::parse(std::ifstream *pFile) {
    beg_data = pFile->tellg();
    if (_size == 0) {           // on lit jusqu'à la fin du fichier
        pFile->seekg(0, pFile->end);
    } else {
        pFile->seekg((uint64_t) pFile->tellg() + _size);
    }
}
void Mdat::setParent(Box *pParent) {
    Box::setParent(pParent, "root");
}

void Free::parse(std::ifstream *pFile) {
    pFile->seekg( (uint64_t) pFile->tellg() + _size);
}
void Free::setParent(Box *pParent) {
    _parent = pParent;
}

void Pdin::parse(std::ifstream *pFile) {
    char buffer[4];

    for (uint64_t i=0; i<_size; i+=8) {
        pFile->read(buffer, 4);
        rate.push_back(static_cast<uint32_t>(std::stoi(buffer, 0, 16)));
        
        pFile->read(buffer, 4);
        initial_delay.push_back(static_cast<uint32_t>(std::stoi(buffer, 0, 16)));
    }
}
void Pdin::setParent(Box *pParent) {
    Box::setParent(pParent, "root");
}

void Moov::parse(std::ifstream *pFile) {
    parseBox(pFile, this, _size);
}
void Moov::setParent(Box *pParent) {
    Box::setParent(pParent, "root");
}

// A VERIF (champ pre_defined)
void Mvhd::parse(std::ifstream *pFile) {
    parseVF(pFile);
    
    char buffer[8];
    if (_version == 1) {
        // creation_time
        pFile->read(buffer, 8);
        creation_time = str2uint64(buffer);
        // modification_time
        pFile->read(buffer, 8);
        modification_time = str2uint64(buffer);
        // timescale
        pFile->read(buffer, 4);
        timescale = str2uint32(buffer);
        // duration
        pFile->read(buffer, 8);
        duration = str2uint64(buffer);
    } else if (version == 0) {
        // creation_time
        pFile->read(buffer, 4);
        creation_time = str2uint32(buffer);
        // modification_time
        pFile->read(buffer, 4);
        modification_time = str2uint32(buffer);
        // timescale
        pFile->read(buffer, 4);
        timescale = str2uint32(buffer);
        // duration
        pFile->read(buffer, 4);
        duration = str2uint32(buffer);
    } else {
        throw "Mvhd version must be 0 or 1.";
    }
    // rate
    pFile->read(buffer, 4);
    rate = str2uint32(buffer);
    // volume
    pFile->read(buffer, 2);
    volume = str2uint16(buffer);
    // reserved (2 octets)
    pFile->seekg( (uint64_t) pFile->tellg() + 2);
    // reserved ( (4 octets)[2] )
    pFile->seekg( (uint64_t) pFile->tellg() + 8);
    // matrix
    for (int i=0; i<9; i++) {
        pFile->read(buffer, 4);
        matrix[i] = str2int32(buffer);
    }
    // next_track_ID
    pFile->read(buffer, 4);
    creation_time = str2uint32(buffer);
}
void Mvhd::setParent(Box *pParent) {
    Box::setParent(pParent, "moov");
}

void Trak::parse(std::ifstream *pFile) {
    parseBox(pFile, this, _size);
}
void Trak::setParent(Box *pParent) {
    Box::setParent(pParent, "moov");
}

void Tkhd::parse(std::ifstream *pFile) {
    parseVF(pFile);
    
    char buffer[8];
    if (_version == 1) {
        // creation_time
        pFile->read(buffer, 8);
        creation_time = str2uint64(buffer);
        // modification_time
        pFile->read(buffer, 8);
        modification_time = str2uint64(buffer);
        // track_ID
        pFile->read(buffer, 4);
        track_ID = str2uint32(buffer);
        // reserved (4 octets)
        pFile->seekg( (uint64_t) pFile->tellg() + 4);
        // duration
        pFile->read(buffer, 8);
        duration = str2uint64(buffer);
    } else if (version == 0) {
        // creation_time
        pFile->read(buffer, 4);
        creation_time = str2uint32(buffer);
        // modification_time
        pFile->read(buffer, 4);
        modification_time = str2uint32(buffer);
        // track_ID
        pFile->read(buffer, 4);
        track_ID = str2uint32(buffer);
        // reserved (4 octets)
        pFile->seekg( (uint64_t) pFile->tellg() + 4);
        // duration
        pFile->read(buffer, 4);
        duration = str2uint32(buffer);
    } else {
        throw "Mvhd version must be 0 or 1.";
    }
    // reserved ((4 octets)[2])
    pFile->seekg( (uint64_t) pFile->tellg() + 8);
    // layer
    pFile->read(buffer, 2);
    layer = str2int16(buffer);
    // alternate_group
    pFile->read(buffer, 2);
    alternate_group = str2int16(buffer);
    // volume
    pFile->read(buffer, 2);
    volume = str2int16(buffer);
    // reserved (2 octets)
    pFile->seekg( (uint64_t) pFile->tellg() + 2);
    // matrix
    for (int i=0; i<9; i++) {
        pFile->read(buffer, 4);
        matrix[i] = str2int32(buffer);
    }
    // width
    pFile->read(buffer, 4);
    width = str2uint32(buffer);
    // height
    pFile->read(buffer, 4);
    height = str2uint32(buffer);
}
void Tkhd::setParent(Box *pParent) {
    Box::setParent(pParent, "trak");
}


void displayFileTree(Box *pRoot, char *fileName) {
    std::vector<treeBoxDisplay_s> queue;
    queue.push_back(treeBoxDisplay_s {pRoot, 0});
    treeBoxDisplay_s current;

    std::cout << fileName << std::endl;
    while (!queue.empty()) {
        // parcours en profondeur
        current = queue.back();
        queue.pop_back();
        for (Box *pChildBox : current.pBox->getChild()) {
            queue.push_back(treeBoxDisplay_s {pChildBox, current.level + 1});
        }
        // affichage graphique sur la sortie standard
        for (int i=0; i<current.level; i++) {
            std::cout << "│   ";
        }
        std::cout << "└───" << current.pBox->getType() << std::endl;
    }
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
    root.setSize(0);
    root.parse(&file);

    displayFileTree(&root, filepath);
    
    return 0;
}
