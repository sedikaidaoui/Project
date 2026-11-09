#include "Fonts.hpp"
#include "FontAwesome.hpp"
#include "FontsTwo.hpp"
#include "FontInter.hpp"
#include "NoyhHeavy.hpp"     // NOYH Heavy : police du menu (Fonts/noyh-heavy.otf)
#include "Imagens/LogoBytes.hpp"
#include <cstdio>     // fopen_s
#include <cstring>    // memcmp
#include <cstdint>    // uint16_t

namespace FWork {
    void Fonts::Initialize(ID3D11Device* Device) {
        ImGuiIO& io = ImGui::GetIO();
        ImFontConfig FontAwesomeConfig;
        FontAwesomeConfig.GlyphMinAdvanceX = 25.f * (2.0f / 3.0f);

        static const ImWchar IconRanges[] = {
            ICON_MIN_FA, ICON_MAX_FA, 0
        };

        // ------------------------------------------------------------------
        //  POLICE DU MENU — NOYH Heavy (Fonts/noyh-heavy.otf).
        //  C'est la PREMIERE fonte ajoutee : ImGui l'utilise par defaut
        //  (c'est Fonts[0]), donc tout le menu passe en NOYH Heavy.
        //  FontDataOwnedByAtlas = false : les octets sont statiques dans la
        //  DLL, ImGui ne doit surtout pas les free().
        //  Si l'OTF etait refuse, on retombe sur Inter Black (meme rang).
        // ------------------------------------------------------------------
        {
            ImFontConfig noyhCfg;
            noyhCfg.FontDataOwnedByAtlas = false;
            noyhCfg.OversampleH = 2;
            noyhCfg.OversampleV = 1;
            noyhCfg.PixelSnapH  = true;

            InterBlack = io.Fonts->AddFontFromMemoryTTF(
                (void*)NoyhHeavy_data, (int)NoyhHeavy_size, 14, &noyhCfg);

            if (!InterBlack)   // filet de securite
                InterBlack = io.Fonts->AddFontFromMemoryCompressedTTF(InterBlack_compressed_data, InterBlack_compressed_size, 14);
        }

        // ------------------------------------------------------------------
        //  NOYH Heavy en 18px : libelles des widgets / parametres du menu
        //  (FunctionFont). Meme police que Fonts[0] -> le menu ecrit BIEN
        //  avec la fonte qui a ete donnee. Place AVANT InterRegular : le bloc
        //  de fusion (Arabe / emojis) continue de cibler InterRegular.
        // ------------------------------------------------------------------
        {
            ImFontConfig noyhMenuCfg;
            noyhMenuCfg.FontDataOwnedByAtlas = false;
            noyhMenuCfg.OversampleH = 2;
            noyhMenuCfg.OversampleV = 1;
            noyhMenuCfg.PixelSnapH  = true;

            NoyhMenu = io.Fonts->AddFontFromMemoryTTF(
                (void*)NoyhHeavy_data, (int)NoyhHeavy_size, 20, &noyhMenuCfg);   // libelles : 20px
            if (!NoyhMenu)
                NoyhMenu = InterBlack;   // filet de securite
        }
        InterBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 17);
        InterBold12 = io.Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 15);
        InterExtraBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterExtraBold_compressed_data, InterExtraBold_compressed_size, 13);
        InterExtraLight = io.Fonts->AddFontFromMemoryCompressedTTF(InterExtraLight_compressed_data, InterExtraLight_compressed_size, 14);
        InterLight = io.Fonts->AddFontFromMemoryCompressedTTF(InterLight_compressed_data, InterLight_compressed_size, 12);
        InterMedium = io.Fonts->AddFontFromMemoryCompressedTTF(InterMedium_compressed_data, InterMedium_compressed_size, 17);
        InterRegular = io.Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 17);

        // ------------------------------------------------------------------
        //  NOMS ESP — glyphes MANQUANTS ajoutes a InterRegular.
        //  (MergeMode : la fusion se fait dans la DERNIERE fonte ajoutee,
        //   donc ce bloc doit rester pile ici.)
        //  Arabe + formes liees, Latin etendu, symboles de decoration,
        //  Indic/Tibetan/Javanais, emoji.  Sans ca les noms arabes et les
        //  emojis s'affichent en trous ("les names" qui manquent).
        //  Tout vient de C:\Windows\Fonts : rien a installer chez l'autre.
        // ------------------------------------------------------------------
        {
            static const ImWchar kArabicRanges[] = {
                0x0600, 0x06FF,   // Arabic
                0x0750, 0x077F,   // Arabic Supplement
                0x08A0, 0x08FF,   // Arabic Extended-A
                0xFB50, 0xFDFF,   // Arabic Presentation Forms-A
                0xFE70, 0xFEFF,   // Arabic Presentation Forms-B (lettres liees)
                0
            };
            static const ImWchar kLatinExtRanges[] = {
                0x0100, 0x024F,   // Latin Extended-A/B
                0x0370, 0x03FF,   // Greek
                0x0400, 0x04FF,   // Cyrillic
                0x1E00, 0x1EFF,   // Latin Extended Additional
                0
            };
            static const ImWchar kSymbolRanges[] = {
                0x2000, 0x206F,   // ponctuation generale
                0x20A0, 0x20CF,   // monnaies
                0x2100, 0x218F,   // lettres / nombres
                0x2190, 0x21FF,   // fleches
                0x2200, 0x22FF,   // maths
                0x2300, 0x23FF,   // divers techniques
                0x2460, 0x24FF,   // chiffres encadres
                0x2500, 0x257F,   // tableaux
                0x2580, 0x259F,   // blocs
                0x25A0, 0x25FF,   // formes geometriques
                0x2600, 0x26FF,   // symboles divers
                0x2700, 0x27BF,   // dingbats (ornements)
                0x2B00, 0x2BFF,   // symboles + fleches divers (etoiles)
                0
            };
            static const ImWchar kIndicRanges[] = {
                0x0900, 0x097F,   // Devanagari
                0x0F00, 0x0FFF,   // Tibetain
                0x1000, 0x109F,   // Birman
                0x1780, 0x17FF,   // Khmer
                0xA980, 0xA9DF,   // Javanais (ornements)
                0
            };
            static const ImWchar kEmojiRanges[] = {
                0xFE0F, 0xFE0F,   // variation selector-16
                0x1F1E6, 0x1F1FF, // indicateurs regionaux (drapeaux)
                0x1F300, 0x1F64F, // emoticones / transport
                0x1F680, 0x1F6FF,
                0x1F900, 0x1F9FF,
                0x1FA70, 0x1FAFF,
                0
            };

            // Filet de securite : on ne fusionne que les polices qui existent
            // et qui ont des glyphes VECTORIELS (glyf/CFF).  Segoe UI Emoji est
            // un font bitmap (CBDT) : stbtt ne sait pas le lire, Build()
            // echouerait et TOUT le texte du menu disparaitrait.
            auto HasVectorGlyphs = [](const char* path) -> bool {
                FILE* f = nullptr;
                if (fopen_s(&f, path, "rb") != 0 || !f) return false;

                unsigned char hdr[12] = {};
                bool ok = false;
                if (fread(hdr, 1, sizeof(hdr), f) == sizeof(hdr)) {
                    const uint16_t numTables = (uint16_t)((hdr[4] << 8) | hdr[5]);
                    if (numTables > 0 && numTables < 512) {
                        for (int i = 0; i < numTables; ++i) {
                            unsigned char rec[16] = {};
                            if (fread(rec, 1, sizeof(rec), f) != sizeof(rec)) break;
                            if (memcmp(rec, "glyf", 4) == 0 || memcmp(rec, "CFF ", 4) == 0) { ok = true; break; }
                        }
                    }
                }
                fclose(f);
                return ok;
            };

            auto MergeFrom = [&io, &HasVectorGlyphs](const char* file, const ImWchar* ranges) {
                char path[512];
                snprintf(path, sizeof(path), "C:\\Windows\\Fonts\\%s", file);
                if (!HasVectorGlyphs(path)) return;

                ImFontConfig cfg;
                cfg.MergeMode   = true;
                cfg.OversampleH = 1;
                cfg.OversampleV = 1;
                cfg.PixelSnapH  = false;
                io.Fonts->AddFontFromFileTTF(path, 17.0f, &cfg, ranges);
            };

            // Arabe (base + formes de presentation)
            MergeFrom("tahoma.ttf",        kArabicRanges);
            MergeFrom("arial.ttf",         kArabicRanges);
            MergeFrom("segoeui.ttf",       kArabicRanges);
            MergeFrom("sakkalmajalla.ttf", kArabicRanges);   // Arabe pur si present
            MergeFrom("trado.ttf",         kArabicRanges);   // Traditional Arabic si present
            // Latin etendu / Greek / Cyrillic
            MergeFrom("tahoma.ttf",        kLatinExtRanges);
            MergeFrom("arial.ttf",         kLatinExtRanges);
            MergeFrom("segoeui.ttf",       kLatinExtRanges);
            // Symboles de decoration (tableaux, blocs, formes, dingbats, etoiles)
            MergeFrom("seguisym.ttf",      kSymbolRanges);
            MergeFrom("ebrima.ttf",        kSymbolRanges);
            MergeFrom("segoeui.ttf",       kSymbolRanges);
            // Indic / Tibetain / Javanais (ornements)
            MergeFrom("nirmala.ttf",       kIndicRanges);
            // Emoji (UTF-32 -> necessite IMGUI_USE_WCHAR32)
            MergeFrom("seguiemj.ttf",      kEmojiRanges);
        }

        InterRegular14 = io.Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 15);
        InterSemiBold = io.Fonts->AddFontFromMemoryCompressedTTF(InterSemiBold_compressed_data, InterSemiBold_compressed_size, 16);
        InterThin = io.Fonts->AddFontFromMemoryCompressedTTF(InterThin_compressed_data, InterThin_compressed_size, 14);

        GeistRegular = io.Fonts->AddFontFromMemoryCompressedTTF(GeistRegular_compressed_data, GeistRegular_compressed_size, 16);
        GeistRegularMedium = io.Fonts->AddFontFromMemoryCompressedTTF(GeistRegular_compressed_data, GeistRegular_compressed_size, 18);
        GeistMedium = io.Fonts->AddFontFromMemoryCompressedTTF(GeistRegular_compressed_data, GeistRegular_compressed_size, 14);
        GeistBold = io.Fonts->AddFontFromMemoryCompressedTTF(GeistBold_compressed_data, GeistBold_compressed_size, 36);
        GeistBoldMedium = io.Fonts->AddFontFromMemoryCompressedTTF(GeistBold_compressed_data, GeistBold_compressed_size, 16);

        ImFontConfig customFontConfig;
        customFontConfig.MergeMode = true;
        customFontConfig.OversampleH = 1;
        customFontConfig.OversampleV = 1;
        customFontConfig.PixelSnapH = true;

        static const ImWchar customRanges[] = { 0xe000, 0xe204, 0x00 };
        IconWeapon = io.Fonts->AddFontFromMemoryCompressedTTF(weapon_compressed_data, weapon_compressed_size, 41.0f, &customFontConfig, customRanges);


        FontAwesomeRegular = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeRegular_compressed_data, FontAwesomeRegular_compressed_size, 25.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);
        FontAwesomeSolid = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 27.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);
        FontAwesomeSolid18 = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 18.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);
        FontAwesomeSolidBig = io.Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 30.f * (2.0f / 3.0f), &FontAwesomeConfig, &IconRanges[0]);

        // ------------------------------------------------------------------
        //  LIBELLES DES FONCTIONS — NOYH Heavy 20px  (FunctionFont)
        //  Meme police ET meme taille que les TITRES DES CARTES
        //  (Fonts::NoyhMenu) : demande "l font li fl3onwan dyal les card".
        //  Retour a 20px sur demande ("rj3 dakchi kifma kan") : le
        //  retrecissement demande ("sghr") porte sur la LIGNE, pas sur le
        //  texte — voir CheckboxRow (hauteur de ligne) si besoin.
        //  GlyphExtraSpacing.x = 1.0px : largeur du texte (demande "fl3ard").
        //  Place TRES en fin : la fusion Arabe/emoji cible la fonte AJOUTEE
        //  PREMIEREMENT dans son bloc, et IconWeapon cible la derniere fonte
        //  de ce bloc — on ne doit rien déplacer avant eux.
        // ------------------------------------------------------------------
        {
            ImFontConfig fnCfg;
            fnCfg.FontDataOwnedByAtlas = false;   // octets statiques dans la DLL
            fnCfg.OversampleH = 2;
            fnCfg.OversampleV = 1;
            fnCfg.PixelSnapH  = true;
            fnCfg.GlyphExtraSpacing.x = 1.0f;   // largeur du texte (fl3ard)

            NoyhFunctions = io.Fonts->AddFontFromMemoryTTF(
                (void*)NoyhHeavy_data, (int)NoyhHeavy_size, 20.0f, &fnCfg);
            if (!NoyhFunctions)   // filet de securite
                NoyhFunctions = NoyhMenu;
        }

        if (Device) {
            __try {
                D3DX11CreateShaderResourceViewFromMemory(Device, LogoCirco, sizeof(LogoCirco), NULL, NULL, &Logo, NULL);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                Logo = nullptr;
            }
        }
    }

    // Fusionne les chiffres 0-9 (Inter) dans la DERNIERE fonte ajoutee au atlas.
    // A appeler juste apres l'ajout de Spheris : MergeMode cible Fonts.back().
    void Fonts::MergeDigits(float size_pixels) {
        ImGuiIO& io = ImGui::GetIO();
        static const ImWchar DigitRanges[] = { 0x0030, 0x0039, 0 };

        ImFontConfig digitCfg;
        digitCfg.MergeMode = true;   // fusionne dans la fonte precedente (Spheris)
        digitCfg.PixelSnapH = false;

        io.Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, size_pixels, &digitCfg, DigitRanges);
    }
}
