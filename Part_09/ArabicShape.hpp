#pragma once
// ============================================================================
//  ArabicShape  —  rendu correct des noms arabes dans l'ESP
//
//  ImGui n'a ni bidi ni shaping : sans ce fichier un nom arabe s'affiche
//  lettre par lettre (formes isolees) et dans le mauvais sens visuel.
//
//  1) supprime les harakat (diacritiques) qui decalent l'avance des glyphes
//  2) formes contextuelles : isolated / initial / medial / final
//  3) ligatures LAM + ALEF (لا)
//  4) inversion visuelle (RTL) de chaque bloc arabe
//
//  Entree / sortie : UTF-8.
//  Si la police ne contient PAS la forme demandee -> on garde la lettre de
//  base : jamais pire qu'avant.
// ============================================================================

#include <imgui.h>
#include <cstdint>
#include <string>
#include <vector>

namespace ArabicShape
{
    enum Join : uint8_t { JU = 0, JR /*n'existe qu'a droite*/, JD /*double*/, JC /*tatweel*/ };

    struct Form { uint32_t base, iso, fin, ini, med; uint8_t join; };

    inline const Form* Find(uint32_t c)
    {
        static const Form kTable[] =
        {
            // base   isol.   final  init.  med.   jonction
            {0x0621, 0xFE80, 0,      0,      0,     JU},          // hamza
            {0x0622, 0xFE81, 0xFE82, 0,      0,     JR},          // alef madda
            {0x0623, 0xFE83, 0xFE84, 0,      0,     JR},          // alef hamza above
            {0x0624, 0xFE85, 0xFE86, 0,      0,     JR},          // waw hamza
            {0x0625, 0xFE87, 0xFE88, 0,      0,     JR},          // alef hamza below
            {0x0626, 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C, JD},          // yeh hamza
            {0x0627, 0xFE8D, 0xFE8E, 0,      0,     JR},          // alef
            {0x0628, 0xFE8F, 0xFE90, 0xFE91, 0xFE92, JD},          // beh
            {0x0629, 0xFE93, 0xFE94, 0,      0,     JR},          // teh marbuta
            {0x062A, 0xFE95, 0xFE96, 0xFE97, 0xFE98, JD},          // teh
            {0x062B, 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C, JD},          // theh
            {0x062C, 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0, JD},          // jeem
            {0x062D, 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4, JD},          // hah
            {0x062E, 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8, JD},          // khah
            {0x062F, 0xFEA9, 0xFEAA, 0,      0,     JR},          // dal
            {0x0630, 0xFEAB, 0xFEAC, 0,      0,     JR},          // thal
            {0x0631, 0xFEAD, 0xFEAE, 0,      0,     JR},          // reh
            {0x0632, 0xFEAF, 0xFEB0, 0,      0,     JR},          // zain
            {0x0633, 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4, JD},          // seen
            {0x0634, 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8, JD},          // sheen
            {0x0635, 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC, JD},          // sad
            {0x0636, 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0, JD},          // dad
            {0x0637, 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4, JD},          // tah
            {0x0638, 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8, JD},          // zah
            {0x0639, 0xFEC9, 0xFECA, 0xFECB, 0xFECC, JD},          // ain
            {0x063A, 0xFECD, 0xFECE, 0xFECF, 0xFED0, JD},          // ghain
            {0x0640, 0,      0,      0,      0,     JC},          // tatweel
            {0x0641, 0xFED1, 0xFED2, 0xFED3, 0xFED4, JD},          // feh
            {0x0642, 0xFED5, 0xFED6, 0xFED7, 0xFED8, JD},          // qaf
            {0x0643, 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC, JD},          // kaf
            {0x0644, 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0, JD},          // lam
            {0x0645, 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4, JD},          // meem
            {0x0646, 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8, JD},          // noon
            {0x0647, 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC, JD},          // heh
            {0x0648, 0xFEED, 0xFEEE, 0,      0,     JR},          // waw
            {0x0649, 0xFEEF, 0xFEF0, 0,      0,     JR},          // alef maksura
            {0x064A, 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4, JD},          // yeh
            {0x067E, 0xFB56, 0xFB57, 0xFB58, 0xFB59, JD},          // peh
            {0x0686, 0xFB7A, 0xFB7B, 0xFB7C, 0xFB7D, JD},          // tcheh
            {0x0688, 0xFB88, 0xFB89, 0,      0,     JR},          // ddal
            {0x0691, 0xFB8C, 0xFB8D, 0,      0,     JR},          // rreh
            {0x0698, 0xFB8A, 0xFB8B, 0,      0,     JR},          // zhe
            {0x06A9, 0xFB8E, 0xFB8F, 0xFB90, 0xFB91, JD},          // keheh
            {0x06AF, 0xFB92, 0xFB93, 0xFB94, 0xFB95, JD},          // gaf
            {0x06BA, 0xFB9E, 0xFB9F, 0,      0,     JR},          // noon ghunna
            {0x06CC, 0xFBFC, 0xFBFD, 0xFBFE, 0xFBFF, JD},          // farsi yeh
        };

        for (const Form& f : kTable)
            if (f.base == c) return &f;
        return nullptr;
    }

    inline bool IsArabic(uint32_t c)
    {
        return (c >= 0x0600 && c <= 0x06FF) || (c >= 0x0750 && c <= 0x077F) ||
               (c >= 0x08A0 && c <= 0x08FF) || (c >= 0xFB50 && c <= 0xFDFF) ||
               (c >= 0xFE70 && c <= 0xFEFF);
    }

    inline bool IsMark(uint32_t c)
    {
        return (c >= 0x0610 && c <= 0x061A) || (c >= 0x064B && c <= 0x065F) || c == 0x0670 ||
               (c >= 0x06D6 && c <= 0x06DC) || (c >= 0x06DF && c <= 0x06E4) ||
               (c >= 0x06E7 && c <= 0x06E8) || (c >= 0x06EA && c <= 0x06ED);
    }

    inline bool     IsAlef(uint32_t c) { return c == 0x0622 || c == 0x0623 || c == 0x0625 || c == 0x0627; }
    inline Join     JoinOf(uint32_t c) { const Form* f = Find(c); return f ? (Join)f->join : JU; }

    // ligature LAM + ALEF : forme finale si le lam est deja lie a gauche
    inline uint32_t LamAlef(uint32_t alef, bool joinedToPrev)
    {
        switch (alef)
        {
            case 0x0622: return joinedToPrev ? 0xFEF6 : 0xFEF5;
            case 0x0623: return joinedToPrev ? 0xFEF8 : 0xFEF7;
            case 0x0625: return joinedToPrev ? 0xFEFA : 0xFEF9;
            default:     return joinedToPrev ? 0xFEFC : 0xFEFB;   // alef simple
        }
    }

    // ---- UTF-8 -------------------------------------------------------------
    inline void DecodeUtf8(const std::string& s, std::vector<uint32_t>& out)
    {
        out.clear();
        size_t i = 0;
        while (i < s.size())
        {
            const unsigned char c = (unsigned char)s[i];
            uint32_t cp = 0; size_t n = 0;
            if (c < 0x80)               { cp = c;          n = 1; }
            else if ((c >> 5) == 6)     { cp = c & 0x1F;   n = 2; }
            else if ((c >> 4) == 14)    { cp = c & 0x0F;   n = 3; }
            else if ((c >> 3) == 30)    { cp = c & 0x07;   n = 4; }
            else { ++i; continue; }                       // octet parasite : ignore
            if (i + n > s.size()) break;                  // sequence tronquee
            for (size_t k = 1; k < n; ++k)
                cp = (cp << 6) | ((unsigned char)s[i + k] & 0x3F);
            out.push_back(cp);
            i += n;
        }
    }

    inline void EncodeUtf8(uint32_t cp, std::string& out)
    {
        if (cp < 0x80) out.push_back((char)cp);
        else if (cp < 0x800)
        {
            out.push_back((char)(0xC0 | (cp >> 6)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        }
        else if (cp < 0x10000)
        {
            out.push_back((char)(0xE0 | (cp >> 12)));
            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        }
        else
        {
            out.push_back((char)(0xF0 | (cp >> 18)));
            out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (cp & 0x3F)));
        }
    }

    // ---- point d'entree ----------------------------------------------------
    // font = police qui va dessiner (pour ne jamais demander un glyphe absent)
    inline std::string Process(const std::string& utf8, ImFont* font = nullptr)
    {
        // test rapide : toute lettre arabe utf-8 commence par 0xD8..0xDF
        bool maybe = false;
        for (unsigned char c : utf8)
            if (c >= 0xD6) { maybe = true; break; }
        if (!maybe)
            return utf8;                                   // zero cout dans 99% des cas

        std::vector<uint32_t> cps;
        DecodeUtf8(utf8, cps);
        if (cps.empty())
            return utf8;

        auto Has = [font](uint32_t cp) -> bool {
            if (cp == 0 || !font) return true;
            return font->FindGlyphNoFallback((ImWchar)cp) != nullptr;
        };

        std::string out;
        out.reserve(utf8.size());

        size_t i = 0;
        while (i < cps.size())
        {
            // hors bloc arabe : copie directe
            if (!IsArabic(cps[i]) || IsMark(cps[i]))
            {
                EncodeUtf8(cps[i], out);
                ++i;
                continue;
            }

            // ---- collecte du bloc arabe ----
            size_t j = i;
            while (j < cps.size() && IsArabic(cps[j])) ++j;

            std::vector<uint32_t> shaped;
            shaped.reserve(j - i);

            uint8_t prevJoin = JU;
            bool    hasPrev  = false;

            for (size_t p = i; p < j; )
            {
                if (IsMark(cps[p])) { ++p; continue; }     // harakat supprimes

                const uint32_t base = cps[p];

                size_t np = p + 1;
                while (np < j && IsMark(cps[np])) ++np;
                const uint32_t next = (np < j) ? cps[np] : 0;

                const Form* f   = Find(base);
                const Join  jt  = f ? (Join)f->join : JU;

                const bool joinsPrev = hasPrev &&
                                       (prevJoin == JD || prevJoin == JC) &&
                                       (jt == JD || jt == JR);

                // ---- ligature LAM + ALEF ----
                if (base == 0x0644 && next != 0 && IsAlef(next))
                {
                    const uint32_t lig = LamAlef(next, joinsPrev);
                    if (Has(lig))
                    {
                        shaped.push_back(lig);
                        p = np + 1;                       // consomme l'alef aussi
                        hasPrev = true; prevJoin = JR;    // la ligature n'a qu'un cote libre
                        continue;
                    }
                    // glyphe absent -> on traite lam puis alef normalement
                }

                uint32_t chosen = base;
                if (f && f->iso != 0)
                {
                    const bool joinsNext = (jt == JD) && next != 0 &&
                                           (JoinOf(next) == JD || JoinOf(next) == JR);

                    if (joinsPrev && joinsNext && f->med != 0 && Has(f->med))      chosen = f->med;
                    else if (joinsPrev && f->fin != 0 && Has(f->fin))               chosen = f->fin;
                    else if (joinsNext && f->ini != 0 && Has(f->ini))               chosen = f->ini;
                    else if (Has(f->iso))                                           chosen = f->iso;
                    else                                                            chosen = base;
                }

                shaped.push_back(chosen);
                hasPrev = true;
                prevJoin = jt;
                p = np;
            }

            // ---- inversion visuelle (RTL) ----
            for (size_t k = shaped.size(); k-- > 0; )
                EncodeUtf8(shaped[k], out);

            i = j;
        }

        return out;
    }
}
