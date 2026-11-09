#pragma once

#include <algorithm>
#include <string>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>

namespace RankEsp {

inline std::string GetRankName(int points, int rankId)
{
    (void)rankId;

    std::string rankStr = "Unknown";

    if (points < 1300)
        rankStr = "Bronze";
    else if (points < 1600)
        rankStr = "Silver";
    else if (points < 2100)
        rankStr = "Gold";
    else if (points < 2600) {
        if (points < 2225)
            rankStr = "Platinum I";
        else if (points < 2350)
            rankStr = "Platinum II";
        else if (points < 2475)
            rankStr = "Platinum III";
        else
            rankStr = "Platinum IV";
    }
    else if (points < 3200) {
        if (points < 2750)
            rankStr = "Diamond I";
        else if (points < 2900)
            rankStr = "Diamond II";
        else if (points < 3050)
            rankStr = "Diamond III";
        else
            rankStr = "Diamond IV";
    }
    else if (points < 6000)
        rankStr = "Heroic";
    else
        rankStr = "Grandmaster";

    return rankStr + " (" + std::to_string(points) + ")";
}

// ---------------------------------------------------------------------------
//  ICONE DE RANG — methode reprise du projet source "ESP RANK - COPIA"
//  36 badges embarques dans la DLL (RankIconsData.hpp) :
//  B(1..3) S(1..3) G(1..4) P(1..5) D(1..5) H(1..5) M(1..5) GM(1..6)
//  Le choix se fait avec le SCORE, exactement comme GetIconPathFromScore().
// ---------------------------------------------------------------------------
inline int IconIndexFromScore(int score)
{
    if (score < 0) score = 0;

    int idx;
    if      (score >= 3400) idx = 29 + (std::min)((score - 3400) / 100, 5);
    else if (score >= 3000) idx = 22 + (std::min)((score - 3000) / 80, 4);
    else if (score >= 2600) idx = 16 + (std::min)((score - 2600) / 80, 4);
    else if (score >= 2200) idx = 11 + (std::min)((score - 2200) / 80, 4);
    else if (score >= 1800) idx = 6  + (std::min)((score - 1800) / 80, 4);
    else if (score >= 1400) idx = 2  + (std::min)((score - 1400) / 100, 3);
    else if (score >= 1000) idx = 0  + (std::min)((score - 1000) / 133, 2);
    else                    idx = 0  + (std::min)(score / 333, 2);

    if (idx < 0)  idx = 0;
    if (idx > 35) idx = 35;
    return idx;
}

// ---------------------------------------------------------------------------
//  POINTEUR DE PROFIL (BaseProfileInfo*) — ordre de confiance :
//   (1) 0x18CC : ESP RANK - COPIA, le projet de reference  <-- utilise en 1er
//   (2) 0x18DC : note "FF India Max / OB55" ecrite dans ce meme projet
//   (3) 0x1A74 : dump OB55 V7A (1.130.1) — PlayerNetwork :
//                  protected BaseProfileInfo GFBBIPJFPAD; // 0x1A74
//  On ne garde que le premier pointeur qui ressemble a un VRAI profil
//  (score ET rank plausibles ensemble) : on ne lit jamais de la chair.
// ---------------------------------------------------------------------------
inline bool LooksLikeProfile(uint32_t p)
{
    if (p < 0x10000) return false;              // jamais un petit entier

    const int score = Mem.ReadS<int>(p + Offsets::ProfileScore);   // 0x5C RankPoint
    const int rank  = Mem.ReadS<int>(p + Offsets::ProfileRankId);  // 0x58 Rank

    if (score <= 0 || score > 50000) return false;   // score de rang attendu
    if (rank < 0 || rank > 80)       return false;   // tier id attendu
    return true;
}

inline bool GetProfile(uint32_t entity, uint32_t& out)
{
    out = 0;
    if (entity == 0) return false;

    static const uintptr_t kCandidates[] = {
        Offsets::ProfileInfo,         // 0x18CC — reference
        Offsets::ProfileInfoOB55,     // 0x18DC — note OB55 de la reference
        Offsets::Player_ProfileDump   // 0x1A74 — dump OB55 V7A
    };

    for (uintptr_t cand : kCandidates) {
        uint32_t p = 0;
        if (Mem.Read(entity + cand, p) && LooksLikeProfile(p)) {
            out = p;
            return true;
        }
    }
    return false;
}

// Score de rang (BaseProfileInfo.RankPoint) -> choix de l'icone.
inline bool ReadScore(uint32_t entity, int& outScore)
{
    outScore = 0;

    uint32_t info = 0;
    if (!GetProfile(entity, info)) return false;

    const int s = Mem.ReadS<int>(info + Offsets::ProfileScore);
    if (s <= 0 || s > 50000)   // profil pas rempli / lecture pourrie
        return false;

    outScore = s;
    return true;
}

inline bool ReadRankFromEntity(uint32_t entity, std::string& outRank, int* outScore = nullptr)
{
    if (outScore) *outScore = 0;

    uint32_t profilePtr = 0;
    if (!GetProfile(entity, profilePtr)) {
        outRank = "Unknown";
        if (outScore) ReadScore(entity, *outScore);
        return false;
    }

    const int rankId     = Mem.ReadS<int>(profilePtr + Offsets::ProfileRankId);   // 0x58
    const int rankPoints = Mem.ReadS<int>(profilePtr + Offsets::ProfileScore);    // 0x5C

    if (rankPoints <= 0 || rankPoints > 50000) {   // score incoherent : rien
        outRank.clear();
        if (outScore) *outScore = 0;
        return false;
    }

    outRank = GetRankName(rankPoints, rankId);
    if (outScore) *outScore = rankPoints;
    return true;
}

} // namespace RankEsp
