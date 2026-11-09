using AotForms;
using System;
using System.Collections.Generic;
using System.Numerics;
using System.Threading;

namespace AotForms
{
    internal static class EnemyPull360
    {
        private static Thread pullThread;
        private static CancellationTokenSource cts;
        private static bool isRunning = false;

        private static readonly Dictionary<uint, Vector3> originalPositions = new();
        private static Entity currentTarget = null;

        internal static void Start()
        {
            if (isRunning) return;

            cts = new CancellationTokenSource();
            pullThread = new Thread(() => Work(cts.Token))
            {
                IsBackground = true,
                Priority = ThreadPriority.AboveNormal
            };
            pullThread.Start();
            isRunning = true;
        }

        internal static void Stop()
        {
            if (!isRunning) return;

            cts?.Cancel();
            RestoreAllPositions();
            isRunning = false;
        }

        private static void Work(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                try
                {
                    if (!Config.EnemyPullEnabled)
                    {
                        Thread.Sleep(1);
                        continue;
                    }

                    if (Core.Width == -1 || Core.Height == -1 || !Core.HaveMatrix)
                    {
                        Thread.Sleep(1);
                        continue;
                    }

                    if (IsLocalFiring())
                    {
                        currentTarget = FindBestTarget();
                        if (currentTarget != null)
                        {
                            ApplyInstantPull(currentTarget);
                        }
                    }
                    else
                    {
                        RestoreAllPositions();
                        currentTarget = null;
                    }
                }
                catch { }

                Thread.Sleep(Config.EnemyPullTickMs > 0 ? Config.EnemyPullTickMs : 6);
            }
        }

        private static Entity FindBestTarget()
        {
            Entity bestTarget = null;
            float closestDist = float.MaxValue;
            var screenCenter = new Vector2(Core.Width / 2f, Core.Height / 2f);

            foreach (var entity in Core.Entities.Values)
            {
                if (!entity.IsKnown || entity.IsDead) continue;
                if (Config.IgnoreKnocked && entity.IsKnocked) continue;

                var head2D = W2S.WorldToScreen(Core.CameraMatrix, entity.Head, Core.Width, Core.Height);
                if (head2D.X < 1 || head2D.Y < 1) continue;

                float dist3D = Vector3.Distance(Core.LocalMainCamera, entity.Head);
                if (dist3D > Config.EnemyPullMaxDistance) continue;

                float crosshairDist = Vector2.Distance(screenCenter, head2D);
                if (crosshairDist > Config.AimBotFov) continue;

                if (crosshairDist < closestDist)
                {
                    closestDist = crosshairDist;
                    bestTarget = entity;
                }
            }
            return bestTarget;
        }

        private static bool IsLocalFiring()
        {
            try
            {
                if (InternalMemory.Read(Core.LocalPlayer + Offsets.IsFiring, out bool value))
                    return value;
                return false;
            }
            catch
            {
                return false;
            }
        }

        private static void ApplyInstantPull(Entity entity)
        {
            try
            {
                if (!InternalMemory.Read(entity.Address + (uint)Bones.Root, out uint rootBonePtr)) return;
                if (!InternalMemory.Read(rootBonePtr + 0x8, out uint transformValue)) return;
                if (!InternalMemory.Read(transformValue + 0x8, out uint transformObjPtr)) return;
                if (!InternalMemory.Read(transformObjPtr + 0x20, out uint matrixPtr)) return;

                if (!Transform.GetNodePosition(rootBonePtr, out var currentPos)) return;

                if (!originalPositions.ContainsKey(entity.Address))
                    originalPositions[entity.Address] = currentPos;

                Vector3 originalPos = originalPositions[entity.Address];

                Vector3 fireDirection = new Vector3(
                    Core.CameraMatrix.M13,
                    Core.CameraMatrix.M23,
                    Core.CameraMatrix.M33
                );
                fireDirection = Vector3.Normalize(fireDirection);

                Vector3 camPos = Core.LocalMainCamera;

                Vector3 toEnemy = currentPos - camPos;
                float projLength = Vector3.Dot(toEnemy, fireDirection);
                Vector3 linePoint = camPos + fireDirection * projLength;

                Vector3 finalPos = linePoint;

                float deltaX = finalPos.X - originalPos.X;
                float deltaZ = finalPos.Z - originalPos.Z;

                // Left/Right (X, Z) – same clamp ±5f
                if (Math.Abs(deltaX) > 5f)
                    finalPos.X = originalPos.X + Math.Sign(deltaX) * 5f;

                if (Math.Abs(deltaZ) > 5f)
                    finalPos.Z = originalPos.Z + Math.Sign(deltaZ) * 5f;

                // Y axis: bande neeche na jaye – sirf upar allow, max +1f
                if (finalPos.Y < originalPos.Y)
                    finalPos.Y = originalPos.Y;
                else if (finalPos.Y - originalPos.Y > 1f)
                    finalPos.Y = originalPos.Y + 1f;

                InternalMemory.Write((ulong)(matrixPtr + 0x60), finalPos);
            }
            catch { }
        }

        private static void RestoreAllPositions()
        {
            foreach (var entry in originalPositions)
            {
                try
                {
                    if (!InternalMemory.Read(entry.Key + (uint)Bones.Root, out uint rootBonePtr)) continue;
                    if (!InternalMemory.Read(rootBonePtr + 0x8, out uint transformValue)) continue;
                    if (!InternalMemory.Read(transformValue + 0x8, out uint transformObjPtr)) continue;
                    if (!InternalMemory.Read(transformObjPtr + 0x20, out uint matrixPtr)) continue;

                    InternalMemory.Write((ulong)(matrixPtr + 0x60), entry.Value);
                }
                catch { }
            }
            originalPositions.Clear();
        }
    }
}
