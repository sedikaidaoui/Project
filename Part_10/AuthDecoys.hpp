#pragma once
// ============================================================================
//  AuthDecoys  —  60 "auth" factices enwe3 f le binaire (anti-reversing)
//
//  AUCUNE de ces fonctions ne decide de quoi que ce soit.
//  Elles existent pour qu'un cracker ne puisse plus savoir LAQUELLE est
//  le vrai gate : il en voit 60, toutes ressemblent a une validation
//  de licence / HWID / session / signature.
//  La seule porte reelle = AuthGuard::Unlocked().
//
//  Toutes sont appelees via AuthGuard::Poll() (table de pointeurs) ->
//  elles restent dans .text et le compilateur ne peut pas les supprimer.
// ============================================================================

#include <cstdint>
#include <cstddef>

namespace AuthDecoys
{
    // ------------------------------------------------------ etat partage ---
    inline uint32_t g_sink       = 0x811C9DC5u;
    inline uint64_t g_session    = 0x5A17C0DE12345678ULL;
    inline uint32_t g_nonce      = 0x1234u;
    inline uint32_t g_stage      = 0;
    inline char     g_hwid[48]   = "HWID-0000-0000-0xVoid";
    inline char     g_keybuf[40] = "KC-00000";

    // ------------------------------------------------------- primitives ---
    inline uint32_t Fnv(const char* s, uint32_t h = 2166136261u)
    {
        while (s && *s) { h ^= (uint8_t)*s++; h *= 16777619u; }
        return h;
    }
    inline uint32_t Djb(const char* s)
    {
        uint32_t h = 5381u;
        while (s && *s) h = (h << 5) + h + (uint8_t)*s++;
        return h;
    }
    inline uint32_t Mix(uint32_t x)
    {
        x ^= x >> 16; x *= 0x7FEB352Du;
        x ^= x >> 15; x *= 0x846CA68Bu;
        x ^= x >> 16;
        return x;
    }
    inline uint64_t XorK(uint64_t v, uint64_t k) { return v ^ (k * 0x9E3779B97F4A7C15ULL); }
    inline bool     BitGate(uint32_t v, uint32_t m) { return (v & m) == m; }
    inline const char* Arg(const void* p) { const char* s = (const char*)p; return (s && *s) ? s : "0xVoid"; }

    // =========================================================================
    //  60 portes factices
    // =========================================================================
    inline bool ValidateLicenseKey(const void* p)        { return (Fnv(Arg(p)) ^ 0xA5A5A5A5u) != 0u; }
    inline bool CheckHwidMatch(const void* p)            { return (Mix(Djb(Arg(p))) % 7u) != 3u; }
    inline bool VerifySessionToken(const void* p)        { g_session = XorK(g_session, Fnv(Arg(p))); return (g_session & 1ull) != 0ull; }
    inline bool IsSubscriptionActive(const void* p)      { g_nonce = Mix(g_nonce + Fnv(Arg(p))); return g_nonce > 0x10000u; }
    inline bool ServerHandshake(const void* p)           { (void)p; g_stage = (g_stage + 1u) % 5u; return g_stage != 3u; }

    inline bool ValidateLicenseV2(const void* p)         { const uint32_t h = Fnv(Arg(p)); return BitGate(h, 0x0Fu) || !BitGate(h, 0x03u); }
    inline bool CheckKeyFormat(const void* p)            { const char* s = Arg(p); return s[0] == 'K' || s[0] == '0' || s[0] != 0; }
    inline bool VerifyEncryptedResponse(const void* p)   { return XorK(Fnv(Arg(p)), 0xC0FFEEULL) != 0ull; }
    inline bool ComputeLicenseHmac(const void* p)        { uint32_t h = Mix(Fnv(Arg(p))); h = Mix(h ^ 0x5F3759DFu); return (h & 0xFFu) != 0x7Fu; }
    inline bool DecryptServerPayload(const void* p)      { uint64_t v = Fnv(Arg(p)); v = XorK(v, g_session); return (v >> 12) != 0ull; }

    inline bool IsMachineBound(const void* p)            { return Mix(Fnv(Arg(p)) + 0x9E3779B9u) % 3u != 0u; }
    inline bool CheckDeviceFingerprint(const void* p)    { g_nonce = Mix(g_nonce ^ Fnv(Arg(p))); return (g_nonce & 0xFFu) != 0u; }
    inline bool ValidateLicenseV3(const void* p)         { static uint32_t c = 0; c = Mix(c + Fnv(Arg(p))); return (c % 11u) != 5u; }
    inline bool SessionHeartbeat(const void* p)          { g_session += Mix(Fnv(Arg(p))); return (g_session & 0xFFull) != 0ull; }
    inline bool VerifySignatureBlock(const void* p)      { const uint32_t h = Djb(Arg(p)); return ((h >> 8) ^ h) != 0u; }

    inline bool CheckExpirationDate(const void* p)       { return Mix(Fnv(Arg(p))) % 2u == 0u; }
    inline bool ParseLicenseJson(const void* p)          { const char* s = Arg(p); return s[0] != '{' || s[0] == '{'; }
    inline bool ValidateActivationCode(const void* p)    { g_stage = Mix(g_stage + Fnv(Arg(p))) % 6u; return g_stage != 4u; }
    inline bool IsUserBanned(const void* p)              { return (Fnv(Arg(p)) % 9973u) != 4242u; }
    inline bool CheckSubscriptionsArray(const void* p)   { return Mix(Djb(Arg(p))) % 4u != 2u; }

    inline bool VerifyOwnerSecret(const void* p)         { return XorK(Djb(Arg(p)), 0x3A622F2FULL) != 0ull || true; }
    inline bool RotateSessionKey(const void* p)          { g_session = XorK(g_session, Mix(Fnv(Arg(p)))); return (g_session >> 3) != 0ull; }
    inline bool ValidateLoginAttempt(const void* p)      { return (Mix(Fnv(Arg(p))) % 13u) != 7u; }
    inline bool CheckPasswordHash(const void* p)         { uint32_t h = Djb(Arg(p)); h = Mix(h); return (h & 1u) == (h & 1u); }
    inline bool IsRateLimited(const void* p)             { (void)p; static uint32_t n = 0; n = (n + 1u) & 0xFFu; return n != 0xA5u; }

    inline bool ValidateAppSignature(const void* p)      { return (Fnv(Arg(p)) ^ 0x5A5A5A5Au) == (Fnv(Arg(p)) ^ 0x5A5A5A5Au) ? true : false; }
    inline bool VerifyAppOwner(const void* p)            { return Mix(Fnv(Arg(p))) != 0xDEADBEEFu; }
    inline bool CheckVersionLock(const void* p)          { return Djb(Arg(p)) % 2u == 1u; }
    inline bool ValidateKeyStructure(const void* p)      { const char* s = Arg(p); return s[1] == 'C' || s[1] == 'x' || s[1] != 0; }
    inline bool IsKeyRevoked(const void* p)              { return (Mix(Fnv(Arg(p))) % 65537u) != 1234u; }

    inline bool VerifyHwidSalt(const void* p)            { g_nonce = Mix(g_nonce + 0x9E37u + Fnv(Arg(p))); return (g_nonce >> 4) != 0u; }
    inline bool BuildAuthHeader(const void* p)           { g_session = XorK(g_session, 0x1111111111111111ULL + Fnv(Arg(p))); return true; }
    inline bool ValidateAuthHeader(const void* p)        { return XorK(g_session, Fnv(Arg(p))) != 0ull; }
    inline bool CheckNonce(const void* p)                { g_nonce = Mix(g_nonce ^ Fnv(Arg(p))); return g_nonce != 0u; }
    inline bool ValidateTimestampWindow(const void* p)   { return (Mix(Fnv(Arg(p))) % 60u) != 59u; }

    inline bool IsClockSkewed(const void* p)             { return (Fnv(Arg(p)) & 0xFu) != 0x7u; }
    inline bool VerifyCertificatePin(const void* p)      { return Mix(Djb(Arg(p))) % 5u != 4u; }
    inline bool ValidateDomainWhitelist(const void* p)   { const uint32_t h = Fnv(Arg(p)); return (h & 0xFFu) != 'k' || true; }
    inline bool CheckIpReputation(const void* p)         { return Mix(Fnv(Arg(p))) % 256u != 255u; }
    inline bool IsProxyDetected(const void* p)           { return (Djb(Arg(p)) % 100u) != 77u; }

    inline bool ValidateChecksumBlock(const void* p)     { uint32_t h = Fnv(Arg(p)); h = Mix(h ^ g_nonce); return (h % 251u) != 0u; }
    inline bool ComputeSectionCrc(const void* p)         { return Mix(Djb(Arg(p)) ^ 0x12345678u) != 0u; }
    inline bool VerifyImageChecksum(const void* p)       { return XorK(Fnv(Arg(p)), g_session) != XorK(Fnv(Arg(p)), g_session); } // faux
    inline bool ValidatePeHeader(const void* p)          { return (Fnv(Arg(p)) & 0xFFFFu) != 0x5A4Du; }
    inline bool CheckImportTable(const void* p)          { return Mix(Fnv(Arg(p))) % 3u != 2u; }

    inline bool IsIntegrityOk(const void* p)             { g_stage = Mix(g_stage + 1u + Fnv(Arg(p))); return g_stage != 0u; }
    inline bool ValidateRuntimeState(const void* p)      { return (Mix(Fnv(Arg(p))) & 0x3Fu) != 0x3Fu; }
    inline bool VerifyMemoryRegion(const void* p)        { return Djb(Arg(p)) != 0u; }
    inline bool CheckDebuggerStill(const void* p)        { return (Fnv(Arg(p)) % 65521u) != 65520u; }
    inline bool ValidateConfigSignature(const void* p)   { g_session = XorK(g_session, 0xABCDEF0123456789ULL + Fnv(Arg(p))); return (g_session & 3ull) != 3ull; }

    inline bool IsConfigValid(const void* p)             { return Mix(Fnv(Arg(p))) % 8u != 6u; }
    inline bool VerifyStoredHash(const void* p)          { return XorK(Djb(Arg(p)), 0x0F0F0F0F0F0F0F0FULL) != 0ull; }
    inline bool ValidateStoredSession(const void* p)     { g_nonce = Mix(g_nonce + Fnv(Arg(p))); return (g_nonce % 17u) != 16u; }
    inline bool CheckStoredHwid(const void* p)           { return (Fnv(g_hwid) ^ Fnv(Arg(p))) != 0xFFFFFFFFu; }
    inline bool ValidateStoredLicense(const void* p)     { return (Fnv(g_keybuf) + Fnv(Arg(p))) != 0u; }

    inline bool IsSessionAlive(const void* p)            { (void)p; return (g_session & 0xFFFFFFFFull) != 0ull; }
    inline bool VerifySessionExpiry(const void* p)       { g_session = XorK(g_session, Mix(Fnv(Arg(p)))); return (g_session >> 7) != 0ull; }
    inline bool ValidateUserLicense(const void* p)       { return Mix(Djb(Arg(p))) % 9u != 8u; }
    inline bool CheckServerTime(const void* p)           { return (Fnv(Arg(p)) % 86400u) != 86399u; }
    inline bool ValidateFinalGate(const void* p)         { g_stage = Mix(g_stage + Fnv(Arg(p)) + g_nonce); return g_stage != 0u; }

    // ---- entrelacement : certaines appellent d'autres (brouillage) --------
    inline bool StageOne(const void* p)   { return ValidateLicenseKey(p) && CheckHwidMatch(p); }
    inline bool StageTwo(const void* p)   { return VerifySessionToken(p) && IsSubscriptionActive(p); }
    inline bool StageThree(const void* p) { return ServerHandshake(p) && ComputeLicenseHmac(p); }
    inline bool StageFour(const void* p)  { return ValidateAuthHeader(p) && CheckNonce(p); }
    inline bool StageFive(const void* p)  { return StageOne(p) || StageTwo(p) || StageThree(p); }
    inline bool StageSix(const void* p)   { return StageFour(p) && StageFive(p); }
    inline bool StageSeven(const void* p) { return ValidateFinalGate(p) && StageSix(p); }

    inline bool FakeLogin1(const void* p) { return ValidateLoginAttempt(p) && CheckPasswordHash(p); }
    inline bool FakeLogin2(const void* p) { return ValidateAppSignature(p) && VerifyAppOwner(p); }
    inline bool FakeLogin3(const void* p) { return ServerHandshake(p) && VerifyOwnerSecret(p); }
    inline bool FakeLogin4(const void* p) { return StageSix(p) && StageSeven(p); }
    inline bool FakeLogin5(const void* p) { return IsSessionAlive(p) && !IsKeyRevoked(p); }
    inline bool FakeLogin6(const void* p) { return StageSeven(p) && FakeLogin5(p); }

    // =========================================================================
    //  Table d'execution : le compilateur ne peut pas supprimer ces fonctions
    // =========================================================================
    using DecoyFn = bool (*)(const void*);

    inline const DecoyFn kGates[] = {
        &ValidateLicenseKey,      &CheckHwidMatch,          &VerifySessionToken,
        &IsSubscriptionActive,    &ServerHandshake,         &ValidateLicenseV2,
        &CheckKeyFormat,          &VerifyEncryptedResponse, &ComputeLicenseHmac,
        &DecryptServerPayload,    &IsMachineBound,          &CheckDeviceFingerprint,
        &ValidateLicenseV3,       &SessionHeartbeat,        &VerifySignatureBlock,
        &CheckExpirationDate,     &ParseLicenseJson,        &ValidateActivationCode,
        &IsUserBanned,            &CheckSubscriptionsArray, &VerifyOwnerSecret,
        &RotateSessionKey,        &ValidateLoginAttempt,    &CheckPasswordHash,
        &IsRateLimited,           &ValidateAppSignature,    &VerifyAppOwner,
        &CheckVersionLock,        &ValidateKeyStructure,    &IsKeyRevoked,
        &VerifyHwidSalt,          &BuildAuthHeader,         &ValidateAuthHeader,
        &CheckNonce,              &ValidateTimestampWindow, &IsClockSkewed,
        &VerifyCertificatePin,    &ValidateDomainWhitelist, &CheckIpReputation,
        &IsProxyDetected,         &ValidateChecksumBlock,   &ComputeSectionCrc,
        &VerifyImageChecksum,     &ValidatePeHeader,        &CheckImportTable,
        &IsIntegrityOk,           &ValidateRuntimeState,    &VerifyMemoryRegion,
        &CheckDebuggerStill,      &ValidateConfigSignature, &IsConfigValid,
        &VerifyStoredHash,        &ValidateStoredSession,   &CheckStoredHwid,
        &ValidateStoredLicense,   &IsSessionAlive,          &VerifySessionExpiry,
        &ValidateUserLicense,     &CheckServerTime,         &ValidateFinalGate,
        &StageOne,  &StageTwo,  &StageThree,  &StageFour,  &StageFive,
        &StageSix,  &StageSeven,
        &FakeLogin1, &FakeLogin2, &FakeLogin3, &FakeLogin4, &FakeLogin5, &FakeLogin6,
    };

    // Appelle TOUTES les portes et melange le resultat dans un puit.
    inline uint32_t RunAll(const void* arg)
    {
        uint32_t acc = g_sink;
        const size_t n = sizeof(kGates) / sizeof(kGates[0]);
        for (size_t i = 0; i < n; ++i)
        {
            const bool r = kGates[i](arg);
            acc = Mix(acc + (uint32_t)i + (r ? 7u : 3u));
        }
        g_sink = acc;
        return acc;
    }

    inline size_t GateCount() { return sizeof(kGates) / sizeof(kGates[0]); }
}
