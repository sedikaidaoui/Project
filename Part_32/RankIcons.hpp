#pragma once
// ===========================================================================
//  RankIcons — badges de rang embarques DANS la DLL.
//
//  Les 36 PNG (64x64) sont des tableaux de bytes (RankIconsData.hpp) :
//  aucun fichier externe, donc ca marche pour n'importe qui a qui on donne
//  uniquement le DLL.
//
//  Utilisation : RankIcons::SetDevice(dev) une fois au init, puis
//                RankIcons::Get(index) -> ImTextureID pour AddImage().
// ===========================================================================

#include <EspLines/RankIconsData.hpp>
#include <imgui.h>
#include <d3d11.h>
#include <D3DX11tex.h>

namespace RankIcons
{
    inline ID3D11Device*             g_device = nullptr;
    inline ID3D11ShaderResourceView* g_srv[RankIconsData::kCount] = {};

    // appele depuis ui.cpp ou le device D3D11 est connu
    inline void SetDevice(ID3D11Device* dev)
    {
        g_device = dev;
    }

    inline bool Ready()
    {
        return g_device != nullptr;
    }

    // index 0..35 (voir RankEsp::IconIndexFromScore) — lazy load + cache
    inline ImTextureID Get(int index)
    {
        if (index < 0 || index >= RankIconsData::kCount)
            return nullptr;

        if (g_srv[index])
            return (ImTextureID)g_srv[index];

        if (!g_device)
            return nullptr;

        const RankIconsData::Icon& icon = RankIconsData::kIcons[index];
        ID3D11ShaderResourceView* srv = nullptr;

        if (FAILED(D3DX11CreateShaderResourceViewFromMemory(
                g_device, icon.data, (UINT)icon.size, nullptr, nullptr, &srv, nullptr)) || !srv)
            return nullptr;

        g_srv[index] = srv;
        return (ImTextureID)srv;
    }
}
