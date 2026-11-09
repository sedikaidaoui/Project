package com.terminalx999;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.view.View;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class ParticlesView extends View {

    private static final int PARTICLE_COUNT = 0;
    private final List<BokehParticle> particles = new ArrayList<>();
    private final Paint glowPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint corePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint bgPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Random random = new Random();

    public ParticlesView(Context context) {
        super(context);
        init();
    }

    public ParticlesView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        glowPaint.setStyle(Paint.Style.FILL);
        corePaint.setStyle(Paint.Style.FILL);
        bgPaint.setStyle(Paint.Style.FILL);
        bgPaint.setColor(0x00000000);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
    }

    private void createParticles(int width, int height) {
        particles.clear();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        // Red particles disabled
    }

    private class BokehParticle {
        float x, y;
        float speedY;
        float speedX;
        float radius;
        int alpha;
        int color;

        BokehParticle(int width, int height, boolean randomizeY) {
            reset(width, height, randomizeY);
        }

        void reset(int width, int height, boolean randomizeY) {
            x = random.nextFloat() * width;
            y = randomizeY ? random.nextFloat() * height : height + 20;
            radius = (float) (dp(4) + random.nextFloat() * dp(8));
            speedY = -(0.3f + random.nextFloat() * 0.9f);
            speedX = (random.nextFloat() - 0.5f) * 0.4f;
            alpha = 110 + random.nextInt(125);

            float chance = random.nextFloat();
            if (chance > 0.3f) {
                color = 0xFFFF0000;
            } else if (chance > 0.1f) {
                color = 0xFFFF2222;
            } else {
                color = 0xFFCC0000;
            }
        }

        void update(int width, int height) {
            y += speedY;
            x += speedX;
            if (y < -40 || x < -40 || x > width + 40) {
                reset(width, height, false);
            }
        }

        private float dp(float val) {
            return val * getResources().getDisplayMetrics().density;
        }
    }
}
