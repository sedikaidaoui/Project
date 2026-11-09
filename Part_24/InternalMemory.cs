
using System.Runtime.InteropServices;
using System.Text;

namespace AotForms
{
    internal static class InternalMemory
    {
        [DllImport("AotBst.dll")]
        static extern nint CPU(nint pVM, uint cpuId);

        [DllImport("AotBst.dll")]
        static extern int InternalRead(nint pVM, ulong address, nint buffer, uint size);

        [DllImport("AotBst.dll")]
        static extern int Cast(nint pVCpu, ulong address, out ulong physAddress);

        [DllImport("AotBst.dll")]
        static extern int InternalWrite(nint pVM, ulong address, nint buffer, uint size);


        static nint pVMAddr;
        static nint cpuAddr;
        internal static Dictionary<ulong, ulong> Cache;
        internal static void Initialize(nint pVM)
        {
            pVMAddr = pVM;
            cpuAddr = CPU(pVM, 0);
            Cache = new Dictionary<ulong, ulong>();
        }

        internal static bool Convert(ulong address, out ulong phys) {
            phys = 0;

            if (Cache.TryGetValue(address, out var cachedPhys)) {
                phys = cachedPhys;
                return true;
            }

            cpuAddr = CPU(pVMAddr, 0);
            var status = Cast(cpuAddr, address, out phys);

            if (status == 0 && !Config.NoCache) {
                Cache[address] = phys;
                return true;
            }

            return false;
        }

        internal static unsafe bool Read<T>(ulong address, out T data) where T : struct {
            data = default;

            var result = Convert(address, out address);

            if (!result)
                return false;

            T buffer = default;
            var bufferReference = __makeref(buffer);
            var size = (uint)Marshal.SizeOf<T>();

            var status = InternalRead(pVMAddr, address, *(nint*)&bufferReference, size);
            data = buffer;
            return status == 0;
        }

        public static byte[] ReadBytes(ulong address, int size)
        {
            byte[] buffer = new byte[size];
            if (ReadArray(address, ref buffer))
            {
                return buffer;
            }
            return new byte[0];
        }

        public static unsafe void WriteBytes(ulong address, byte[] bytes)
        {
            var result = Convert(address, out address);

            if (!result || bytes == null || bytes.Length == 0)
                return;

            fixed (byte* pBytes = bytes)
            {
                var size = (uint)bytes.Length;
                InternalWrite(pVMAddr, address, *(nint*)&pBytes, size);
            }
        }
        public static async Task RVA_Patch(uint address, byte[] replace)
        {

            bool allOffsetsUpdated = false;

            while (!allOffsetsUpdated)
            {
                allOffsetsUpdated = true;

                try
                {
                    WriteBytes(address, replace);
                }
                catch
                {
                    allOffsetsUpdated = false;
                }

                try
                {
                    byte[] currentValue = ReadBytes(address, replace.Length);

                    if (currentValue == null)
                    {
                        allOffsetsUpdated = false;
                        break;
                    }

                    if (!currentValue.SequenceEqual(replace))
                    {
                        allOffsetsUpdated = false;
                        break;
                    }
                }
                catch
                {
                    allOffsetsUpdated = false;
                    break;
                }
            }
        }
        internal static unsafe bool ReadArray<T>(ulong address, ref T[] array) where T : struct
        {
            var result = Convert(address, out address);

            if (!result)
                return false;

            var size = (uint)((ulong)Marshal.SizeOf(array[0]) * (ulong)array.Length);
            var typedReference = __makeref(array[0]);

            var status = InternalRead(pVMAddr, address, *(nint*)&typedReference, size);

            return status == 0;
        }

        internal static string ReadString(ulong address, int size, bool unicode = true)
        {
            var stringBytes = new byte[size];

            var read = ReadArray(address, ref stringBytes);

            if (!read) return "";

            var readString = unicode ? Encoding.Unicode.GetString(stringBytes) : Encoding.Default.GetString(stringBytes);

            if (readString.Contains('\0'))
                readString = readString.Substring(0, readString.IndexOf('\0'));

            return readString;
        }

        internal static unsafe void Write<T>(ulong address, T value) where T : struct
        {
            var result = Convert(address, out address);

            if (!result)
                return;

            var size = (uint)Marshal.SizeOf<T>();
            var bufferReference = __makeref(value);

            InternalWrite(pVMAddr, address, *(nint*)&bufferReference, size);
        }
    }
}
