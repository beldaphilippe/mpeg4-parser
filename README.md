# MPEG-4

## MP4 : description du format de fichier

La structure d'un fichier se caractérise par une organisation sous forme d'objets.  
Une introduction claire : [A Quick Dive Into MP4](https://github.com/alfg/quick-dive-into-mp4).  

Le header d'une boîte est constitué ainsi :  
taille (4 octets), type (4 octets)  
Puis suivent les données de la boîte.  

La taille obtenue dans l'entête suit les règles suivantes :  
    * la taille est la taille de la boîte en entier, en englobant l'entête complet et tous les champs ou boîtes contenus dans la boîte courante.  
    * si elle vaut 0, la boîte courante est la dernière du fichier et son contenu s'étant jusqu'à la fin du fichier.  
    * si elle vaut 1, la taille est contenue dans le champ `largesize` de la boîte.  
    
### ftyp (file type box)

| Type de boîte | `moov`                                           |
|---------------|--------------------------------------------------|
| Contenant     | Fichier                                          |
| Obigatoire    | Oui                                              |
| Quantité      | Unique                                           |
| Description   | Contient des informations sur le fichier stocké. |

| Champs            | Taille (octets) | Description                                                     |
|:------------------|:----------------|:----------------------------------------------------------------|
| major_brand       | 4               | indentifiant de type                                            |
| minor_version     | 4               | version mineure du type majeur (informatif seulement)           |
| compatible_brands | 4 x N           | liste des types compatibles, s'étend jusqu'à la fin de la boîte |


### mdat (media data box)

| Type de boîte | `mdat`                                                    |
|---------------|-----------------------------------------------------------|
| Contenant     | Fichier                                                   |
| Obigatoire    | Non                                                       |
| Quantité      | Zéro ou plus                                              |
| Description   | Contient les données multimédia, commes les images vidéo. |

| Champs | Taille (octets) | Description                                            |
|:-------|:----------------|:-------------------------------------------------------|
| data   | N               | données multimédia, s'étend jusqu'à la fin de la boîte |


### free | skip

| Type de boîte | `free` ou `skip`                                                               |
|---------------|--------------------------------------------------------------------------------|
| Contenant     | Fichier ou autre boîte                                                         |
| Obigatoire    | Non                                                                            |
| Quantité      | Zéro ou plus                                                                   |
| Description   | Contenu non pertinent, pouvant être ignoré ou supprimé (attention aux offsets) |

| Champs | Taille (octets) | Description                        |
|:-------|:----------------|:-----------------------------------|
| data   | N               | s'étend jusqu'à la fin de la boîte |


### pdin (progressive download information box)

| Type de boîte | `pdin`                                                                                                                                                |
|---------------|-------------------------------------------------------------------------------------------------------------------------------------------------------|
| Contenant     | Fichier                                                                                                                                               |
| Obigatoire    | Non                                                                                                                                                   |
| Quantité      | Zéro ou un                                                                                                                                            |
| Description   | Contient des paires de nombres représentant des associations de bitrate de téléchargement de fichier (octets/s) et de délai initial de playback (ms). |
|               |                                                                                                                                                       |

| Champs        | Taille (octets) | Description                           |
|:--------------|:----------------|:--------------------------------------|
| rate          | 4               | vitesse de téléchargement en octets/s |
| initial_delay | 4               | délai initial de lecture              |


### moov (movie box)

| Type de boîte | `moov`                                                                         |
|---------------|--------------------------------------------------------------------------------|
| Contenant     | Fichier ou autre boîte                                                         |
| Obigatoire    | Oui                                                                            |
| Quantité      | Unique                                                                         |
| Description   | Contenu non pertinent, pouvant être ignoré ou supprimé (attention aux offsets) |

