using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Client;
using Reborn;

namespace AotForms
{
    public static class BootstrapRuntime
    {
        public static IntPtr MainWindowHandle { get; set; }

        public static readonly api Auth = new api(
            "Yami963813's Application",
            "ZUhklZUfJa",
            "139299e0e1c014e89ad5dbfd6a473cf7aa120cbece3ff3f3978aabeeb087df5c",
            "1.0");

        static BootstrapRuntime()
        {
            Auth.init();
            Auth.check();
        }

        public static IntPtr FindRenderWindow(IntPtr parent)
        {
            IntPtr renderWindow = IntPtr.Zero;
            WinAPI.EnumChildWindows(parent, (hWnd, lParam) =>
            {
                var sb = new StringBuilder(256);
                WinAPI.GetWindowText(hWnd, sb, sb.Capacity);
                string windowName = sb.ToString();
                if (!string.IsNullOrEmpty(windowName) && windowName != "HD-Player")
                    renderWindow = hWnd;
                return true;
            }, IntPtr.Zero);
            return renderWindow;
        }

        public static async Task<string?> InitEmulatorAsync()
        {
            var processes = Process.GetProcessesByName("HD-Player");
            if (processes.Length != 1)
                return "Open emulator.";

            var proc = processes[0];
            string? mainModulePath = Path.GetDirectoryName(proc.MainModule?.FileName);
            if (string.IsNullOrEmpty(mainModulePath))
                return "Reinstall emulator.";

            var adbPath = Path.Combine(mainModulePath, "HD-Adb.exe");
            if (!File.Exists(adbPath))
                return "Adb not Found. Reinstall emulator.";

            var adb = new Adb(adbPath);
            await adb.Kill();
            if (!await adb.Start())
                return "ADB failed to start. Restart emulator and try again.";

            var moduleAddr = await adb.FindModule("com.dts.freefireth", "libil2cpp.so");
            if (moduleAddr == 0)
                return "Go to lobby, logout once, then try again.";

            Offsets.Il2Cpp = moduleAddr;
            Core.Handle = FindRenderWindow(MainWindowHandle);
            return null;
        }

        public static async Task StartCheatsAsync()
        {
            new Thread(Data.Work) { IsBackground = true }.Start();
            new Thread(AimbotV2.Work) { IsBackground = true }.Start();
            new Thread(AimbotAi.Work) { IsBackground = true }.Start();
            new Thread(Kcsafesilnet.Work) { IsBackground = true }.Start();
            new Thread(KCBRUTALSILENT.Work) { IsBackground = true }.Start();
            new Thread(UpPlayer.Work) { IsBackground = true }.Start();
            new Thread(Tele.Work) { IsBackground = true }.Start();
            new Thread(ShakeKill.Work) { IsBackground = true }.Start();
            new Thread(KCTeleport.Work) { IsBackground = true }.Start();
            new Thread(KCSPAWNKIL.Work) { IsBackground = true }.Start();
            new Thread(KCCLIMBUP.Start) { IsBackground = true }.Start();
            new Thread(DownPlayer.Work) { IsBackground = true }.Start();
            new Thread(EnemyPull360.Start) { IsBackground = true }.Start();
            new Thread(EnemyPull360v2.Start) { IsBackground = true }.Start();
            new Thread(fprceaim.Start) { IsBackground = true }.Start();
            new Thread(undergroundkill.Work) { IsBackground = true }.Start();
            new Thread(KCMAGNETPULL.Start) { IsBackground = true }.Start();
            new Thread(KCMap_Teleport.Work) { IsBackground = true }.Start();
            new Thread(Teleportwall.Work) { IsBackground = true }.Start();
            await Task.Delay(1000);
        }
    }
}
