using System;
using System.Numerics;


namespace AotForms
{
    internal class KCMap_Teleport
    {

        static uint staticGameFacade = 0;

        // ================= MAIN LOOP =================
        internal static void Work()
        {
            while (true)
            {
                FlyInitializer();

                if (!Config.teleportmap)
                {
                    continue;
                }

                TeleportLoop(MarkedPos());
            }
        }

        // ================= FLY HOOK =================
        public static async void EnableFlyHook(bool enable)
        {
            Config.flyhacks = true;

            if (enable)
            {
                await InternalMemory.RVA_Patch(
                    (uint)(Core.LibUnity + 0x64DA5C),
                    new byte[] { 0xAC, 0xC5, 0xA9, 0x3F, 0x00, 0x10, 0xA0, 0xE1 }
                );
            }
            else
            {
                await InternalMemory.RVA_Patch(
                    (uint)(Core.LibUnity + 0x64DA5C),
                    new byte[] { 0xAC, 0xC5, 0x27, 0x37, 0x00, 0x10, 0xA0, 0xE1 }
                );
            }
        }

        static void FlyInitializer()
        {
            if (!Config.flyhacks) return;

            if (!InternalMemory.Read<uint>(Core.LocalPlayer + 0X124C, out var phyxdata))
                return;

            if (!InternalMemory.Read<uint>(phyxdata + 0xC, out var phyxstate))
                return;

            InternalMemory.Write(phyxstate + 0x8, 1);
            Thread.Sleep(40);
            InternalMemory.Write(phyxstate + 0x8, 0);
            Config.flyhacks = false;
        }

        static void TeleportLoop(Vector3 pos)
        {

            if (!InternalMemory.Read<uint>(Core.LocalPlayer + (uint)Bones.Root, out uint root))
                return;

            if (!InternalMemory.Read<uint>(root + 0x8, out uint transform))
                return;

            if (!InternalMemory.Read<uint>(transform + 0x8, out uint obj))
                return;

            if (!InternalMemory.Read<uint>(obj + 0x20, out uint matrix))
                return;

            InternalMemory.Write(matrix + 0x60, pos);

            // Config.teleportmap = false;

        }

        private static Vector3 MarkedPos()
        {
            if (staticGameFacade == 0)
            {
                if (!InternalMemory.Read<uint>(Offsets.Il2Cpp + Offsets.InitBase, out var baseGameFacade))
                    return Vector3.Zero;

                if (!InternalMemory.Read<uint>(baseGameFacade, out var gameFacade))
                    return Vector3.Zero;

                if (!InternalMemory.Read<uint>(gameFacade + Offsets.StaticClass, out staticGameFacade))
                    return Vector3.Zero;
            }

            if (!InternalMemory.Read<uint>(staticGameFacade + 0x0, out var currentGame))
                return Vector3.Zero;

            if (!InternalMemory.Read<uint>(currentGame + 0x8, out uint UIInGameScene))
                return Vector3.Zero;

            if (!InternalMemory.Read<uint>(UIInGameScene + 0x1E4, out uint m_BigMapCtrl))
                return Vector3.Zero;

            if (!InternalMemory.Read<uint>(m_BigMapCtrl + 0x50, out uint m_MapContentCtrl))
                return Vector3.Zero;

            if (!InternalMemory.Read<uint>(m_MapContentCtrl + 0x88, out uint m_LocalMapMarkController))
                return Vector3.Zero;

            if (!InternalMemory.Read(m_LocalMapMarkController + 0x54, out Vector3 selectedpos))
                return Vector3.Zero;

            return selectedpos;
        }
    }
}
