#include <cstdint>
#include <streambuf>
#include <string>
#include <sys/types.h>
#include <vector>
#include <array>
#include <memory> // for unique pointers (C++11)
#include <fstream>
#include <iostream>


// Ensemble des types de boite disponibles
enum class BoxType {
    Unknown,
    Root,
    Ftyp,
    Mdat,
    Free,
    Pdin,
    Moov,
    Mvhd,
    Trak,
    Tkhd,
    Edts,
    Elst,
    Mdia,
    Mdhd,
    Hdlr,
    Minf,
    Vmhd,
    Dinf,
    Url,
    Urn,
    Dref,
    Stbl,
    Btrt,
    Stsd,
    
    Meta
};

// Classe abstraite de base pour tous les types de boîtes
class Box {
public:
    BoxType  type = BoxType::Unknown;
    uint64_t size = 0; // Choix de renseigner une taille unique sur 8 octets (pas de largesize)
    
    Box(const Box&) = delete;                // no copy
    Box& operator=(const Box&) = delete;

    Box(Box&&) noexcept = default;           // enable move
    Box& operator=(Box&&) noexcept = default;
    
    // destructor
    virtual ~Box() = default;
    
    // getter/setter
    void    setParseOffset(uint8_t a_parse_offset) { m_parse_offset = a_parse_offset; }
    uint8_t getParseOffset() const { return m_parse_offset; }
    
    Box         *getParent()         { return m_parent; }
    virtual void setParent(Box *a_pParent) = 0;
    
    const std::vector<std::unique_ptr<Box>>& getChildren() const { return m_children; }
    void addChild(std::unique_ptr<Box>& a_pChild)    { m_children.push_back(std::move(a_pChild)); }

    // Parse la boite
    virtual void parse(std::ifstream& a_file) = 0;
    
    // Affiche les informations de la boite
    //     @outstream: flux d'affichage
    virtual void print(std::ostream& a_outstream);

protected:
    uint8_t m_parse_offset = 0; // offset à appliquer pour le parsing, après tous les entêtes des classes héritées
    Box*    m_parent = nullptr; // pointeur vers la boîte parente
    std::vector<std::unique_ptr<Box>> m_children; // vecteur contenant les enfants de la boîte

    
    // constructor
    // explicit Box(std::string type) : _type(std::move(type)) {}
    Box() = default;
    
    // Renseigne le parent de la boîte si celui-ci est valide, lève une exception sinon.
    // L'implementation de cette fonction sert de base pour les classes derivees.
    //     @pParent: pointeur vers la boîte candidat parent
    //     @expectedParentType: le type attendu du parent, à renseigner dans chaque sous-classe
    void setParent(Box *a_pParent, BoxType a_expectedParentType);
    void setParent(Box *a_pParent, std::vector<BoxType> a_expectedParentType);
};

// Classe de base etendue
class FullBox : public Box {
public:
    uint8_t version = 0;
    std::array<char, 3> flags = {0, 0, 0};
    
    void setFlags(std::array<char, 3> a_flags) {
        flags[0] = a_flags[0];
        flags[1] = a_flags[1];
        flags[2] = a_flags[2];
    }

    virtual void print(std::ostream& a_outstream) override;
    virtual void parse(std::ifstream& a_file) override;
};

// Boite racine, sans informations particulières
class Root final : public Box {
public:
    Root() {
        type = BoxType::Root;
    }
    
    void setParent(Box *pParent) override final;

    // Parse tout le fichier
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
}; 

class Ftyp final : public Box {
public:
    std::array<char, 4> major_brand;
    uint32_t            minor_version;
    std::vector<std::array<char, 4>> compatible_brands;

    Ftyp() {
        type = BoxType::Ftyp;
    }
    
    void setParent(Box *a_parent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Mdat final : public Box {
public:
    uint64_t beg_data; // index de début des données images/audio dans le bitstream

    Mdat() {
        type = BoxType::Mdat;
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte : avance le bitstream jusqu'à la prochaine boîte et stocke
    // le début des données. La fin de la boîte est connue grâce à sa taille.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Boite pour l'alignement (padding)
class Free final : public Box {
public:
    Free() {
        type = BoxType::Free;
    }
    
    void setParent(Box *pParent) override final;
    
    // Saute la boîte entièrement
    void parse(std::ifstream& a_file) override final;
};

class Pdin final : public FullBox {
public:
    std::vector<uint32_t> rate;
    std::vector<uint32_t> initial_delay;

    Pdin() {
        type = BoxType::Pdin;
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream &a_file) override final;
};

class Moov final : public Box {
public:
    Moov() {
        type = BoxType::Moov;
    }
    
    void setParent(Box *a_pParent) override final;
    
    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Moov header
class Mvhd final : public FullBox {
public:
    Mvhd() {
        type = BoxType::Mvhd;
        flags = {0, 0, 0};
    }
    
    // On utilise pour des uint64 pour le stockage peu importe la version
    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t timescale;
    uint64_t duration;
    uint32_t rate      = 0x00010000;
    uint16_t volume    = 0x0100;
    std::array<int32_t, 9> matrix = {0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x40000000}; // Matrice unité
    uint32_t next_track_ID;

    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Trak final : public Box {
public:
    Trak() {
        type = BoxType::Trak;
    }
    
    void setParent(Box *pParent) override final;
    
    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Trak header
class Tkhd final : public FullBox {
public:
    Tkhd() {
        type = BoxType::Tkhd;
    }
    
    // On utilise des uint64 peu importe la version
    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t track_ID;
    uint64_t duration;
    int16_t  layer = 0;
    int16_t  alternate_group = 0;
    int16_t  volume;
    std::array<int32_t, 9> matrix = {0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x40000000}; // Matrice unité
    uint32_t width;
    uint32_t height;

    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outfile) override final;

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Edts final : public Box {
public:
    Edts() {
        type = BoxType::Edts;
    }
    
    void setParent(Box *pParent) override final;

    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Timeline map
class Elst final : public FullBox {
public:
    Elst() {
        type = BoxType::Elst;
    }

    uint32_t entry_count;
    std::vector<uint64_t> segment_duration;
    std::vector<int64_t> media_time;
    std::vector<int16_t> media_rate_integer;
    std::vector<int16_t> media_rate_fraction;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Contains all the objects that declare information about the media data within a track.
class Mdia final : public Box {
public:
    Mdia() {
        type = BoxType::Mdia;
    }
    
    void setParent(Box *pParent) override final;

    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Mdia header
class Mdhd final : public FullBox {
public:
    Mdhd() {
        type = BoxType::Mdhd;
    }

    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t timescale;
    uint64_t duration;
    uint16_t language; // (5 bin)[3], le premier? bit est inutilisé
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Hdlr final : public FullBox {
public:
    Hdlr() {
        type = BoxType::Hdlr;
    }

    uint32_t handler_type;
    std::string name;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Minf final : public Box {
public:
    Minf() {
        type = BoxType::Minf;
    }
    
    void setParent(Box *pParent) override final;

    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Video media header
class Vmhd final : public FullBox {
public:
    Vmhd() {
        type = BoxType::Vmhd;
        version = 1;
        flags[0] = 1;
    }

    uint16_t graphicsmode = 0;
    std::array<uint16_t, 3> opcolor = {0, 0, 0};
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Contains objects that declare the location of the media information in a track.
class Dinf final : public Box {
public:
    Dinf() {
        type = BoxType::Dinf;
    }
    
    void setParent(Box *pParent) override final;

    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Url final : public FullBox {
public:
    Url() {
        type = BoxType::Url;
        version = 0;
    }

    std::string location;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Urn final : public FullBox {
public:
    Urn() {
        type = BoxType::Urn;
        version = 0;
    }

    std::string name;
    std::string location;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Dref final : public FullBox {
public:
    Dref() {
        type = BoxType::Dref;
        version = 0;
        flags = {0,0,0};
    }

    uint32_t entry_count;
    // data_entry est ici remplacée par `children`
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override final;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stbl final : public Box {
public:
    Stbl() {
        type = BoxType::Stbl;
    }
    
    void setParent(Box *pParent) override final;
    
    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class SampleEntry : public Box {
public:
    uint16_t data_reference_index;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override;
    
    virtual void parse(std::ifstream& a_file) override;
};

class Btrt final : public Box {
public:
    uint32_t bufferSizeDB;
    uint32_t maxBitrate;
    uint32_t avgBitrate;
    
    Btrt() {
        type = BoxType::Btrt;
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    virtual void parse(std::ifstream& a_file) override;
};

class Stsd final : public FullBox {
public:
    uint32_t entry_count;
    // toutes les instances sont stockées dans `children`

    Stsd() {
        type = BoxType::Stsd;
        flags = {0, 0, 0};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream) override;
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    virtual void parse(std::ifstream& a_file) override;
};

class Meta : public FullBox {
public:
    Meta() {
        type = BoxType::Meta;
    }
    
    void setParent(Box *pParent) override final;
    void parse(std::ifstream& a_file) override final;
};


class Udta : public Box {}; 

class Iods : public Box {
public:
    // ObjectDecriptor OD;
};

class Moof : public Box {}; 

class Mfra : public Box {}; 

class Stts : public Box {
public:
    uint32_t entry_count;
    std::vector<uint32_t> sample_count;
    std::vector<uint32_t> sample_delta;
};

class Stsc : public Box {
public:
    uint32_t entry_count;
    uint32_t *first_chunk;
    uint32_t *samples_per_chunk;
    uint32_t *samples_description_index;

    void setVars(uint32_t fc[], uint32_t spc[], uint32_t sdi[]) {
        first_chunk               = new uint32_t[entry_count];
        samples_per_chunk         = new uint32_t[entry_count];
        samples_description_index = new uint32_t[entry_count];
        for (uint32_t i=0; i<entry_count; i++) {
            first_chunk[i]               = fc[i];
            samples_per_chunk[i]         = spc[i];
            samples_description_index[i] = sdi[i];
        }
    }
};

class Stsz : public Box {
public:
    uint32_t sample_size = 0;
    uint32_t sample_count;
    std::vector<uint32_t> entry_size;

    void setSampleSize(uint32_t sizes[]) {
        if (sample_size == 0) {
            entry_size.resize(sample_count);
            for (uint32_t i=0; i<sample_count; i++) {
                entry_size[i] = sizes[i];
            }
        }
    }
    
};



struct TreeBoxDisplay {
    Box* pBox;
    int level;                  // profondeur dans l'arbre
};
