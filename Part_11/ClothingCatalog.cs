using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;

namespace AotForms
{
    internal sealed class ClothingItem
    {
        public uint Id { get; init; }
        public string Name { get; init; } = string.Empty;
        public string Category { get; init; } = string.Empty;
        public string Rarity { get; init; } = "WHITE";
    }

    /// <summary>
    /// A local catalog used exclusively by the wardrobe UI. Selections remain in
    /// this application and are never applied to another process.
    /// </summary>
    internal static class ClothingCatalog
    {
        public static readonly IReadOnlyList<string> Categories = new[]
        {
            "Top", "Bottom", "Shoes", "Head", "Mask", "Facepaint", "Other"
        };

        private static readonly List<ClothingItem> Items = new();
        private static bool _loaded;

        public static int Count => Items.Count;

        public static void EnsureLoaded()
        {
            if (_loaded) return;
            _loaded = true;

            string path = Path.Combine(AppContext.BaseDirectory, "clothes.json");
            if (!File.Exists(path)) return;

            try
            {
                using JsonDocument document = JsonDocument.Parse(File.ReadAllText(path));
                if (document.RootElement.ValueKind != JsonValueKind.Array) return;

                foreach (JsonElement entry in document.RootElement.EnumerateArray())
                {
                    if (!TryGetItemId(entry, out uint id))
                        continue;

                    if (entry.TryGetProperty("itemType", out JsonElement typeElement) &&
                        !string.Equals(typeElement.GetString(), "CLOTHES", StringComparison.OrdinalIgnoreCase))
                        continue;

                    string name = entry.TryGetProperty("description", out JsonElement nameElement)
                        ? nameElement.GetString() ?? "Unnamed item"
                        : "Unnamed item";
                    string rarity = entry.TryGetProperty("Rare", out JsonElement rarityElement)
                        ? rarityElement.GetString() ?? "WHITE"
                        : "WHITE";
                    string icon = entry.TryGetProperty("icon", out JsonElement iconElement)
                        ? iconElement.GetString() ?? string.Empty
                        : string.Empty;

                    Items.Add(new ClothingItem { Id = id, Name = name, Rarity = rarity, Category = GetCategory(id, name, icon) });
                }
            }
            catch (JsonException)
            {
                // Keep the UI available even when a user-supplied catalog is malformed.
            }
        }

        private static bool TryGetItemId(JsonElement entry, out uint id)
        {
            id = 0;
            if (!entry.TryGetProperty("itemID", out JsonElement idElement)) return false;

            return idElement.ValueKind switch
            {
                JsonValueKind.String => uint.TryParse(idElement.GetString(), out id),
                JsonValueKind.Number => idElement.TryGetUInt32(out id),
                _ => false,
            };
        }

        private static string GetCategory(uint id, string name, string icon)
        {
            if (name.Contains("(Mask)", StringComparison.OrdinalIgnoreCase)) return "Mask";
            if (name.Contains("(Facepaint)", StringComparison.OrdinalIgnoreCase)) return "Facepaint";
            if (icon.Contains("hair", StringComparison.OrdinalIgnoreCase)) return "Head";

            return (id / 1_000_000) switch
            {
                203 => "Top",
                204 => "Bottom",
                205 => "Shoes",
                211 => "Head",
                214 => "Facepaint",
                _ => "Other",
            };
        }

        public static IEnumerable<ClothingItem> ForCategory(string category) =>
            Items.Where(item => string.Equals(item.Category, category, StringComparison.OrdinalIgnoreCase));
    }
}
