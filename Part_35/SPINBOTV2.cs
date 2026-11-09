using AotForms;
using System;
using System.Numerics;
using System.Threading;
using System.Threading.Tasks;

namespace AotForms
{
    public static class RapidSpin
    {
        private static Task? _spinLoopTask;
        private static CancellationTokenSource? _cts;
        private static volatile bool _enabled;

        private const float SpinDegreesPerTick = 4.2f;
        private const int LoopDelayMs = 5;

        public static bool IsActive => _enabled;

        public static void Activate()
        {
            if (_enabled)
                return;

            _enabled = true;

            _cts?.Dispose();
            _cts = new CancellationTokenSource();

            _spinLoopTask = Task.Run(() => SpinLoopAsync(_cts.Token), _cts.Token);
        }

        public static void Deactivate()
        {
            if (!_enabled)
                return;

            try
            {
                _cts?.Cancel();
            }
            catch
            {
                // suppress
            }

            _enabled = false;
            _cts?.Dispose();
            _cts = null;
        }

        private static async Task SpinLoopAsync(CancellationToken cancelToken)
        {
            float currentYaw = 0f;

            while (!cancelToken.IsCancellationRequested)
            {
                try
                {
                    if (!Config.spinbot)
                    {
                        currentYaw = 0f;
                        await Task.Delay(LoopDelayMs, cancelToken);
                        continue;
                    }

                    ulong rawLocal = Core.LocalPlayer;
                    if (rawLocal == 0)
                    {
                        await Task.Delay(12, cancelToken);
                        continue;
                    }

                    uint localPlayer = (uint)rawLocal;

                    // Root → Transform chain
                    if (!InternalMemory.Read(localPlayer + (uint)Bones.Root, out uint root) || root == 0)
                        goto next;

                    if (!InternalMemory.Read(root + 0x8, out uint t1) || t1 == 0)
                        goto next;

                    if (!InternalMemory.Read(t1 + 0x8, out uint t2) || t2 == 0)
                        goto next;

                    if (!InternalMemory.Read(t2 + 0x20, out uint visualState) || visualState == 0)
                        goto next;

                    // Position + Rotation
                    if (!InternalMemory.Read(visualState + 0x60, out Vector3 pos))
                        goto next;

                    currentYaw += SpinDegreesPerTick;
                    if (currentYaw >= 360f)
                        currentYaw -= 360f;

                    Quaternion rot = Quaternion.CreateFromAxisAngle(
                        Vector3.UnitY,
                        currentYaw * (MathF.PI / 180f)
                    );

                    // Write back
                    InternalMemory.Write(visualState + 0x60, pos);
                    InternalMemory.Write(visualState + 0x70, rot);
                }
                catch
                {
                    // silent fail on memory read/write issues
                }

            next:
                await Task.Delay(LoopDelayMs, cancelToken);
            }
        }

        // Optional: clean shutdown helper
        public static void Shutdown()
        {
            Deactivate();
            _spinLoopTask?.Wait(250);
            _spinLoopTask = null;
        }
    }
}