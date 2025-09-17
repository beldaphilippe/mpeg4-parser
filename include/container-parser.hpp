#include <cstdint>
#include <streambuf>
#include <string>
#include <sys/types.h>
#include <vector>
#include <array>
#include <memory> // for unique pointers (C++11)
#include <fstream>
#include <iostream>


class Box {
public:
    std::array<char, 4>  type = {'u', 'n', 'k', 'n'};
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
    void print(std::ostream& a_outstream);

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
    void setParent(Box *a_pParent, std::array<char, 4> a_expectedParentType);
    void setParent(Box *a_pParent, std::vector<std::array<char, 4>> a_expectedParentType);
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

    void print(std::ostream& a_outstream);
    virtual void parse(std::ifstream& a_file) override;
};

// Boite racine, sans informations particulières
class Root final : public Box {
public:
    Root() {
        type = {'r', 'o', 'o', 't'};
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
        type = {'f', 't', 'y', 'p'};
    }
    
    void setParent(Box *a_parent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Mdat final : public Box {
public:
    uint64_t beg_data; // index de début des données images/audio dans le bitstream

    Mdat() {
        type = {'m', 'd', 'a', 't'};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte : avance le bitstream jusqu'à la prochaine boîte et stocke
    // le début des données. La fin de la boîte est connue grâce à sa taille.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Boite pour l'alignement (padding)
class Free final : public Box {
public:
    Free() {
        type = {'f', 'r', 'e', 'e'};
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
        type = {'p', 'd', 'i', 'n'};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream &a_file) override final;
};

class Moov final : public Box {
public:
    Moov() {
        type = {'m', 'o', 'o', 'v'};
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
        type = {'m', 'v', 'h', 'd'};
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
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Trak final : public Box {
public:
    Trak() {
        type = {'t', 'r', 'a', 'k'};
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
        type = {'t', 'k', 'h', 'd'};
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
    void print(std::ostream& a_outfile);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Edts final : public Box {
public:
    Edts() {
        type = {'e', 'd', 't', 's'};
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
        type = {'e', 'l', 's', 't'};
    }

    uint32_t entry_count;
    std::vector<uint64_t> segment_duration;
    std::vector<int64_t> media_time;
    std::vector<int16_t> media_rate_integer;
    std::vector<int16_t> media_rate_fraction;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Contains all the objects that declare information about the media data within a track.
class Mdia final : public Box {
public:
    Mdia() {
        type = {'m', 'd', 'i', 'a'};
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
        type = {'m', 'd', 'h', 'd'};
    }

    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t timescale;
    uint64_t duration;
    uint16_t language; // (5 bin)[3], le premier? bit est inutilisé
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Hdlr final : public FullBox {
public:
    Hdlr() {
        type = {'h', 'd', 'l', 'r'};
    }

    uint32_t handler_type;
    std::string name;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Minf final : public Box {
public:
    Minf() {
        type = {'m', 'i', 'n', 'f'};
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
        type = {'v', 'm', 'h', 'd'};
        version = 1;
        flags[0] = 1;
    }

    uint16_t graphicsmode = 0;
    std::array<uint16_t, 3> opcolor = {0, 0, 0};
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

// Contains objects that declare the location of the media information in a track.
class Dinf final : public Box {
public:
    Dinf() {
        type = {'d', 'i', 'n', 'f'};
    }
    
    void setParent(Box *pParent) override final;

    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Url final : public FullBox {
public:
    Url() {
        type = {'u', 'r', 'l', ' '};
        version = 0;
    }

    std::string location;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Urn final : public FullBox {
public:
    Urn() {
        type = {'u', 'r', 'n', ' '};
        version = 0;
    }

    std::string name;
    std::string location;
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Dref final : public FullBox {
public:
    Dref() {
        type = {'d', 'r', 'e', 'f'};
        version = 0;
        flags = {0,0,0};
    }

    uint32_t entry_count;
    // data_entry est ici remplacée par `children`
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stbl final : public Box {
public:
    Stbl() {
        type = {'s', 't', 'b', 'l'};
    }
    
    void setParent(Box *pParent) override final;
    
    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class SampleEntry : public Box {
public:
    uint16_t data_reference_index;
    
    void print(std::ostream& a_outstream);
    virtual void parse(std::ifstream& a_file) override;
};

class Btrt final : public Box {
public:
    uint32_t bufferSizeDB;
    uint32_t maxBitrate;
    uint32_t avgBitrate;
    
    Btrt() {
        type = {'b', 't', 'r', 't'};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    virtual void parse(std::ifstream& a_file) override;
};

class Stsd final : public FullBox {
public:
    uint32_t entry_count;
    // toutes les instances sont stockées dans `children`

    Stsd() {
        type = {'s', 't', 's', 'd'};
        flags = {0, 0, 0};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    virtual void parse(std::ifstream& a_file) override;
};

class Meta final : public FullBox {
public:
    Meta() {
        type = {'m', 'e', 't', 'a'};
    }
    
    void setParent(Box *pParent) override final;
    void parse(std::ifstream& a_file) override final;
};

class Frma final : public Box {
public:
    std::array<char, 4> data_format;
    
    Frma() {
        type = {'f', 'r', 'm', 'a'};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    void parse(std::ifstream& a_file) override final;
    
private:
    uint64_t m_beg_data;
};

class Cinf final : public Box {
public:
    // stocké dans m_children
    // OriginalFormatBox(fmt);
    // avcC config;

    Cinf() {
        type = {'c', 'i', 'n', 'f'};
    }
    
    void setParent(Box *pParent) override final;
    void parse(std::ifstream& a_file) override final;
};

class Avcc final : public Box {
public:
    uint64_t beg_data; // index de début des données images/audio dans le bitstream

    Avcc() {
        type = {'a', 'v', 'c', 'c'};
    }
    
    void setParent(Box *pParent) override final;
    
    // Parse la boîte : avance le bitstream jusqu'à la prochaine boîte et stocke
    // le début des données. La fin de la boîte est connue grâce à sa taille.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class VisualSampleEntry : public SampleEntry {
public:
    uint16_t width;
    uint16_t height;
    uint32_t horizresolution = 0x00480000; // 72 dpi
    uint32_t vertresolution  = 0x00480000; // 72 dpi
    uint16_t frame_count = 1;
    std::string compressorname = std::string(32, '\0');
    uint16_t depth = 0x0018;
    // optionnel, contenu dans les enfants
    // CleanApertureBox clap;
    // PixelAspectRatioBox pasp;
    
    void print(std::ostream& a_outstream);
    virtual void parse(std::ifstream& a_file) override;
};

class Icpv final : public VisualSampleEntry {
public:
    // dans les enfants
    // CompleteTrackInfoBox();
    // AVCConfigurationBox config;
    // MPEG4BitRateBox (); // optional
    // MPEG4ExtensionDescriptorsBox (); // optional
    
    std::array<char, 4> transformed_type; // pas à parser, info a transmettre a un enfant
    
    Icpv() {
        type = {'i', 'c', 'p', 'v'};
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);
    
    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stts final : public FullBox {
public:
    uint32_t entry_count;
    std::vector<uint32_t> sample_count;
    std::vector<uint32_t> sample_delta;
    
    Stts() {
        type = {'s', 't', 't', 's'};
        version = 0;
        setFlags({0, 0, 0});
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stss final : public FullBox {
public:
    uint32_t entry_count;
    std::vector<uint32_t> sample_number;
    
    Stss() {
        type = {'s', 't', 's', 's'};
        version = 0;
        setFlags({0, 0, 0});
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stsc final : public FullBox {
public:
    uint32_t entry_count;
    std::vector<uint32_t> first_chunk;
    std::vector<uint32_t> samples_per_chunk;
    std::vector<uint32_t> samples_description_index;
    
    Stsc() {
        type = {'s', 't', 's', 'c'};
        version = 0;
        setFlags({0, 0, 0});
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stsz final : public FullBox {
public:
    uint32_t sample_size;
    uint32_t sample_count;
    std::vector<uint32_t> entry_size;
    
    Stsz() {
        type = {'s', 't', 's', 'z'};
        version = 0;
        setFlags({0, 0, 0});
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Stco final : public FullBox {
public:
    uint32_t entry_count;
    std::vector<uint32_t> chunk_offset;
    
    Stco() {
        type = {'s', 't', 'c', 'o'};
        version = 0;
        setFlags({0, 0, 0});
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Smhd final : public FullBox {
public:
    int16_t balance = 0;
    
    Smhd() {
        type = {'s', 'm', 'h', 'd'};
        version = 0;
        setFlags({0, 0, 0});
    }
    
    void setParent(Box *pParent) override final;
    void print(std::ostream& a_outstream);

    // Parse la boîte actuelle et avance le bitstream jusqu'à la prochaine boîte.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Enca final : public Box {
public:
    uint64_t beg_data; // index de début des données images/audio dans le bitstream

    Enca() {
        type = {'e', 'n', 'c', 'a'};
    }
    
    void setParent(Box *pParent) override final;
    
    // Parse la boîte : avance le bitstream jusqu'à la prochaine boîte et stocke
    // le début des données. La fin de la boîte est connue grâce à sa taille.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Udta final : public Box {
public:
    Udta() {
        type = {'u', 'd', 't', 'a'};
    }
    
    void setParent(Box *a_pParent) override final;
    
    // Avance le bitstream jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};

class Ilst final : public Box {
public:
    uint64_t beg_data; // index de début des données images/audio dans le bitstream

    Ilst() {
        type = {'i', 'l', 's', 't'};
    }
    
    void setParent(Box *pParent) override final;
    
    // Parse la boîte : avance le bitstream jusqu'à la prochaine boîte et stocke
    // le début des données. La fin de la boîte est connue grâce à sa taille.
    //     @file: le bitstream du fichier analysé
    void parse(std::ifstream& a_file) override final;
};


class Iods : public Box {
public:
    // ObjectDecriptor OD;
};

class Moof : public Box {}; 

class Mfra : public Box {}; 




struct TreeBoxDisplay {
    Box* pBox;
    int level;                  // profondeur dans l'arbre
};
