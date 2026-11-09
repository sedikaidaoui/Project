# ============================================================================
#  stamp_integrity.ps1  —  sceau d'integrite du binaire 0xVoid
#
#  Apres chaque build : calcule le hash FNV-1a 64 du DLL (les 32 octets du
#  slot de sceau comptes comme zeroes) et l'ecrit DANS le slot, plus la
#  marque "stampe".
#
#  Au demarrage AuthGuard::VerifySeal() recalcule la meme chose sur le fichier
#  charge -> si quelqu'un modifie le DLL (patch, ajout de code, remplacement
#  d'une chaine...) le hash ne correspond plus = le panel se decharge.
#
#  Usage : powershell -File stamp_integrity.ps1 -Dll "C:\...\0xVoid Internal.dll"
# ============================================================================
param(
    [Parameter(Mandatory = $true)]
    [string]$Dll
)

$ErrorActionPreference = 'Stop'
$exitCode = 0

try {
    if (-not (Test-Path -LiteralPath $Dll)) {
        Write-Host "[stamp] DLL introuvable : $Dll"
        exit 0
    }

    # --- helpers en C# (5 Mo boucle PowerShell = trop lent) -----------------
    # MSBuild herite un LIB avec chemins relatifs -> csc le prend comme erreur.
    $env:LIB = ''
    $env:INCLUDE = ''
    $env:LIBPATH = ''
    if (-not ('SealTool' -as [type])) {
        Add-Type -TypeDefinition @"
using System;
public static class SealTool
{
    // little-endian : magie 0x0F1E2D3C4B5A6978 -> 78 69 5A 4B 3C 2D 1E 0F
    static readonly byte[] Magic = { 0x78,0x69,0x5A,0x4B,0x3C,0x2D,0x1E,0x0F };

    public static int FindSlot(byte[] b)
    {
        for (int i = 0; i + 32 <= b.Length; i++)
        {
            bool ok = true;
            for (int k = 0; k < 8 && ok; k++) if (b[i + k] != Magic[k]) ok = false;
            if (!ok) continue;

            // slot[3] : soit tout zero (build neuf), soit deja stampe
            bool zeros = true, stamped = true;
            for (int k = 24; k < 32; k++)
            {
                if (b[i + k] != 0) zeros = false;
                if (b[i + k] != Stamp[k - 24]) stamped = false;
            }
            if (zeros || stamped) return i;
        }
        return -1;
    }

    static readonly byte[] Stamp = { 0xED,0x5E,0xDE,0xC0,0xE5,0x0D,0x1C,0xA1 }; // 0xA11C0DE5C0DE5EED

    public static ulong Fnv(byte[] b, int off)
    {
        ulong h = 14695981039346656037UL;
        for (long i = 0; i < b.Length; i++)
        {
            byte v = (i >= off && i < off + 32) ? (byte)0 : b[i];
            h ^= v;
            h *= 1099511628211UL;
        }
        return h;
    }

    public static void PutU64(byte[] b, int off, ulong v)
    {
        for (int k = 0; k < 8; k++) b[off + k] = (byte)((v >> (8 * k)) & 0xFF);
    }

    public static ulong GetU64(byte[] b, int off)
    {
        ulong v = 0;
        for (int k = 0; k < 8; k++) v |= ((ulong)b[off + k]) << (8 * k);
        return v;
    }
}
"@
    }

    $bytes = [System.IO.File]::ReadAllBytes($Dll)
    $off = [SealTool]::FindSlot($bytes)

    if ($off -lt 0) {
        Write-Host "[stamp] slot de sceau INTROUVABLE dans $Dll (le binaire est-il celui du projet ?)"
        exit 0
    }

    $hash = [SealTool]::Fnv($bytes, $off)

    [SealTool]::PutU64($bytes, $off + 8, $hash)                                  # hash du fichier
    [SealTool]::PutU64($bytes, $off + 24, [System.Convert]::ToUInt64('A11C0DE5C0DE5EED', 16))   # marque "stampe"

    [System.IO.File]::WriteAllBytes($Dll, $bytes)

    # --- verification immediate (relecture) --------------------------------
    $again = [System.IO.File]::ReadAllBytes($Dll)
    $off2 = [SealTool]::FindSlot($again)
    if ($off2 -lt 0) {
        Write-Host "[stamp] slot introuvable apres ecriture"
        exit 0
    }
    $stored = [SealTool]::GetU64($again, $off2 + 8)
    $rehash = [SealTool]::Fnv($again, $off2)

    if ($stored -ne $rehash) {
        Write-Host "[stamp] ECHEC verification : stocke=$stored recalcule=$rehash"
        $exitCode = 0     # jamais de build rouge : sans sceau le panel marche quand meme
    }
    else {
        Write-Host ("[stamp] OK  sceau ecrit a 0x{0:X}  hash=0x{1:X16}" -f $off, $hash)
    }
}
catch {
    Write-Host "[stamp] ignore : $($_.Exception.Message)"
}

exit $exitCode
