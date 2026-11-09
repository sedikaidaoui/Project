#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")
#include <d3d11.h>
#include <tchar.h>


namespace font
{
	inline ImFont* icomoon_logo = nullptr;
	inline ImFont* description_font = nullptr;
	inline ImFont* esp_font = nullptr;
	inline ImFont* regular_m = nullptr;
	inline ImFont* regular_l = nullptr;
	inline ImFont* icomoon_page = nullptr;
	inline ImFont* small_font = nullptr;
	inline ImFont* inter_semibold = nullptr;
	inline ImFont* s_inter_semibold = nullptr;
	inline ImFont* inter_medium = nullptr;
	inline ImFont* icon_notify = nullptr;
}

namespace utils
{
	inline float CalculateTextWidthWithoutColorCodes(const ImFont* font, const char* text_begin, const char* text_end = nullptr) {
		if (!text_end)
			text_end = text_begin + strlen(text_begin); // Если text_end не указан, вычисляем его

		float text_width = 0.0f;
		const char* s = text_begin;

		while (s < text_end) {
			if (*s == '^' && (s + 1) < text_end && *(s + 1) >= '0' && *(s + 1) <= '9') {
				// Пропускаем цветовой код (^ и следующую цифру)
				s += 2;
				continue;
			}

			// Декодируем символ UTF-8
			unsigned int c = (unsigned int)*s;
			if (c < 0x80) {
				s += 1;
			}
			else {
				s += ImTextCharFromUtf8(&c, s, text_end);
				if (c == 0) // Некорректный UTF-8
					break;
			}

			// Получаем глиф и добавляем его ширину к общей ширине текста
			const ImFontGlyph* glyph = font->FindGlyph((ImWchar)c);
			if (glyph) {
				text_width += glyph->AdvanceX;
			}
		}

		return text_width;
	}

	inline ImColor GetColorWithAlpha(ImColor color, float alpha)
	{
		return ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);
	}

	inline ImVec2 center_text(ImVec2 min, ImVec2 max, const char* text)
	{
		return min + (max - min) / 2 - ImGui::CalcTextSize(text) / 2;
	}

	inline ImColor GetDarkColor(const ImColor& color)
	{
		// �������� �������� ������� ����� �� �������� �����.
		float r, g, b, a;
		r = color.Value.x;
		g = color.Value.y;
		b = color.Value.z;
		a = 255;

		float darkPercentage = 0.2f;
		float darkR = r * darkPercentage;
		float darkG = g * darkPercentage;
		float darkB = b * darkPercentage;

		return ImColor(darkR, darkG, darkB, a);
	}
	inline ImVec4 ImColorToImVec4(const ImColor& color)
	{
		return ImVec4(color.Value.x, color.Value.y, color.Value.z, color.Value.w);
	}

}




inline namespace c
{
	inline ID3D11ShaderResourceView* highlight_image = nullptr;
	inline bool is_picker_open = false;
	inline ImRect main_window_rect;

	inline ImColor dark_color(30, 30, 30, 255);          // Очень тёмный серый
	inline ImColor second_color(255, 255, 255, 15);      // Темно-серый
	inline ImColor background_color(22, 22, 24, 255);    // Почти чёрный фон
	inline ImColor stroke_color(255, 255, 255, 15);      // Полупрозрачный белый для разделителей
	inline ImColor window_bg_color(8, 2, 4, 210);        // Очень тёмный фон с насыщенным кроваво-красным оттенком

	// Bloody red accent theme
	inline ImVec4 accent = ImColor(200, 20, 40);         // Глубокий кроваво-красный акцент
	inline ImVec4 separator = ImColor(40, 40, 40);       // Тёмно-серый цвет для разделителей

	inline namespace anim
	{
		inline float speed;
		inline ImColor active(200, 20, 40, 255);          // Основной активный кроваво-красный цвет
		inline ImColor regular(20, 20, 23, 255);          // Объекты по умолчанию с тёмным цветом
	}

	inline namespace bg
	{
		inline ImVec4 background = ImColor(10, 10, 10);   // Очень тёмный фон
		inline ImVec2 size = ImVec2(920, 700);            // Размер остаётся прежним
		inline float rounding = 10.f;                     // Закруглённые углы для более мягкого вида
	}


	inline namespace child
	{
		inline ImVec4 background = ImColor(15, 15, 19, 0); // Тёмный фон для дочерних элементов
		inline ImVec4 stroke = ImColor(25, 25, 30);        // Тёмно-серый stroke
		inline float rounding = 5.f;                       // Уменьшенное округление
	}

	namespace page
	{
		inline ImVec4 background_active = ImColor(35, 6, 10);   // Тёмный кроваво-красный фон активной страницы
		inline ImVec4 background = ImColor(20, 4, 6);           // Ещё более тёмный красный фон страницы

		inline ImVec4 text_hov = ImColor(255, 185, 190);        // Светлый кроваво-красный текст при наведении
		inline ImVec4 text = ImColor(235, 200, 205);            // Мягкий красный текст

		inline float rounding = 5.f;                            // Закруглённые углы для страниц
	}

	inline namespace elements
	{
		inline ImVec4 background_hovered = ImColor(40, 8, 12);  // Более насыщенный красный при наведении
		inline ImVec4 background = ImColor(22, 8, 10, 255);     // Тёмный кроваво-красный фон элементов
		inline float rounding = 5.f;                            // Достаточно округленные элементы
	}

	inline namespace checkbox
	{
		inline ImVec4 mark = ImColor(200, 20, 40, 255);  // Кроваво-красный цвет для отметок
	}

	inline namespace text
	{
		inline namespace label
		{
			inline ImColor active = ImColor(255, 255, 255, 255);   // Белый для активных текстов
			inline ImColor hovered = ImColor(220, 220, 220, 255);   // Светло-зелёный для наведения
			inline ImColor regular = ImColor(200, 200, 200, 255);    // Мягкий зелёный цвет для обычного текста
		}

		inline namespace description
		{
			inline ImColor active = ImColor(230, 230, 255, 255);    // Тёмный цвет для активной подсказки
			inline ImColor hovered = ImColor(200, 200, 225, 255);    // Даже более тёмный для наведения
			inline ImColor regular = ImColor(118, 118, 131, 255);     // Ещё более тёмный для обычного состояния
		}
	}
}

