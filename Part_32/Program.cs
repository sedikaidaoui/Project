using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace AotForms
{
    /// <summary>
    /// Font paths aligned with CounterStrike2 overlay (Program.cs): tahomabd.ttf + fonts/fa-solid-900.ttf merge.
    /// </summary>
    internal static class UiFonts
    {
        /// <summary>C:\Windows\Fonts\tahomabd.ttf — base font for UI text, header, labels.</summary>
        public static string TahomabdPath =>
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Fonts), "tahomabd.ttf");

        /// <summary>Next to Client.dll: fonts\fa-solid-900.ttf (Font Awesome merge, same relative path as CounterStrike2).</summary>
        public static string FaSolidPath =>
            Path.Combine(AppContext.BaseDirectory, "fonts", "fa-solid-900.ttf");
    }

    internal static class Program
    {
        [UnmanagedCallersOnly(EntryPoint = "Load")]
        public static void Load(nint pVM)
        {
            if (pVM != 0)
            {
                InternalMemory.Initialize(pVM);
                var process = Process.GetCurrentProcess();
                ComWrappers.RegisterForMarshalling(WinFormsComInterop.WinFormsComWrappers.Instance);
                Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
                Application.EnableVisualStyles();

                BootstrapRuntime.MainWindowHandle = process.MainWindowHandle;
                var uiThread = new Thread(() =>
                {
                    try
                    {
                        new ESP().Run();
                    }
                    catch (Exception ex)
                    {
                        Debug.WriteLine(ex);
                    }
                })
                {
                    IsBackground = false
                };
                uiThread.SetApartmentState(ApartmentState.STA);
                uiThread.Start();
            }
            else
            {
                MessageBox.Show("Please Restart Your Emulator And Try again.");
                Environment.Exit(0);
            }
        }
    }
}
