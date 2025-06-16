#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <fstream>
#include <iostream>


// Classe abstraite initiale pour tous les types de boîtes
class Box {
public:
    // constructor
    // explicit Box(std::string type) : _type(std::move(type)) {}
    Box() = default;
    // destructor
    virtual ~Box() = default;
    
    // getter/setter
    std::string& getType()           { return _type; }
    uint64_t     getSize() const     { return _size; }
    void         setSize(uint64_t s) { _size = s; }
    Box         *getParent()         { return _parent; }
    virtual void setParent(Box *pParent) = 0;
    std::vector<std::unique_ptr<Box>> getChild() { return _children; }
    void addChild(std::unique_ptr<Box> pChild)   { _children.push_back(pChild); }

    virtual void parse(std::ifstream *pFile) = 0;

protected:
    // Renseigne le parent de la boîte si celui-ci est valide, lève une exception sinon.
    //     @pParent: pointeur vers la boîte candidat parent
    //     @expectedParentType: le type attendu du parent, à renseigner dans chaque sous-classe
    void setParent(Box *pParent, const char expectedParentType[5]);
    
    std::string _type = "boxt";
    uint64_t    _size; // Choix de renseigner une taille unique sur 8 octets (pas de largesize)
    
    Box* _parent = nullptr;     // référence vers la boîte parente
    std::vector<std::unique_ptr<Box>> _children; // vecteur contenant les enfants de la boîte
};

class FullBox : public Box {
public:
    // getter/setter
    uint8_t  getVersion()          { return _version; }
    void     setVersion(uint8_t v) {_version = v; }
    uint32_t getFlags()            {return _flags; }
    void     setFlags(uint32_t f)  { _flags = f; }
    
protected:
    uint8_t  _version = 0;
    uint32_t _flags   = 0; // on utilisera seulement les 24 premiers bits de _flags

    // Parse la version et les flags
    void parseVF(std::ifstream& pFile);
};

class Root final : public FullBox {
public:
    void parse(std::ifstream *pFile) override;
    void setParent(Box *pParent) override;
}; // conteneur racine, sans informations particulières

class Ftyp final : public Box {
public:
    char     major_brand[4];
    uint32_t minor_version;
    std::vector<std::array<char, 4>> compatible_brands;

    void setParent(Box *pParent) override;
    // Parse le fichier : avance jusqu'à la prochaine boîte et stocke les données de la boîte dans sa classe
    void parse(std::ifstream *pFile) override;

private:
    char _type[5] = "ftyp";
};

class Mdat final : public Box {
public:
    uint64_t beg_data; // index de début des données images/audio dans le stream
    void setParent(Box *pParent) override;
    // Parse le fichier : avance le stream jusqu'à la prochaine boîte et stocke
    // le début des données. La fin de la boîte est connue grâce à sa taille.
    void parse(std::ifstream *pFile) override;
        
private:
    char _type[5] = "mdat";
};

class Free final : public Box {
public:
    char *data;
    
    void setParent(Box *pParent) override;
    // Saute la boîte entièrement
    void parse(std::ifstream *pFile) override;
    
private:
    char _type[5] = "free";
};

class Pdin final : public FullBox {
public:
    std::vector<uint32_t> rate;
    std::vector<uint32_t> initial_delay;

    void setParent(Box *pParent) override;
    void parse(std::ifstream *pFile) override;

private:
    char _type[5] = "pdin";
};

class Moov final : public Box {
public:
    void setParent(Box *pParent) override;
    // Avance le stream <*pFile> jusqu'à la prochaine boite de même niveau en parsant toutes les boîtes contenues.
    //     @pFile: un pointeur vers le stream du fichier analysé
    //     @returns: pas de valeur de retour
    void parse(std::ifstream *pFile) override;
    
private:
    char _type[5] = "moov";
}; 

class Mvhd final : public FullBox {
public:
    int version;
    // On utilise pour des uint64 peu importe la version
    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t timescale;
    uint64_t duration;
    uint32_t rate      = 0x00010000;
    uint16_t volume    = 0x0100;
    std::array<int32_t, 9> matrix = {0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x40000000}; // Matrice unité
    uint32_t next_track_ID;

    void setParent(Box *pParent) override;
    void parse(std::ifstream *pFile) override;
};

class Trak final : public Box {
public:
    void setParent(Box *pParent) override;
    void parse(std::ifstream *pFile) override;
}; 

class Tkhd final : public FullBox {
public:
    int      version;
    int32_t  flags;
    // On utilise des uint64 peu importe la version
    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t track_ID;
    uint64_t duration;
    int16_t  layer;
    int16_t  alternate_group;
    uint16_t volume;
    std::array<int32_t, 9> matrix = {0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x40000000}; // Matrice unité
    uint32_t width;
    uint32_t height;

    void setParent(Box *pParent) override;
    void parse(std::ifstream *pFile) override;
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

class Meta : public Box {};


struct treeBoxDisplay_s {
    Box *pBox;
    int level;                  // profondeur dans l'arbre
};
